#include "../../ProfilerCommon.h"
#include "../../LegacyCompat.h" // Whidbey Framework Support

#include "../../../holder.h"

#include <condition_variable>
#include <chrono>
#include <thread>

#ifndef WINDOWS
#include <pthread.h>
#endif


/*******************************************************************************
 *
 *  This test serves as a regression case for two similar bugs.
 *
 *  522028 -PROFILER BUG BASH: CorProfInfo::GetAppDomainStaticAddress failing from
 *      GarbageCollectionFinished when GC started by ForceGC
 *
 *  522030 -PROFILER BUG BASH: CorProfInfo::GetAppDomainStaticAddress assert failure
 *      "index < m_aModuleIndices" in GarbageCollectionFinished
 *
 ******************************************************************************/

// In order to call GetAppDomainStaticAddress on a class, you have to know what AppDomain
// The class lives in.  This map keeps track of the correct AppDomain for each loaded class.
typedef map<ClassID, AppDomainID>ClassAppDomainMap;

#define STOP 0
#define GC 1
#define ALLEVENTS 2

class VSW522028 // & VSW522030
{
public:

    VSW522028(IPrfCom * pPrfCom);

    void StartThreadInitialization(IPrfCom * pPrfCom);

    static HRESULT ClassLoadFinishedWrapper(
                    IPrfCom *pPrfCom,
                    ClassID classId,
                    HRESULT hrStatus)

    {
        return Instance()->ClassLoadFinished(pPrfCom, classId, hrStatus);
    }

    static HRESULT GarbageCollectionFinishedWrapper(
                    IPrfCom *pPrfCom)
    {
        return Instance()->GarbageCollectionFinished(pPrfCom);
    }

    VOID AsynchThreadFunc(LPVOID pParam);

    HRESULT Verify(IPrfCom * pPrfCom);

    static VSW522028* Instance()
    {
        return This;
    }

private:

    HRESULT ClassLoadFinished(
                    IPrfCom *pPrfCom,
                    ClassID classId,
                    HRESULT hrStatus);

    HRESULT GarbageCollectionFinished(
                    IPrfCom *pPrfCom);

    static VSW522028* This;

    /*  Handle to asynchronous sampling thread. */
    AutoEvent m_hAsynchThreadEvent;

    ULONG m_success;
    ULONG m_failure;
    ULONG m_cForceGC;

    ClassAppDomainMap m_classADMap;

    AutoEvent m_gcCollectSignal;

    int gcCollectEvents[ALLEVENTS];
};


/*
 *  ThreadFunc is used  to create the asynchronous ForceGC thread.
 */
DWORD WINAPI VSW522028AsynchronousThreadFunc(LPVOID pParam)
{
#ifdef WINDOWS
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#endif
    VSW522028::Instance()->AsynchThreadFunc(pParam);
    return 0;
}


VSW522028::VSW522028(IPrfCom * pPrfCom)
{
    VSW522028::This = this;
    m_success = m_failure = m_cForceGC = 0;
}

void VSW522028::StartThreadInitialization(IPrfCom * pPrfCom)
{
    thread t1(VSW522028AsynchronousThreadFunc, pPrfCom);
#ifdef LINUX
    int priority = 0;
    sched_param sch;
    sch.sched_priority = 0;
    pthread_setschedparam(t1.native_handle(), priority, &sch);
#endif
    t1.detach();
}


HRESULT VSW522028::ClassLoadFinished(
                    IPrfCom *pPrfCom,
                    ClassID classId,
                    HRESULT hrStatus)
{
    HRESULT hr = S_OK;

    ThreadID threadId = NULL;
    AppDomainID appDomainId = NULL;
    CorElementType baseElemType;
    ClassID        baseClassId;
    ULONG          cRank;

    // We don't care about array classes, so skip them.

    hr = PINFO->IsArrayClass(
        classId,
        &baseElemType,
        &baseClassId,
        &cRank);
    if (hr == S_OK)
    {
        return S_OK;
    }


    hr = PINFO->GetCurrentThreadID(&threadId);
    if (FAILED(hr))
    {
        FAILURE(L"GetCurrentThreadID returned 0x" << HEX(hr) << L"\n");
        m_failure++;
        return hr;
    }

    hr = PINFO->GetThreadAppDomain(threadId, &appDomainId);
    if (FAILED(hr))
    {
        FAILURE(L"GetThreadAppDomain returned 0x" << HEX(threadId) << " for ThreadID 0x" << HEX(hr) << "\n");
        m_failure++;
        return hr;
    }

    m_classADMap.insert(make_pair(classId, appDomainId));

    gcCollectEvents[GC] = 1;
    m_gcCollectSignal.Signal();
    return hr;
}


HRESULT VSW522028::GarbageCollectionFinished(
                    IPrfCom *pPrfCom)
{
    HRESULT hr = S_OK;

    for (ClassAppDomainMap::iterator iCADM = m_classADMap.begin();
            iCADM != m_classADMap.end();
            iCADM++)
    {
        ClassID classId = iCADM->first;
        AppDomainID appDomainId = iCADM->second;

        ModuleID classModuleId = NULL;
        hr = PINFO->GetClassIDInfo2(classId,
                                    &classModuleId,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);
        if (FAILED(hr))
        {
            FAILURE(L"GetClassIDInfo2 returned 0x" << HEX(hr) << " for ClassID 0x" << HEX(classId) << L"\n");
            m_failure++;
            continue;
        }

        COMPtrHolder<IMetaDataImport> pIMDImport;

        hr = PINFO->GetModuleMetaData(classModuleId,
                                        ofRead,
                                        IID_IMetaDataImport,
                                        (IUnknown **)&pIMDImport);
        if (hr == CORPROF_E_DATAINCOMPLETE)
		{
			// Module is being unloaded...
			continue;
		}
		if (FAILED(hr))
        {
            FAILURE(L"GetModuleMetaData returned 0x" << HEX(hr) << L" for ModuleID 0x" << HEX(classModuleId) << L"\n");
            m_failure++;
            continue;
        }

        HCORENUM hEnum = NULL;
        mdTypeDef token = NULL;
        mdFieldDef fieldTokens[SHORT_LENGTH];
        ULONG cTokens = NULL;

        // Get class token to enum all field    s from MetaData.  (Needed for statics)
        hr = PINFO->GetClassIDInfo2(classId,
                                            NULL,
                                            &token,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL);
        if (hr == CORPROF_E_DATAINCOMPLETE)
        {
            // Class load not complete.  We can not inspect yet.
            continue;
        }
        if (FAILED(hr))
        {
            FAILURE(L"GetClassIDInfo2returned 0x" << HEX(hr) << "\n");
            m_failure++;
            continue;
        }

        // Enum all fields of the class from the MetaData
        hr = pIMDImport->EnumFields(&hEnum,
                                            token,
                                            fieldTokens,
                                            SHORT_LENGTH,
                                            &cTokens);
        if (FAILED(hr))
        {
            FAILURE(L"IMetaDataImport::EnumFields returned 0x" << HEX(hr) << "\n");
            m_failure++;
            continue;
        }

        for (ULONG i = 0; i < cTokens; i++)
        {
            mdTypeDef fieldClassToken = NULL;
            WCHAR tokenName[256];
            ULONG nameLength = NULL;
            DWORD fieldAttributes = NULL;
            PCCOR_SIGNATURE pvSig = NULL;
            ULONG cbSig = NULL;
            DWORD corElementType = NULL;

            hr = pIMDImport->GetFieldProps(fieldTokens[i],    // The field for which to get props.
                                            &fieldClassToken,                                  // Put field's class here.
                                            tokenName,                                           // Put field's name here.
                                            256,                                                      // Size of szField buffer in wide chars.
                                            &nameLength,                                        // Put actual size here
                                            &fieldAttributes,                                      // Put flags here.
                                            &pvSig,                                                  // [OUT] point to the blob value of meta data
                                            &cbSig,                                                  // [OUT] actual size of signature blob
                                            &corElementType,                                   // [OUT] flag for value type. selected ELEMENT_TYPE_*
                                            NULL,                                                     // [OUT] constant value
                                            NULL);                                                    // [OUT] size of constant value, string only, wide chars

            if (FAILED(hr))
            {
                FAILURE(L"GetFieldProps returned 0x" << HEX(hr) << " for Field " << i);
                m_failure++;
                continue;
            }

            if ((IsFdStatic(fieldAttributes)) && (!IsFdLiteral(fieldAttributes)))
            {
                COR_PRF_STATIC_TYPE fieldInfo = COR_PRF_FIELD_NOT_A_STATIC;
                hr = PINFO->GetStaticFieldInfo(classId, fieldTokens[i], &fieldInfo);
                if (FAILED(hr))
                {
                    FAILURE(L"GetStaticFieldInfo returned HR=0x" << HEX(hr) << " for field " << HEX(fieldTokens[i]) << " (" << tokenName << ")\n");
                    m_failure++;
                    continue;
                }

                if (fieldInfo & COR_PRF_FIELD_APP_DOMAIN_STATIC)
                {
                    PVOID staticOffSet = NULL;

                    hr = PINFO->GetAppDomainStaticAddress(classId,
                                                fieldTokens[i],
                                                appDomainId,
                                                &staticOffSet);

                    if (FAILED(hr) && (hr != CORPROF_E_DATAINCOMPLETE))
                    {
                        FAILURE(L"GetAppDomainStaticAddress Failed HR 0x" << HEX(hr) << "\n");
                        m_failure++;
                        continue;
                    }
                    m_success++;
                }

            }

        }

    }
    gcCollectEvents[GC] = 1;
    m_gcCollectSignal.Signal();
    return hr;
}


VOID VSW522028::AsynchThreadFunc(LPVOID pParam)
{
    IPrfCom *pPrfCom = (IPrfCom *)pParam;
	DISPLAY("AsynchThreadFunc Started");
	DWORD dwEvent = 0;
    while (true)
    {
        m_gcCollectSignal.Wait();

        if (gcCollectEvents[STOP] == 1)
        {
            gcCollectEvents[STOP] = 0;
            goto end;
        }
        else if (gcCollectEvents[GC] == 1)
        {
            gcCollectEvents[GC] = 0;
            YieldProc(10);
            PINFO->ForceGC();
            m_cForceGC++;
        }
    }
end:

    m_hAsynchThreadEvent.Signal();
	DISPLAY("AsynchThreadFunc Ended");
}


HRESULT VSW522028::Verify(IPrfCom * pPrfCom)
{
    gcCollectEvents[STOP] = 1;
    m_gcCollectSignal.Signal();
    m_hAsynchThreadEvent.WaitFor(100);
    
    if ((m_success > 0) && (m_cForceGC > 0) && (m_failure == 0))
    {
        DISPLAY(L"\nVSW522028 && VSW522030 Passed.  Success " << m_success << ", Failure " << m_failure << ", ForceGC " << m_cForceGC << "\n\n");
        return S_OK;
    }
    else
    {
        DISPLAY(L"\nVSW522028 && VSW522030 Failed.  Success " << m_success << ", Failure " << m_failure << ", ForceGC " << m_cForceGC << "\n\n");
        return E_FAIL;
    }
}


VSW522028* VSW522028::This = NULL;

VSW522028* global_VSW522028 = NULL;


HRESULT VSW522028_Verify(IPrfCom * pPrfCom)
{
    HRESULT hr = VSW522028::Instance()->Verify(pPrfCom);

    delete global_VSW522028;

    return hr;
}


void VSW_522028_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY( L"Initialize VSW522028 && VSW522030 Extension ...\n" );

    global_VSW522028 = new VSW522028(pPrfCom);
    global_VSW522028->StartThreadInitialization(pPrfCom);

    pModuleMethodTable->FLAGS =  COR_PRF_MONITOR_CLASS_LOADS |
                                                    COR_PRF_MONITOR_GC |
                                                    COR_PRF_USE_PROFILE_IMAGES;

    pModuleMethodTable->VERIFY = (FC_VERIFY) &VSW522028_Verify;
    pModuleMethodTable->CLASSLOADFINISHED = (FC_CLASSLOADFINISHED) &VSW522028::ClassLoadFinishedWrapper;
    pModuleMethodTable->GARBAGECOLLECTIONFINISHED = (FC_GARBAGECOLLECTIONFINISHED) &VSW522028::GarbageCollectionFinishedWrapper;

    return;
}
