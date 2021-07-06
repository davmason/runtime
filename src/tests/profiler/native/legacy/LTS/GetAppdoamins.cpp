// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
//
// GetAppdomains.cpp
// 
// ======================================================================================
#include "LTSCommon.h"
#include "../LegacyCompat.h"

typedef vector<ThreadID> ThreadPool;
typedef vector<AppDomainID> AppDoaminPool;
typedef vector<ClassID> ClassPool;
ClassPool classPool;
#define TIMER 0
#define CLASSLOADED 1
#define APPDOMAINCREATED 2
#define READYTOVERIFY 3
#define SAMPLINGTHREAD 4
#define ALLEVENTS 5

AutoEvent hEvent;
AutoEvent samplingThreadEvent;

int32_t dwEventValues[ALLEVENTS + 1] = {0};

#define MAX_OBJECT_RANGES 0x100
#define THREADSTATIC L"myThreadStaticTestVar"
#define APPDOMAINSTATIC L"myAppdomainStaticTestVar"
#define CONTEXTSTATIC L"myContextStaticTestVar"
#define RVASTATIC L"myRVAStaticTestVar"

#pragma region GC Structures

#define GetAppdomains_FAILURE(message)       \
    {                                    \
        m_ulFailure++;                   \
        FAILURE(message)                 \
    }                                       

#pragma endregion // GC Structures

class GetAppdomains
{
    public:

        GetAppdomains(IPrfCom * pPrfCom);
        ~GetAppdomains();

        #pragma region static_wrapper_methods

        static HRESULT ThreadCreatedWrapper(IPrfCom * pPrfCom,
                    ThreadID managedThreadId)
        {
            return STATIC_CLASS_CALL(GetAppdomains)->ThreadCreated(pPrfCom, managedThreadId);
        }   

        static HRESULT AppDomainCreationFinishedWrapper(IPrfCom * pPrfCom,
                    AppDomainID appDomainId,
                    HRESULT     hrStatus)
        {
            return STATIC_CLASS_CALL(GetAppdomains)->AppDomainCreationFinished(pPrfCom, appDomainId, hrStatus);    
        }

        static HRESULT AppDomainShutdownStartedWrapper(IPrfCom * pPrfCom,
                AppDomainID appDomainId)
        {
            return STATIC_CLASS_CALL(GetAppdomains)->AppDomainShutdownStarted(pPrfCom, appDomainId);  
        }

        static HRESULT ThreadDestroyedWrapper(IPrfCom * pPrfCom,
                        ThreadID managedThreadId)
        {
            return STATIC_CLASS_CALL(GetAppdomains)->ThreadDestroyed(pPrfCom, managedThreadId);
        }
         static HRESULT ThreadNameChangedWrapper(IPrfCom * pPrfCom,
                ThreadID threadId,
                ULONG cchName,
                WCHAR name[])
         {
            return STATIC_CLASS_CALL(GetAppdomains)->ThreadNameChanged(pPrfCom, threadId, cchName, name);
         }

        static HRESULT ClassLoadFinishedWrapper(IPrfCom * pPrfCom, ClassID classID,HRESULT hrStatus)
        {
            return STATIC_CLASS_CALL(GetAppdomains)->ClassLoadFinished(pPrfCom, classID, hrStatus);
        }

        static HRESULT ClassUnloadStartedWrapper(IPrfCom * pPrfCom, ClassID classId)
        {
            return STATIC_CLASS_CALL(GetAppdomains)->ClassUnloadStarted(pPrfCom, classId);
        }
        static HRESULT VerifyWrapper(IPrfCom * pPrfCom)
        {
            return STATIC_CLASS_CALL(GetAppdomains)->Verify(pPrfCom);
        }
        static GetAppdomains* Instance()
        {
            return This;
        }

        #pragma endregion // static_wrapper_methods

        DWORD WINAPI GetAppdomains::AsynchThreadFunc(LPVOID pParam);

        HRESULT GetAppdomains::Verify(IPrfCom *  pPrfCom );


    private:

        static GetAppdomains* This;

        #pragma region callback_handler_prototypes
        HRESULT ThreadCreated(IPrfCom * pPrfCom, ThreadID managedThreadId);
        HRESULT ThreadDestroyed(IPrfCom * pPrfCom, ThreadID managedThreadId);
        HRESULT AppDomainCreationFinished(IPrfCom * pPrfCom, AppDomainID appDomainId,HRESULT     hrStatus);
        HRESULT AppDomainShutdownStarted(IPrfCom * pPrfCom, AppDomainID appDomainId);  
        HRESULT ThreadNameChanged(IPrfCom * pPrfCom, ThreadID threadId, ULONG cchName,WCHAR name[]);
        HRESULT ClassLoadFinished(IPrfCom * pPrfCom, ClassID classID,HRESULT hrStatus);
        HRESULT ClassUnloadStarted(IPrfCom * pPrfCom, ClassID classId);
        #pragma endregion // callback_handler_prototypes

        /*  Success/Failure counters */
        ULONG m_ulSuccess;
        ULONG m_ulFailure;

        IPrfCom * m_pPrfCom;

        ThreadPool tPool;
        AppDoaminPool adPool;
        
        ULONG m_ulAppdomains;
        ULONG m_ulLiterals;
        ULONG m_ulErrorArgument;
        ULONG m_ulGetAppdomainStaticAddress;
        ULONG m_ulGetThreadStaticAddress2;
        ULONG m_ulGetThreadStaticAddress;
        ULONG m_ulDataIncomplete;
        ULONG m_ulGetContextStaticAddress;
        ULONG m_ulGetRVAStaticAddress;
        ULONG m_ulNewThreadInit;
        ULONG m_ulSOKWithZeroValue;
};
GetAppdomains* GetAppdomains::This = NULL;

//insert every managed threadid into threadpool
HRESULT GetAppdomains::ThreadCreated(IPrfCom * pPrfCom, ThreadID managedThreadId)
{
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    //DISPLAY(L"managedThread "<<HEX(managedThreadId)<<L" Created.");
    for(ULONG i=0;i<tPool.size();++i)
    {
        if (tPool.at(i) == managedThreadId)
        {
            return S_OK;
        }
    }
    tPool.push_back(managedThreadId);
    HRESULT hr = S_OK;
    
    return hr;  
}

HRESULT GetAppdomains::ThreadDestroyed(IPrfCom * pPrfCom, ThreadID managedThreadId)
{
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    for(ULONG i=0;i<tPool.size();++i)
    {
        if (tPool.at(i) == managedThreadId)
        {
            tPool[i] = 0;
            return S_OK;
        }
    }
    return S_OK;
}

//insert every appdomain into appdomainpool
HRESULT GetAppdomains::AppDomainCreationFinished(IPrfCom * pPrfCom, AppDomainID appDomainId,HRESULT     hrStatus)
{
    if (FAILED(hrStatus))return S_OK;
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    m_ulAppdomains++;
    DISPLAY(L"AppDomain "<<HEX(appDomainId)<<L" Created.");
    adPool.push_back(appDomainId);
    dwEventValues[APPDOMAINCREATED] = 1;

    hEvent.Signal();
    HRESULT hr = S_OK;
    for(ULONG i=0;i<adPool.size();++i)
    {
        if (adPool.at(i) == appDomainId)
        {
            return S_OK;
        }
    }
    return hr;    
    
}
HRESULT GetAppdomains::AppDomainShutdownStarted(IPrfCom * pPrfCom, AppDomainID appDomainId)
{
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    for(ULONG i=0;i<adPool.size();++i)
    {
        if (adPool.at(i) == appDomainId)
        {
            adPool[i] = 0;
            return S_OK;
        }
    }
    return S_OK;
}

HRESULT GetAppdomains::ThreadNameChanged(IPrfCom * pPrfCom, ThreadID threadId, ULONG cchName, PROFILER_WCHAR name[])
{
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
	PLATFORM_WCHAR nameTemp[STRING_LENGTH];

	ConvertProfilerWCharToPlatformWChar(nameTemp, STRING_LENGTH, name, STRING_LENGTH);
    PLATFORM_WCHAR threadName[]= L"myThreadStaticTestThread";
    if (wcsncmp(nameTemp, threadName, cchName)==0)   
    {
        assert(cchName == wcslen(threadName));
    }
    return S_OK;
}

HRESULT GetAppdomains::ClassLoadFinished(IPrfCom * pPrfCom,
                                    ClassID classId,
                                    HRESULT hrStatus)
{
    if (FAILED(hrStatus)) return S_OK;
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    mdTypeDef token = NULL;
    ModuleID classModuleId = NULL;
    COMPtrHolder<IMetaDataImport> pIMDImport;
    HRESULT hr = S_OK;
    MUST_PASS(PINFO->GetClassIDInfo2(classId, &classModuleId, &token, NULL, NULL, NULL, NULL));
    MUST_PASS(PINFO->GetModuleMetaData(classModuleId, ofRead, IID_IMetaDataImport, (IUnknown **)&pIMDImport));
    
    PROFILER_WCHAR tokenNameTemp[STRING_LENGTH];
    PLATFORM_WCHAR tokenName[STRING_LENGTH];
    MUST_PASS(pIMDImport->GetTypeDefProps(token, tokenNameTemp, STRING_LENGTH, NULL, NULL, NULL));

	ConvertProfilerWCharToPlatformWChar(tokenName, STRING_LENGTH, tokenNameTemp, STRING_LENGTH);
    DISPLAY(L"Class "<< tokenName << L" LoadFinished");
    PLATFORM_WCHAR className[]= L"MyStaticClass";
    if (wcsncmp(tokenName, className, wcslen(className))==0)   
    {
        classPool.push_back(classId);
		dwEventValues[CLASSLOADED] = 1;
        hEvent.Signal();
    }
    
    return S_OK;
} 
HRESULT GetAppdomains::ClassUnloadStarted(IPrfCom * pPrfCom, ClassID classId)
{
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    for(ULONG i=0;i<classPool.size();++i)
    {
        if (classPool.at(i) == classId)
        {
            classPool[i] = 0;
            return S_OK;
        }
    }
    return S_OK;
}


DWORD WINAPI GetAppDomainsAsynchronousThreadFunc(LPVOID pParam)
{
    GetAppdomains::Instance()->AsynchThreadFunc(pParam);
    dwEventValues[SAMPLINGTHREAD] = 1;
    samplingThreadEvent.Signal();
    return 0;
}

void TimerThread()
{
	std::this_thread::sleep_for(std::chrono::nanoseconds(100));
    dwEventValues[TIMER] = 1;
    hEvent.Signal();
}


DWORD WINAPI GetAppdomains::AsynchThreadFunc(LPVOID pParam)
{
    IPrfCom * pPrfCom = static_cast<IPrfCom *>(pParam);
    HRESULT hr=S_OK;    
    while (TRUE)
    {
        DWORD dwEvent = 0;
        hEvent.Wait();

        if (pPrfCom->m_Shutdown > 0)
            break;


        ULONG32 cClassIds = (ULONG)classPool.size();
        ULONG32 cAppDomainIds = 0;
        ULONG count = 0;
        DISPLAY(cClassIds << L"classids .");
        for (ULONG cls = 0;cls<cClassIds;++cls)
        {
            ClassID classId = classPool.at(cls);
            if (classId == 0) continue;
            ModuleID moduleId = NULL;
            mdTypeDef token = NULL;
            WCHAR tokenName[256];
            HCORENUM hEnum  = NULL;    
            mdFieldDef fieldTokens[STRING_LENGTH];
            ULONG cTokens   = NULL;
            ULONG nameLength = NULL;
        
            ModuleID classModuleId = NULL;
            COMPtrHolder<IMetaDataImport> pIMDImport;

        
            DWORD corElementType = NULL;
            PCCOR_SIGNATURE pvSig = NULL;
            ULONG cbSig = NULL;
            DWORD fieldAttributes = NULL;


            MUST_PASS(hr = PINFO->GetClassIDInfo2(classId, &moduleId, &token, NULL, NULL, NULL, NULL));
            MUST_PASS(PINFO->GetModuleMetaData(moduleId, ofRead, IID_IMetaDataImport, (IUnknown **)&pIMDImport));
            MUST_PASS(pIMDImport->GetTypeDefProps(token, tokenName, 256, NULL, NULL, NULL));
            MUST_PASS(pIMDImport->EnumFields(&hEnum, token, fieldTokens, STRING_LENGTH, &cTokens));
                        
            ULONG32 assumeCount=5+1;
            AppDomainID *appDomainIds = (AppDomainID *)malloc(assumeCount*sizeof(AppDomainID));
            if (appDomainIds==NULL) GetAppdomains_FAILURE("Out Of Memory");
            memset(appDomainIds, 0, assumeCount*sizeof(AppDomainID));
            hr = PINFO->GetAppDomainsContainingModule(moduleId, assumeCount,&cAppDomainIds, appDomainIds);
            if (FAILED(hr))
                  GetAppdomains_FAILURE(cAppDomainIds << L" Appdomains returned, hr = " << HEX(hr));
            if (assumeCount<cAppDomainIds)
            {
                assumeCount=cAppDomainIds+1;
                appDomainIds = (AppDomainID *)realloc(appDomainIds, assumeCount*sizeof(AppDomainID));
                if (appDomainIds==NULL) GetAppdomains_FAILURE("Out Of Memory");
                memset(appDomainIds, 0, assumeCount*sizeof(AppDomainID));
                hr = PINFO->GetAppDomainsContainingModule(moduleId, assumeCount,&cAppDomainIds, appDomainIds);
                if (FAILED(hr))
                  GetAppdomains_FAILURE(cAppDomainIds << L" Appdomains returned, hr = " << HEX(hr));
            }
            if (appDomainIds[cAppDomainIds]!=0)
                    GetAppdomains_FAILURE(L"appDomainIds["<<cAppDomainIds<<L"] = " <<appDomainIds[cAppDomainIds] << L" while zero expected.");
            PVOID staticAddress=0;

            for (ULONG tk = 0; tk < cTokens; ++tk)
            {
                ClassID fieldClassId = NULL;
                mdTypeDef fieldClassToken;
                MUST_PASS(pIMDImport->GetFieldProps(fieldTokens[tk],    // The field for which to get props.
                                                &fieldClassToken,  // Put field's class here.
                                                tokenName,         // Put field's name here.
                                                256,               // Size of szField buffer in wide chars.
                                                &nameLength,       // Put actual size here
                                                &fieldAttributes,  // Put flags here.
                                                &pvSig,            // [OUT] point to the blob value of meta data
                                                &cbSig,            // [OUT] actual size of signature blob
                                                &corElementType,   // [OUT] flag for value type. selected ELEMENT_TYPE_*
                                                NULL,              // [OUT] constant value
                                                NULL));            // [OUT] size of constant value, string only, wide chars

                if(IsFdStatic(fieldAttributes))
                {
                    if (S_OK == pIMDImport->GetCustomAttributeByName(fieldTokens[tk], PROFILER_STRING("System.ThreadStaticAttribute"), NULL, NULL))
                    {
                        ULONG valid=0;
                        ULONG valid2=0;
                        for(ULONG i=0;i<tPool.size();++i)
                        {
                            //The static can be found in number(valid) of  thread+appdomain pairs
                            for (ULONG ad = 0;ad<adPool.size();++ad)
                            {
                        
                                if (tPool.at(i) == 0) continue;
                                HRESULT hr = PINFO->GetThreadStaticAddress2(classId, fieldTokens[tk], adPool.at(ad), tPool.at(i), &staticAddress);
                                if (SUCCEEDED(hr))
                                {
									DISPLAY( L"1: "<< valid <<" ClassId " << classId << L"is found in ad " << adPool.at(ad) <<L" + thread " <<tPool.at(i) );
									DISPLAY( L"staticAddress :" << staticAddress);
									valid++;
                                }
                            }

                            //The static can be found in number(valid2) of  thread+appdomain pairs
                            for (ULONG ad = 0;ad<cAppDomainIds;++ad)
                            {
                                if (tPool.at(i) == 0) continue;
								HRESULT hr = PINFO->GetThreadStaticAddress2(classId, fieldTokens[tk], appDomainIds[ad], tPool.at(i), &staticAddress);
                                //DISPLAY(L"GetThreadStaticAddress2 " << HEX(hr) << "  appDomainIds " << appDomainIds[ad] << L" tPool.at(i) " << tPool.at(i));
                                if (SUCCEEDED(hr))
                                {
									DISPLAY( L"2: "<< valid2 <<" ClassId " << classId << L"is found in ad " << appDomainIds[ad] <<L" + thread " <<tPool.at(i) );
									DISPLAY( L"staticAddress :" << staticAddress);
                                    valid2++;
                                }
                            }
                        }
                        if (valid2<valid )
                        {
                            //This check is disabled due to Dev10 won't fix bug 821840.
                            //GetAppdomains_FAILURE(L"ClassId " << classId << " is not found in enough appdomains, " << valid << " expected while " <<valid2 << " in fact." );
                        }
                    }
                    else if (S_OK == pIMDImport->GetCustomAttributeByName(fieldTokens[tk], PROFILER_STRING("System.ContextStaticAttribute"), NULL, NULL))
                    {
                    }
                    else if (IsFdHasFieldRVA(fieldAttributes))
                    {
                    }
                    else //Appdomain static 
                    {  

                        //For every appdomain returned by the API, GetAppDomainStaticAddress works unless the appdomain is NOT initialized yet.
                        for (ULONG ad = 0;ad<cAppDomainIds;++ad)
                        {
                            HRESULT hr = PINFO->GetAppDomainStaticAddress(classId, fieldTokens[tk], appDomainIds[ad], &staticAddress);
                            if (FAILED(hr))
                            {
                                if (CORPROF_E_DATAINCOMPLETE == hr)m_ulDataIncomplete++;
								else GetAppdomains_FAILURE(L"GetAppDomainStaticAddress Failed: "<< hr);
                            }
                            else
                            {
                                PULONG ptr =  static_cast<PULONG>(staticAddress);  
                                if (*ptr != 0xDEF) GetAppdomains_FAILURE(L"static value is NOT 0xDEF,  but "<< *ptr);;
                                DISPLAY(L"Appdomain Static " << HEX(staticAddress) << " value " << HEX(*ptr));
                            }
                        }
                        
                        //This moment, all appdomains should be initialized
						if (dwEventValues[READYTOVERIFY])
                        {   
                            //every class is initialized into a appdomain
                            count = 0;
                            for(ULONG i=0;i<adPool.size();++i)
                            {
                                if (adPool.at(i) != 0)
                                {
                                    //For domain neutral modules, the GetAppDomainStaticAddress returns correct value only if the appdomainId 
                                    // is correct; For none domain neutral modules, the adddomainId is meaningless here, what ever you pass in,
                                    // as long as it is a valid appdomainId, the API will give correct return values.
                                    HRESULT hra = PINFO->GetAppDomainStaticAddress(classId, fieldTokens[tk], adPool.at(i), &staticAddress);
                                    if (SUCCEEDED(hra)) 
                                    {
                                        count++;
                                        ULONG adi=0;
                                        for (adi = 0;adi<cAppDomainIds;++adi)
                                        {
                                            if (appDomainIds[adi] ==  adPool.at(i) ) break;
                                        }
                                        //if appdomainId adPool.at(i) is not included in appDomainIds
                                        if (adi == cAppDomainIds)
                                            GetAppdomains_FAILURE(L"AppdomainId "<<adPool.at(i)<<" is not included in appDomainIds.");
                                    }
                                }
                            }//for all appdomains of the process
                       
                            if (count!=cAppDomainIds)
                                GetAppdomains_FAILURE(count << L"Appdomains other than " << cAppDomainIds << " containing module.");
                            //terminate thread    
                            return 0;
                        }
                    }//if appdomain static
                }//if static field
            }//for tokens
            free(appDomainIds);
        }//for classids
		if (dwEventValues[READYTOVERIFY])
        {   
            //terminate thread
            //should not calls into here.
            break;
        }
    }//while true

    return 0;
}

GetAppdomains::GetAppdomains(IPrfCom * pPrfCom)
{
    // Initialize result counters
    GetAppdomains::This = this;
    m_ulSuccess = 0;
    m_ulFailure = 0;
    m_ulAppdomains=0;
    m_ulLiterals=0;
    m_ulErrorArgument=0;
    m_ulGetAppdomainStaticAddress=0;
    m_ulGetThreadStaticAddress2=0;
    m_ulGetThreadStaticAddress=0;
    m_ulDataIncomplete=0;
    m_ulGetContextStaticAddress=0;
    m_ulGetRVAStaticAddress=0;
    m_ulNewThreadInit=0;
    m_ulSOKWithZeroValue=0;

    m_pPrfCom = pPrfCom;
	try
	{
		std::thread t1(GetAppDomainsAsynchronousThreadFunc, pPrfCom);
		t1.detach();
	}
	catch (system_error) 
	{
		GetAppdomains_FAILURE(L"Failing to create thread.");
	}

    
    // Sample timeout Negative value makes this relative rather than actual time.  We use the smallest
    // interval possible here, 1 = 100 nanosecond interval.
	try
	{
		std::thread timerThread(TimerThread);
		timerThread.detach();
	}
	catch (system_error)
	{
		GetAppdomains_FAILURE(L"Failing to set timer.");
	}

}



/*
 *  Clean up
 */
GetAppdomains::~GetAppdomains()
{
    GetAppdomains::This = NULL;
    //ToDo: Kill Unmanaged Thread
}


HRESULT GetAppdomains::Verify(IPrfCom * pPrfCom)
{
    dwEventValues[READYTOVERIFY] = 1;
    hEvent.Signal();
    samplingThreadEvent.Wait();

    DISPLAY(L"\nm_ulErrorArgument = "<<m_ulErrorArgument <<
            L"\nm_ulDataIncomplete = "<<m_ulDataIncomplete <<
            L"\nm_ulGetAppdomainStaticAddress = "<<m_ulGetAppdomainStaticAddress <<
            L"\nm_ulGetThreadStaticAddress = "<<m_ulGetThreadStaticAddress <<
            L"\nm_ulGetThreadStaticAddress2 = "<<m_ulGetThreadStaticAddress2 <<
            L"\nm_ulGetContextStaticAddress = "<<m_ulGetContextStaticAddress <<
            L"\nm_ulGetRVAStaticAddress = "<<m_ulGetRVAStaticAddress <<
            L"\nm_ulNewThreadInit = "<<m_ulNewThreadInit <<
            L"\nm_ulSOKWithZeroValue = "<<m_ulSOKWithZeroValue <<
            L"\nm_ulFailure = "<<m_ulFailure <<
            L"\nm_ulSuccess = "<<m_ulSuccess 
            );
            

    if (m_ulFailure > 0)// || m_ulSuccess <=0)
    {
        GetAppdomains_FAILURE(L"Failing test due to ERRORS above.");
        return E_FAIL;
    }

    return S_OK;
}

/*
 *  Verification routine called by TestProfiler
 */
HRESULT GetAppdomains_Verify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(GetAppdomains);
    HRESULT hr = pGetAppdomains->Verify(pPrfCom);

    // Clean up instance of test class
    delete pGetAppdomains;
    pPrfCom->m_pTestClassInstance = NULL;

    return hr;
}


/*
 *  Initialization routine called by TestProfiler
 */
void GetAppdomains_Initialize(ILTSCom * pLTSCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    IPrfCom * pPrfCom = dynamic_cast<IPrfCom *>(pLTSCom);
    DISPLAY(L"Initialize GetAppdomains.\n")

    // Create and save an instance of test class
    SET_CLASS_POINTER(new GetAppdomains(pPrfCom));

    // Initialize MethodTable
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_THREADS|
                                COR_PRF_MONITOR_APPDOMAIN_LOADS|
                                COR_PRF_MONITOR_CLASS_LOADS;

    REGISTER_CALLBACK(THREADCREATED,             GetAppdomains::ThreadCreatedWrapper);
    REGISTER_CALLBACK(THREADDESTROYED,           GetAppdomains::ThreadDestroyedWrapper);
    REGISTER_CALLBACK(THREADNAMECHANGED,         GetAppdomains::ThreadNameChangedWrapper);
    REGISTER_CALLBACK(APPDOMAINCREATIONFINISHED, GetAppdomains::AppDomainCreationFinishedWrapper);
    REGISTER_CALLBACK(APPDOMAINSHUTDOWNSTARTED,  GetAppdomains::AppDomainShutdownStartedWrapper);
    REGISTER_CALLBACK(CLASSLOADFINISHED,         GetAppdomains::ClassLoadFinishedWrapper);
    REGISTER_CALLBACK(CLASSUNLOADSTARTED,        GetAppdomains::ClassUnloadStartedWrapper);
    REGISTER_CALLBACK(VERIFY,                    GetAppdomains::VerifyWrapper);

    return;
}
