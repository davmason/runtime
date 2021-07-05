#include "ProfilerCommon.h"
#include "Asynchronous.h"


/*
 *  Initialize the CAsynchronousFuncTest class.  Set the initial state of the counters and initialize
 *  the synchronization objects.
 */
CAsynchronousFuncTest::CAsynchronousFuncTest(IPrfCom * /*pPrfCom*/, AsynchronousTestFlags testFunc)
{
    // Which function are we testing?
    m_currentFunction = testFunc;

    // Initialize result counters
    m_ulSuccess = 0;
    m_ulFailure = 0;
}


HRESULT CAsynchronousFuncTest::AppDomainCreated(IPrfCom * pPrfCom, AppDomainID appDomainId, HRESULT result)
{
    PROFILER_WCHAR appDomainNameTemp[STRING_LENGTH];
    PLATFORM_WCHAR appDomainName[STRING_LENGTH];
    ULONG nameLength = 0;
    ProcessID procId = 0;


    if (result == S_OK)
    {
        MUST_PASS(PINFO->GetAppDomainInfo(appDomainId,
                                          STRING_LENGTH,
                                          &nameLength,
                                          appDomainNameTemp,
                                          &procId));

        ConvertProfilerWCharToPlatformWChar(appDomainName, STRING_LENGTH, appDomainNameTemp, STRING_LENGTH);

        {
    	    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

            for (UINT i = 0; i < m_appDomainStack.size(); i++)
            {
                // On coreclr, corerun.exe causes a second AppDomainCreation* event for the DefaultDomain
                if ((appDomainId == m_appDomainStack[i]) && (wcscmp(appDomainName, L"corerun.exe") != 0))
                {
                    FAILURE(L"ERROR: Second AppDomainCreated callback received for AppDomainID " << HEX(appDomainId));
                    return E_FAIL;
                }
            }

            m_appDomainStack.push_back(appDomainId);
        }
    }
    return S_OK;
}


HRESULT CAsynchronousFuncTest::AppDomainShutdown(IPrfCom * pPrfCom, AppDomainID appDomainId, HRESULT result)
{
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    if (result == S_OK)
    {
        AppDomainStack::iterator iADS = find(m_appDomainStack.begin(), m_appDomainStack.end(), appDomainId);
        if (iADS != m_appDomainStack.end())
        {
            m_appDomainStack.erase(iADS);
        }
    }

    return S_OK;
}

HRESULT CAsynchronousFuncTest::ThreadCreated(IPrfCom *pPrfCom, ThreadID managedThreadId)
{
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

    // Quick Sanity check on the parameters
    if (managedThreadId == 0)
    {
        FAILURE(L"A NULL parameter was passed to ThreadCreated ThreadID = " << HEX(managedThreadId));
        return E_FAIL;
    }

    AppDomainID threadAppDomain = 0;
    MUST_PASS(PINFO->GetThreadAppDomain(managedThreadId, &threadAppDomain));

    ThreadAppDomainMap::iterator iTADM = m_threadAppDomainMap.find(managedThreadId);

    // if thread already exists, we have a problem.  why are we seeing this thread created again?
    if (iTADM != m_threadAppDomainMap.end())
    {
        FAILURE(L"ERROR: Second ThreadCreated callback received for ThreadID " << HEX(managedThreadId));
        return E_FAIL;
    }

    // Add the thread to our Map to sample later
    m_threadAppDomainMap.insert(make_pair(managedThreadId, threadAppDomain));

    return S_OK;
}


HRESULT CAsynchronousFuncTest::Context_ThreadCreated(IPrfCom * pPrfCom, ThreadID managedThreadId)
{
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

    // Quick Sanity check on the parameters
    if (managedThreadId == 0)
    {
        FAILURE(L"A NULL parameter was passed to Context_ThreadCreated ThreadID = " << HEX(managedThreadId));
        return E_FAIL;
    }

    ContextID threadContext = 0;
    MUST_PASS(PINFO->GetThreadContext(managedThreadId, &threadContext));


    ThreadContextMap::iterator iTCM = m_threadContextMap.find(managedThreadId);

    // if thread already exists, we have a problem.  why are we seeing this thread created again?
    if (iTCM != m_threadContextMap.end())
    {
        FAILURE(L"ERROR: Second Context_ThreadCreated callback received for ThreadID " << HEX(managedThreadId));
        return E_FAIL;
    }

    // Add the thread to our Map to sample later
    m_threadContextMap.insert(make_pair(managedThreadId, threadContext));

    return S_OK;
}


HRESULT CAsynchronousFuncTest::ThreadDestroyed(IPrfCom * pPrfCom, ThreadID managedThreadId)
{
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    // Quick Sanity check on the parameters
    if (managedThreadId == 0)
    {
        FAILURE(L"A NULL parameter was passed to ThreadDestroyed ThreadID = " << HEX(managedThreadId));
        return E_FAIL;
    }

    ThreadAppDomainMap::iterator iTADM = m_threadAppDomainMap.find(managedThreadId);

    if (iTADM == m_threadAppDomainMap.end())
    {
        FAILURE(L"ERROR: Second ThreadDestroyed callback received for ThreadID " << HEX(managedThreadId));
        return E_FAIL;
    }
    m_threadAppDomainMap.erase(managedThreadId);

    return S_OK;
}


HRESULT CAsynchronousFuncTest::Context_ThreadDestroyed(IPrfCom * pPrfCom, ThreadID managedThreadId)
{
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    
    // Quick Sanity check on the parameters
    if (managedThreadId == 0)
    {
        FAILURE(L"A NULL parameter was passed to Context_ThreadDestroyed ThreadID = " << HEX(managedThreadId));
        return E_FAIL;
    }

    ThreadContextMap::iterator iTCM= m_threadContextMap.find(managedThreadId);

    if (iTCM == m_threadContextMap.end())
    {
        FAILURE(L"ERROR: Second Context_ThreadDestroyed callback received for ThreadID " << HEX(managedThreadId));
        return E_FAIL;
    }

    m_threadContextMap.erase(managedThreadId);

    return S_OK;
}


HRESULT CAsynchronousFuncTest::GetCodeInfo2_FunctionEnter3WithInfo(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID)
{
    COR_PRF_CODE_INFO codeInfo[1000];
    ULONG32 len = NULL;
    HRESULT hr  = S_OK;

    MUST_PASS(PINFO->GetCodeInfo2(functionIDOrClientID.functionID, 1000, &len, codeInfo));

    for(ULONG32 i = 0; i < len; i++)
    {
        if (codeInfo[i].startAddress == 0)
        {
            FAILURE(L"CodeInfo not as expected for function: " << HEX(functionIDOrClientID.functionID));
            m_ulFailure++;
            hr = E_FAIL;
        }

        if (codeInfo[i].size == 0)
        {
            FAILURE(L"CodeInfo size not as expected for function: " << HEX(functionIDOrClientID.functionID));
            m_ulFailure++;
            hr = E_FAIL;
        }

        if (hr != E_FAIL)
        {
            m_ulSuccess++;
        }
    }

    return hr;
}


HRESULT CAsynchronousFuncTest::GetFunctionFromIP_FunctionEnter3WithInfo(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID)
{
	COR_PRF_CODE_INFO codeInfos[100];
	ULONG32 len = NULL;

	HRESULT hr= S_OK;
	hr = PINFO->GetCodeInfo2(functionIDOrClientID.functionID, 100, &len, codeInfos);

	if( FAILED(hr) ) 
	{
		return hr;
	}

	for (ULONG32 i = 0; i < len; i++)
	{
		FunctionID functionId;

		MUST_PASS(PINFO->GetFunctionFromIP((LPCBYTE)(codeInfos[i].startAddress), &functionId));

		if (functionId != functionIDOrClientID.functionID)
		{
				FAILURE(L"FunctionInfo not as expected for function: " << HEX(functionIDOrClientID.functionID));
				m_ulFailure++;
		}
        else
        {
    		m_ulSuccess++;
        }
	}

	return S_OK;
}


HRESULT CAsynchronousFuncTest::GetFunctionInfo2_FunctionEnter3WithInfo(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID, COR_PRF_FRAME_INFO func)
{
    // TODO: this test does not work, it attempts to cache COR_PRF_FRAME_INFOs, which is illegal. 
    // COR_PRF_FRAME_INFOs point to stack frames, which are invalidated when functions exit
    return E_FAIL;

    FunctionInfo2Map::iterator iFIM;
    FunctionInfo2Struct * functionInfo2 = NULL;

    bool     found    = false;
    ClassID  classId  = NULL;
    ModuleID moduleId = NULL;
    mdToken  mdtok    = NULL;
    ULONG32  numArgs  = NULL;
    ClassID  argTypes[1000];    

    for (iFIM  = m_FunctionInfo2Map.begin();
         iFIM != m_FunctionInfo2Map.end();
         iFIM++)
    {
        if (functionIDOrClientID.functionID == iFIM->first)
        {
            found = true;
        }

        functionInfo2 = iFIM->second;
        if (functionInfo2->classUnloaded == false)
        {
            HRESULT hr = PINFO->GetFunctionInfo2(iFIM->first, iFIM->second->func, &classId, &moduleId, &mdtok, 1000, &numArgs, argTypes);

            if (functionInfo2->classId != classId)
            {
                FAILURE(L"FunctionInfo class Id not as expected for function: " << HEX(iFIM->first));
                m_ulFailure++;
            }
            else if (functionInfo2->moduleId != moduleId)
            {
                FAILURE(L"FunctionInfo module Id not as expected for function: " << HEX(iFIM->first));
                m_ulFailure++;
            }
            else if (functionInfo2->mdtoken != mdtok)
            {
                FAILURE(L"FunctionInfo md Token not as expected for function: " << HEX(iFIM->first));
                m_ulFailure++;
            }
            else if (functionInfo2->numArgs != numArgs)
            {
                FAILURE(L"FunctionInfo num args not as expected for function: " << HEX(iFIM->first));
                m_ulFailure++;
            }
            else 
            {
                m_ulSuccess++;
            }
        }
    }

    if (!found)
    {
        functionInfo2 = new FunctionInfo2Struct();

        // TODO: fix
        HRESULT hr = PINFO->GetFunctionInfo2(functionIDOrClientID.functionID, func, &classId, &moduleId, &mdtok, 1000, &numArgs, argTypes);

        functionInfo2->func          = func;
        functionInfo2->classId       = classId;
        functionInfo2->moduleId      = moduleId;
        functionInfo2->mdtoken       = mdtok;
        functionInfo2->numArgs       = numArgs;
        functionInfo2->classUnloaded = false;

        {
            lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
            m_FunctionInfo2Map.insert(make_pair(functionIDOrClientID.functionID, functionInfo2));
        }

        m_ulSuccess++;
    }
    
    return S_OK;
}


HRESULT CAsynchronousFuncTest::GetFunctionInfo_FunctionEnter3WithInfo(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID)
{
    FunctionInfoMap::iterator iFIM;
    FunctionInfoStruct * functionInfo = NULL;
    
    ClassID  classId  = NULL;
    ModuleID moduleId = NULL;
    mdToken  mdtok    = NULL;
    bool     found    = false;

    for (iFIM  = m_FunctionInfoMap.begin();
         iFIM != m_FunctionInfoMap.end();
         iFIM++)
    {
        if (functionIDOrClientID.functionID == iFIM->first) 
        {
            found = true;
        }

        functionInfo = iFIM->second;
        if (functionInfo->classUnloaded == false)
        {
            MUST_PASS(PINFO->GetFunctionInfo(iFIM->first, &classId, &moduleId, &mdtok));
            
            if (functionInfo->classId != classId)
            {
                FAILURE(L"FunctionInfo not as expected for function: " << HEX(iFIM->first));
                m_ulFailure++;
            }
            else if (functionInfo->moduleId != moduleId)
            {
                FAILURE(L"FunctionInfo size not as expected for function: " << HEX(iFIM->first));
                m_ulFailure++;
            }
            else if (functionInfo->mdtoken != mdtok)
            {
                FAILURE(L"FunctionInfo size not as expected for function: " << HEX(iFIM->first));
                m_ulFailure++;
            }
            else 
            {
                m_ulSuccess++;
            }
        }
    }

    if (!found)
    {
        functionInfo = new FunctionInfoStruct();
        
        MUST_PASS(PINFO->GetFunctionInfo(functionIDOrClientID.functionID, &classId, &moduleId, &mdtok))

        functionInfo->classId       = classId;
        functionInfo->moduleId      = moduleId;
        functionInfo->mdtoken       = mdtok;
        functionInfo->classUnloaded = false;
        
        {
            lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
            m_FunctionInfoMap.insert(make_pair(functionIDOrClientID.functionID, functionInfo));
        }

        m_ulSuccess++;
    }
    
    return S_OK;

}


HRESULT CAsynchronousFuncTest::GetThreadAppDomain_FunctionEnter3WithInfo(IPrfCom * pPrfCom)
{
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    
    ThreadAppDomainMap::iterator iTADM;
    AppDomainID appDomain = NULL;

    for (iTADM  = m_threadAppDomainMap.begin();
         iTADM != m_threadAppDomainMap.end();
         iTADM++)
    {
        ThreadID thread = iTADM->first;
        appDomain = NULL;

        MUST_PASS(PINFO->GetThreadAppDomain(thread, &appDomain));

        PROFILER_WCHAR wszAppDomain[STRING_LENGTH];
        memset(wszAppDomain, 1, STRING_LENGTH * sizeof(PROFILER_WCHAR));
        ULONG cchSize;
        ProcessID procID;
        MUST_PASS(PINFO->GetAppDomainInfo(appDomain, STRING_LENGTH, &cchSize, wszAppDomain, &procID));

        if (!IsNullTerminated(wszAppDomain, STRING_LENGTH))
        {
            m_ulFailure++;
            FAILURE(L"Malformed AppDomain name");
            return E_FAIL;
        }
        else
        {
            m_ulSuccess++;
        }
    }

    return S_OK;
}


HRESULT CAsynchronousFuncTest::GetThreadContext_FunctionEnter3WithInfo(IPrfCom * pPrfCom)
{
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    ThreadContextMap::iterator iTCM;

    for (iTCM  = m_threadContextMap.begin();
         iTCM != m_threadContextMap.end();
         iTCM++)
    {
        ThreadID thread = iTCM->first;
        ContextID context;

        MUST_PASS(PINFO->GetThreadContext(thread, &context));
    }

    m_ulSuccess++;
    return S_OK;
}


HRESULT CAsynchronousFuncTest::ClassUnloadStarted(IPrfCom * pPrfCom, ClassID  classId)
{
    DISPLAY(L"ClassUnloadStarted - " << HEX(classId));

    if (m_currentFunction == GET_FUNCTION_INFO_2)
    {
        FunctionInfo2Map::iterator iFIM;

        {
            lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

            for (iFIM = m_FunctionInfo2Map.begin();
                 iFIM != m_FunctionInfo2Map.end();
                 iFIM++)
            {
                FunctionInfo2Struct *functionInfo = iFIM->second;
                if (functionInfo->classId == classId)
                {
                    functionInfo->classUnloaded = true;
                }
            }
        }
    }
    if (m_currentFunction == GET_FUNCTION_INFO)
    {
        FunctionInfoMap::iterator iFIM;

        {
            lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
            
            for (iFIM = m_FunctionInfoMap.begin();
                 iFIM != m_FunctionInfoMap.end();
                 iFIM++)
            {
                FunctionInfoStruct *functionInfo = iFIM->second;
                if (functionInfo->classId == classId)
                {
                    functionInfo->classUnloaded = true;
                }
            }
        }
    }

    return S_OK;
}


/*
 *  Verify the results of this run.
 */
HRESULT CAsynchronousFuncTest::Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"ClassUnloads - " << PPRFCOM->m_ClassUnloadStarted);

    if ((m_ulFailure == 0) && (m_ulSuccess > 0))
    {
        return S_OK;
    }
    else
    {
        FAILURE(L"Either some checks failed, or no successful checks were completed.");
        return E_FAIL;
    }
}


/*
 *  Verification routine called by TestProfiler
 */
HRESULT CAsynchronousFuncTest_Verify(IPrfCom *pPrfCom)
{
    LOCAL_CLASS_POINTER(CAsynchronousFuncTest);
    HRESULT hr = pCAsynchronousFuncTest->Verify(pPrfCom);

    // Clean up instance of test class
    delete pCAsynchronousFuncTest;
    pPrfCom->m_pTestClassInstance = NULL;

    return hr;
}


/*
 *  Initialization routine called by TestProfiler
 */
void CAsynchronousFuncTest_Initialize (IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, AsynchronousTestFlags testFunc)
{
    // Create instance of test class
    SET_CLASS_POINTER(new CAsynchronousFuncTest(pPrfCom, testFunc));

	// Set callback flags and functions
	pModuleMethodTable->FLAGS = COR_PRF_MONITOR_ENTERLEAVE      | 
                                COR_PRF_USE_PROFILE_IMAGES      |   
                                COR_PRF_MONITOR_CLASS_LOADS     |
                                COR_PRF_ENABLE_FUNCTION_ARGS    | 
                                COR_PRF_ENABLE_FUNCTION_RETVAL  | 
                                COR_PRF_ENABLE_FRAME_INFO;

	if (testFunc == GET_THREAD_APPDOMAIN) pModuleMethodTable->FLAGS |= COR_PRF_MONITOR_THREADS | COR_PRF_MONITOR_APPDOMAIN_LOADS;
	if (testFunc == GET_THREAD_CONTEXT)   pModuleMethodTable->FLAGS |= COR_PRF_MONITOR_THREADS;
	
	REGISTER_CALLBACK(FUNCTIONENTER3WITHINFO, CAsynchronousFuncTest::FunctionEnter3WithInfoWrapper);
	REGISTER_CALLBACK(CLASSUNLOADSTARTED,        CAsynchronousFuncTest::ClassUnloadStartedWrapper);
	REGISTER_CALLBACK(THREADCREATED,             CAsynchronousFuncTest::ThreadCreatedWrapper);
	REGISTER_CALLBACK(THREADDESTROYED,           CAsynchronousFuncTest::ThreadDestroyedWrapper);
	REGISTER_CALLBACK(APPDOMAINCREATIONFINISHED, CAsynchronousFuncTest::AppDomainCreatedWrapper);
	REGISTER_CALLBACK(APPDOMAINSHUTDOWNFINISHED, CAsynchronousFuncTest::AppDomainShutdownWrapper);
	REGISTER_CALLBACK(VERIFY, CAsynchronousFuncTest_Verify);

	return;
}

