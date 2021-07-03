// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
//
// CallSequence.cpp
// 
// ======================================================================================
#include "LTSCommon.h"
#include "LegacyCompat.h"
#include <set>
#include <thread>

//typedef vector<ThreadID> ThreadPool;
//typedef vector<AppDomainID> AppDoaminPool;
//
typedef std::set<ModuleID> ModulePool;

#define TIMER 0
#define MODULELOADED 1
//#define APPDOMAINCREATED 2
#define READYTOVERIFY 2
#define SAMPLINGTHREAD 3
#define ALLEVENTS 4

AutoEvent callSqquenceEvent;
AutoEvent threadEvent;

int notifiedEvents[ALLEVENTS + 1] = { 0 };

#define MAX_OBJECT_RANGES 0x100
#define THREADSTATIC L"myThreadStaticTestVar"
#define APPDOMAINSTATIC L"myAppdomainStaticTestVar"
#define CONTEXTSTATIC L"myContextStaticTestVar"
#define RVASTATIC L"myRVAStaticTestVar"

#pragma region GC Structures

#define CallSequence_FAILURE(message)       \
    {                                    \
        m_ulFailure++;                   \
        FAILURE(message)                 \
    }                                       
#pragma endregion // GC Structures

class CallSequence
{
    public:

        CallSequence(IPrfCom * pPrfCom);
        ~CallSequence();

        #pragma region static_wrapper_methods

        static HRESULT ModuleLoadFinishedWrapper(IPrfCom * pPrfCom, ModuleID moduleID,HRESULT hrStatus)
        {
            return STATIC_CLASS_CALL(CallSequence)->ModuleLoadFinished(pPrfCom, moduleID, hrStatus);
        }

        static HRESULT ModuleUnloadStartedWrapper(IPrfCom * pPrfCom, ModuleID moduleID)
        {
            return STATIC_CLASS_CALL(CallSequence)->ModuleUnloadStarted(pPrfCom, moduleID);
        }

        static HRESULT GarbageCollectionFinishedWrapper(IPrfCom * pPrfCom)
        {
            return STATIC_CLASS_CALL(CallSequence)->GarbageCollectionFinished(pPrfCom);
        }

        static HRESULT RuntimeResumeStartedWrapper(IPrfCom * pPrfCom)
        {
            return STATIC_CLASS_CALL(CallSequence)->RuntimeResumeStarted(pPrfCom);
        }
        

        static HRESULT RuntimeResumeFinishedWrapper(IPrfCom * pPrfCom)
        {
            return STATIC_CLASS_CALL(CallSequence)->RuntimeResumeFinished(pPrfCom);
        }        


        static HRESULT CallSequence::ProfilerAttachCompleteWrapper(IPrfCom * pPrfCom)
        {
            return STATIC_CLASS_CALL(CallSequence)->ProfilerAttachComplete(pPrfCom);
        }

        static HRESULT VerifyWrapper(IPrfCom * pPrfCom)
        {
            return STATIC_CLASS_CALL(CallSequence)->Verify(pPrfCom);
        }
        static CallSequence* Instance()
        {
            return This;
        }

        #pragma endregion // static_wrapper_methods

        DWORD WINAPI CallSequence::AsynchThreadFunc(LPVOID pParam);
        //HRESULT CallSequence::DoTheTest(IPrfCom *  pPrfCom, DWORD dwEvent);
        HRESULT CallSequence::DoTheTest2(IPrfCom *  pPrfCom, HRESULT hrExpected = S_OK);
        HRESULT CallSequence::DoTheTest3(IPrfCom * pPrfCom, HRESULT hrExpected = S_OK);
        HRESULT CallSequence::Verify(IPrfCom *  pPrfCom );


    private:

        static CallSequence* This;

        #pragma region callback_handler_prototypes
        HRESULT ModuleLoadFinished(IPrfCom * pPrfCom, ModuleID moduleId, HRESULT hrStatus);
        HRESULT ModuleUnloadStarted(IPrfCom * pPrfCom, ModuleID moduleId);

        HRESULT GarbageCollectionFinished(IPrfCom * pPrfCom);
        HRESULT RuntimeResumeFinished(IPrfCom * pPrfCom);
        HRESULT RuntimeResumeStarted(IPrfCom * pPrfCom);
        HRESULT ProfilerAttachComplete(IPrfCom * pPrfCom);

        
        
        #pragma endregion // callback_handler_prototypes

        /*  Success/Failure counters */
        ULONG m_ulSuccess;
        ULONG m_ulFailure;

        IPrfCom * m_pPrfCom;

        ModulePool modulePool;
        
        ULONG m_ulForceGC;
        ULONG m_ulProfilerAttachComplete;
        ULONG m_ulEnumModuleFAIL;
        ULONG m_ulEnumModuleOK;
        ULONG m_ulCallSequenceContainingModuleFAIL;
        ULONG m_ulCallSequenceContainingModuleOK;
};
CallSequence* CallSequence::This = NULL;

HRESULT CallSequence::ModuleLoadFinished(IPrfCom * pPrfCom,
                                    ModuleID moduleId,
                                    HRESULT hrStatus)
{
    if (FAILED(hrStatus)) return S_OK;
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    modulePool.insert(moduleId);
    notifiedEvents[MODULELOADED] = 1;
    callSqquenceEvent.Signal();
    return S_OK;
} 
HRESULT CallSequence::ModuleUnloadStarted(IPrfCom * pPrfCom,
                                    ModuleID moduleId)
{
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    modulePool.erase(moduleId);
    return S_OK;
}

HRESULT CallSequence::DoTheTest2(IPrfCom * pPrfCom, HRESULT hrExpected)
{
    ThreadID threadId=0;
    HRESULT hr = PINFO->GetCurrentThreadID(&threadId);
    ICorProfilerModuleEnum * pModuleEnum = NULL;

    if (FAILED(hrExpected))
    {
        m_ulEnumModuleFAIL++;
    }
    else
    {
        m_ulEnumModuleOK++;
    }

    hr = PINFO->EnumModules( &pModuleEnum);
    if (hrExpected != hr)
    {
        CallSequence_FAILURE( L"DoTheTest2 EnumModules Failed, expected hr = " << hrExpected << L", returned " << hr );
        return hr;
    }
    if (S_OK != hr)
    {
        if (hrExpected == hr) return hr;
        CallSequence_FAILURE(L"ICorProfilerInfo::EnumModules returned 0x" << std::hex << hr);
    }

    ULONG cModules = 0;
    hr = pModuleEnum->GetCount(& cModules);
    if (S_OK != hr)
    {
        CallSequence_FAILURE(L"ICorProfilerModuleEnum::GetCount returned 0x" << std::hex << hr);
    }

    // Retrieve all the module info ..........
    ObjectID * pObjects = NULL;
    pObjects = new ObjectID[cModules];
    if (NULL == pObjects)
    {
        CallSequence_FAILURE(L"Failed to allocate memory");
    }
    
    ULONG cObjFetched = 0;
    hr = pModuleEnum->Next(cModules, pObjects, & cObjFetched);
    if (S_OK != hr)
    {
        CallSequence_FAILURE(L"ICorProfilerModuleEnum::Next returned 0x" << std::hex << hr);
    }
    if (cObjFetched != cModules)
    {
        CallSequence_FAILURE(L"cObjFetched != * pcModules");
    }

    // For each module in the enumeration .......
    //(ModuleID) pObjects[idx]
    ULONG32 assumeCount=10;
    ULONG32 cAppDomainIds=0;
    AppDomainID *appDomainIds = (AppDomainID *)malloc(assumeCount*sizeof(AppDomainID));
    if (appDomainIds==NULL) CallSequence_FAILURE("Out Of Memory");
    memset(appDomainIds, 0, assumeCount*sizeof(AppDomainID));
    for (unsigned int idx = 0; idx < cObjFetched; idx++)
    {
        if (FAILED(hrExpected))
        {
            m_ulCallSequenceContainingModuleFAIL++;
        }
        else
        {
            m_ulCallSequenceContainingModuleOK++;
        }

        hr = PINFO->GetAppDomainsContainingModule((ModuleID) pObjects[idx], assumeCount,&cAppDomainIds, appDomainIds);
        if (hrExpected != hr)
        {
            CallSequence_FAILURE(L"DoTheTest2 Failed, expected hr = "<<HEX(hrExpected) <<", returned "<< HEX(hr) );
            return hr;
        }
        if (hr!=S_OK)
        {
            if (hrExpected == hr) return hr;
            CallSequence_FAILURE(L"CallSequenceContainingModule Failed.");
        }
    }
    return hr;
}

HRESULT CallSequence::DoTheTest3(IPrfCom * pPrfCom,  HRESULT hrExpected)
{
    HRESULT hr = S_OK;
    ULONG32 cModuleIds= (ULONG32)modulePool.size();
    ULONG count=0;
    ULONG assumeCount = 10;
    ULONG32 cAppDomainIds=0;
    AppDomainID *appDomainIds = (AppDomainID *)malloc(assumeCount*sizeof(AppDomainID));
    if (appDomainIds==NULL) CallSequence_FAILURE("Out Of Memory");
    memset(appDomainIds, 0, assumeCount*sizeof(AppDomainID));
    
    std::set<ModuleID>::iterator iEnd = modulePool.end();
    for (std::set<ModuleID>::iterator iBegin = modulePool.begin(); iBegin!=iEnd;++iBegin)
    {
        ModuleID moduleId = *iBegin;
        if (FAILED(hrExpected))
        {
            m_ulCallSequenceContainingModuleFAIL++;
        }
        else
        {
            m_ulCallSequenceContainingModuleOK++;
        }
        hr = PINFO->GetAppDomainsContainingModule(moduleId , assumeCount,&cAppDomainIds, appDomainIds);
        if (hrExpected != hr)
        {
            CallSequence_FAILURE(L"DoTheTest3 Failed, expected hr = "<<HEX(hrExpected) <<", returned "<< HEX(hr) );
            return hr;
        }
        if (hr!=S_OK)
        {
            if (hrExpected == hr) return hr;
            CallSequence_FAILURE(L"CallSequenceContainingModule Failed.");
        }
    }
    return hr;
}


HRESULT CallSequence::GarbageCollectionFinished(IPrfCom * pPrfCom)
{
    ThreadID threadId=0;
    HRESULT hr = S_OK;
    hr = DoTheTest2(pPrfCom, CORPROF_E_UNSUPPORTED_CALL_SEQUENCE);
    hr = DoTheTest3(pPrfCom, CORPROF_E_UNSUPPORTED_CALL_SEQUENCE);
    return hr;
}
HRESULT CallSequence::RuntimeResumeStarted(IPrfCom * pPrfCom)
{
    ThreadID threadId=0;
    HRESULT hr = PINFO->GetCurrentThreadID(&threadId);
    hr = DoTheTest2(pPrfCom, CORPROF_E_UNSUPPORTED_CALL_SEQUENCE);
    hr = DoTheTest3(pPrfCom, CORPROF_E_UNSUPPORTED_CALL_SEQUENCE);
    return hr;
}
HRESULT CallSequence::RuntimeResumeFinished(IPrfCom * pPrfCom)
{
    ThreadID threadId=0;
    HRESULT hr = PINFO->GetCurrentThreadID(&threadId);
    hr = DoTheTest2(pPrfCom);
    hr = DoTheTest3(pPrfCom);
    return hr;
}
HRESULT CallSequence::ProfilerAttachComplete(IPrfCom * pPrfCom)
{
    HRESULT hr = S_OK;
    m_ulProfilerAttachComplete++;
    hr = DoTheTest2(pPrfCom);
    hr = DoTheTest3(pPrfCom);
    return hr;
}


DWORD WINAPI AsynchronousCallSequenceThreadFunc(LPVOID pParam)
{
    CallSequence::Instance()->AsynchThreadFunc(pParam);
    return 0;
}

DWORD WINAPI TimerThreadFunc()
{
	this_thread::sleep_for(std::chrono::nanoseconds(100));
    notifiedEvents[TIMER] = 1;
    callSqquenceEvent.Signal();
	return 0;
}

//HRESULT CallSequence::DoTheTest(IPrfCom * pPrfCom, DWORD dwEvent)
//{
//    HRESULT hr=S_OK;    
//    ULONG32 cClassIds=classPool.size();
//    ULONG32 cAppDomainIds=0;
//    ULONG count=0;
//    DISPLAY(cClassIds << L"classids ." );
//    for (ULONG cls = 0;cls<cClassIds;++cls)
//    {
//        ClassID classId = classPool.at(cls);
//        if (classId == 0) continue;
//        ModuleID moduleId = NULL;
//        mdTypeDef token = NULL;
//        WCHAR tokenName[256];
//        HCORENUM hEnum  = NULL;    
//        mdFieldDef fieldTokens[STRING_LENGTH];
//        ULONG cTokens   = NULL;
//        ULONG nameLength = NULL;
//    
//        ModuleID classModuleId = NULL;
//        IMetaDataImport *pIMDImport = NULL;
//
//    
//        DWORD corElementType = NULL;
//        PCCOR_SIGNATURE pvSig = NULL;
//        ULONG cbSig = NULL;
//        DWORD fieldAttributes = NULL;
//
//
//
//        MUST_PASS(hr = PINFO->GetClassIDInfo2(classId, &moduleId, &token, NULL, NULL, NULL, NULL));
//        MUST_PASS(PINFO->GetModuleMetaData(moduleId, ofRead, IID_IMetaDataImport, (IUnknown **)&pIMDImport));
//        MUST_PASS(pIMDImport->GetTypeDefProps(token, tokenName, 256, NULL, NULL, NULL));
//        MUST_PASS(pIMDImport->EnumFields(&hEnum, token, fieldTokens, STRING_LENGTH, &cTokens));
//                    
//        ULONG32 assumeCount=5+1;
//        AppDomainID *appDomainIds = (AppDomainID *)malloc(assumeCount*sizeof(AppDomainID));
//        if (appDomainIds==NULL) CallSequence_FAILURE("Out Of Memory");
//        memset(appDomainIds, 0, assumeCount*sizeof(AppDomainID));
//        DISPLAY(L"CallSequenceContainingModule");
//        hr = PINFO->CallSequenceContainingModule(moduleId, assumeCount,&cAppDomainIds, appDomainIds);
//        if (FAILED(hr))
//        {
//              DISPLAY(cAppDomainIds << L" Appdomains returned, hr = " << HEX(hr));
//              return hr;
//        }
//        if (assumeCount<cAppDomainIds)
//        {
//            assumeCount=cAppDomainIds+1;
//            appDomainIds = (AppDomainID *)realloc(appDomainIds, assumeCount*sizeof(AppDomainID));
//            if (appDomainIds==NULL) CallSequence_FAILURE("Out Of Memory");
//            memset(appDomainIds, 0, assumeCount*sizeof(AppDomainID));
//            DISPLAY(L"CallSequenceContainingModule");
//            hr = PINFO->CallSequenceContainingModule(moduleId, assumeCount,&cAppDomainIds, appDomainIds);
//            if (FAILED(hr))
//            {
//                DISPLAY(cAppDomainIds << L" Appdomains returned, hr = " << HEX(hr));
//                return hr;
//            }
//        }
//        if (appDomainIds[cAppDomainIds]!=0)
//                CallSequence_FAILURE(L"appDomainIds["<<cAppDomainIds<<L"] = " <<appDomainIds[cAppDomainIds] << L" while zero expected.");
//        PVOID staticAddress=0;
//
//        for (ULONG tk = 0; tk < cTokens; ++tk)
//        {
//            ClassID fieldClassId = NULL;
//            mdTypeDef fieldClassToken;
//            MUST_PASS(pIMDImport->GetFieldProps(fieldTokens[tk],    // The field for which to get props.
//                                            &fieldClassToken,  // Put field's class here.
//                                            tokenName,         // Put field's name here.
//                                            256,               // Size of szField buffer in wide chars.
//                                            &nameLength,       // Put actual size here
//                                            &fieldAttributes,  // Put flags here.
//                                            &pvSig,            // [OUT] point to the blob value of meta data
//                                            &cbSig,            // [OUT] actual size of signature blob
//                                            &corElementType,   // [OUT] flag for value type. selected ELEMENT_TYPE_*
//                                            NULL,              // [OUT] constant value
//                                            NULL));            // [OUT] size of constant value, string only, wide chars
//
//            if(IsFdStatic(fieldAttributes))
//            {
//                if (S_OK == pIMDImport->GetCustomAttributeByName(fieldTokens[tk], L"System.ThreadStaticAttribute", NULL, NULL))
//                {
//                    ULONG valid=0;
//                    ULONG valid2=0;
//                    for(ULONG i=0;i<tPool.size();++i)
//                    {
//                        //The static can be found in number(valid) of  thread+appdomain pairs
//                        for (ULONG ad = 0;ad<adPool.size();++ad)
//                        {
//                    
//                            if (tPool.at(i) == 0) continue;
//                            HRESULT hr = PINFO->GetThreadStaticAddress2(classId, fieldTokens[tk], adPool.at(ad), tPool.at(i), &staticAddress);
//                            if (SUCCEEDED(hr))
//                            {
//                                valid++;
//                            }
//                        }
//
//                        //The static can be found in number(valid2) of  thread+appdomain pairs
//                        for (ULONG ad = 0;ad<cAppDomainIds;++ad)
//                        {
//                            if (tPool.at(i) == 0) continue;
//                            HRESULT hr = PINFO->GetThreadStaticAddress2(classId, fieldTokens[tk], appDomainIds[ad], tPool.at(i), &staticAddress);
//                            //DISPLAY(L"GetThreadStaticAddress2 " << HEX(hr) << "  appDomainIds " << appDomainIds[ad] << L" tPool.at(i) " << tPool.at(i));
//                            if (SUCCEEDED(hr))
//                            {
//                                valid2++;
//                            }
//                        }
//                    }
//                    if (valid2<valid )
//                        CallSequence_FAILURE(L"ClassId " << classId << " is not found in enough appdomains, " << valid << " expected while " <<valid2 << " in fact." );
//                }
//                else if (S_OK == pIMDImport->GetCustomAttributeByName(fieldTokens[tk], L"System.ContextStaticAttribute", NULL, NULL))
//                {
//                }
//                else if (IsFdHasFieldRVA(fieldAttributes))
//                {
//                }
//                else //Appdomain static 
//                {  
//
//                    //For every appdomain returned by the API, CallSequencetaticAddress works unless the appdomain is NOT initialized yet.
//                    for (ULONG ad = 0;ad<cAppDomainIds;++ad)
//                    {
//                        HRESULT hr = PINFO->CallSequencetaticAddress(classId, fieldTokens[tk], appDomainIds[ad], &staticAddress);
//                        if (FAILED(hr))
//                        {
//                            if (CORPROF_E_DATAINCOMPLETE == hr)m_ulDataIncomplete++;
//                            else m_ulFailure++;
//                        }
//                        else
//                        {
//                            PULONG ptr =  static_cast<PULONG>(staticAddress);  
//                            if (*ptr != 0xDEF) m_ulFailure++;
//                            DISPLAY(L"Appdomain Static " << HEX(staticAddress) << " value " << HEX(*ptr));
//                        }
//                    }
//                    
//                    //This moment, all appdomains should be initialized
//                    if (dwEvent == WAIT_OBJECT_0 + READYTOVERIFY)
//                    {   
//                        //every class is initialized into a appdomain
//                        count = 0;
//                        for(ULONG i=0;i<adPool.size();++i)
//                        {
//                            if (adPool.at(i) != 0)
//                            {
//                                //For domain neutral modules, the CallSequencetaticAddress returns correct value only if the appdomainId 
//                                // is correct; For none domain neutral modules, the adddomainId is meaningless here, what ever you pass in,
//                                // as long as it is a valid appdomainId, the API will give correct return values.
//                                HRESULT hra = PINFO->CallSequencetaticAddress(classId, fieldTokens[tk], adPool.at(i), &staticAddress);
//                                if (SUCCEEDED(hra)) 
//                                {
//                                    count++;
//                                    ULONG adi=0;
//                                    for (adi = 0;adi<cAppDomainIds;++adi)
//                                    {
//                                        if (appDomainIds[adi] ==  adPool.at(i) ) break;
//                                    }
//                                    //if appdomainId adPool.at(i) is not included in appDomainIds
//                                    if (adi == cAppDomainIds)
//                                        CallSequence_FAILURE(L"AppdomainId "<<adPool.at(i)<<" is not included in appDomainIds.");
//                                }
//                            }
//                        }//for all appdomains of the process
//                   
//                        if (count!=cAppDomainIds)
//                            CallSequence_FAILURE(count << L"Appdomains other than " << cAppDomainIds << " containing module.");
//                        //terminate thread    
//                        return hr;
//                    }
//                }//if appdomain static
//            }//if static field
//        }//for tokens
//        free(appDomainIds);
//    }//for classids
//    return hr;
//}

DWORD WINAPI CallSequence::AsynchThreadFunc(LPVOID pParam)
{
    IPrfCom * pPrfCom = static_cast<IPrfCom *>(pParam);
    HRESULT hr = S_OK;
    while (TRUE)
    {
        DWORD dwEvent = 0;
        callSqquenceEvent.Wait();

        m_ulForceGC++;
        PINFO->ForceGC();

        if (notifiedEvents[READYTOVERIFY] == 1)
        {
            break;
        }
    }//while true

    threadEvent.Signal();
    return 0;
}

CallSequence::CallSequence(IPrfCom * pPrfCom)
{
    // Initialize result counters
    CallSequence::This = this;
    m_ulSuccess = 0;
    m_ulFailure = 0;
    m_ulForceGC = 0;
    m_ulProfilerAttachComplete = 0;
    m_ulEnumModuleFAIL = 0;
    m_ulEnumModuleOK = 0;
    m_ulCallSequenceContainingModuleFAIL = 0;
    m_ulCallSequenceContainingModuleOK = 0;
    m_pPrfCom = pPrfCom;

	try
	{
		std::thread t1(AsynchronousCallSequenceThreadFunc, pPrfCom);
		t1.detach();
	}
	catch (std::system_error) 
	{
		CallSequence_FAILURE(L"Failing to create thread.");
	}

	try
	{
		thread timer(TimerThreadFunc);
		timer.detach();
	}
	catch (std::system_error)
	{
		CallSequence_FAILURE(L"Failing to set timer.");
	}
}

/*
 *  Clean up
 */
CallSequence::~CallSequence()
{
    CallSequence::This = NULL;
    //ToDo: Kill Unmanaged Thread
}


HRESULT CallSequence::Verify(IPrfCom * pPrfCom)
{
    notifiedEvents[READYTOVERIFY] = 1;
    callSqquenceEvent.Signal();

    threadEvent.Wait();
    DISPLAY(L"\nm_ulForceGC = "<<m_ulForceGC <<
            L"\nm_ulProfilerAttachComplete = "<<m_ulProfilerAttachComplete <<
            L"\nm_ulEnumModuleFAIL = "<<m_ulEnumModuleFAIL <<
            L"\nm_ulEnumModuleOK = "<<m_ulEnumModuleOK <<
            L"\nCallSequenceContainingModuleFAIL = "<<m_ulCallSequenceContainingModuleFAIL <<
            L"\nCallSequenceContainingModuleOK = "<<m_ulCallSequenceContainingModuleOK <<
            L"\nm_ulFailure = "<<m_ulFailure
            );
            
    if (m_ulFailure > 0 )
    {
        CallSequence_FAILURE(L"Failing test due to ERRORS above.");
        return E_FAIL;
    }
    return S_OK;
}

/*
 *  Verification routine called by TestProfiler
 */
HRESULT CallSequence_Verify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(CallSequence);
    HRESULT hr = pCallSequence->Verify(pPrfCom);

    // Clean up instance of test class
    delete pCallSequence;
    pPrfCom->m_pTestClassInstance = NULL;

    return hr;
}


/*
 *  Initialization routine called by TestProfiler
 */
void CallSequence_Initialize(ILTSCom * pLTSCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    IPrfCom * pPrfCom = dynamic_cast<IPrfCom *>(pLTSCom);
    DISPLAY(L"Initialize CallSequence.\n")

    // Create and save an instance of test class
    SET_CLASS_POINTER(new CallSequence(pPrfCom));

    // Initialize MethodTable
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_THREADS|
                                COR_PRF_MONITOR_APPDOMAIN_LOADS|
                                COR_PRF_MONITOR_CLASS_LOADS|
                                COR_PRF_MONITOR_MODULE_LOADS|
                                COR_PRF_MONITOR_GC|
                                COR_PRF_MONITOR_SUSPENDS;

    REGISTER_CALLBACK(MODULELOADFINISHED,         CallSequence::ModuleLoadFinishedWrapper);
    REGISTER_CALLBACK(MODULEUNLOADSTARTED,        CallSequence::ModuleUnloadStartedWrapper);

    //REGISTER_CALLBACK(GARBAGECOLLECTIONFINISHED, CallSequence::GarbageCollectionFinishedWrapper);
    REGISTER_CALLBACK(RUNTIMERESUMESTARTED,     CallSequence::RuntimeResumeStartedWrapper);
    REGISTER_CALLBACK(RUNTIMERESUMEFINISHED,     CallSequence::RuntimeResumeFinishedWrapper);
    REGISTER_CALLBACK(PROFILERATTACHCOMPLETE,     CallSequence::ProfilerAttachCompleteWrapper);
    
    REGISTER_CALLBACK(VERIFY,                    CallSequence::VerifyWrapper);

    return;
}

