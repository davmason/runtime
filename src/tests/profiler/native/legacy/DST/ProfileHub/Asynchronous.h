#include "../../ProfilerCommon.h"

///////////////////////////////////////////////////////////////////////////////

// Structures used to keep info on ID used through the test
#pragma region test_structs

    typedef enum _AsynchronousTestFlags
    {
        TEST_NONE            = 0x00,
        GET_CODE_INFO_2      = 0x02,
        GET_FUNCTION_FROM_IP = 0x04,
        GET_FUNCTION_INFO    = 0x08,
        GET_FUNCTION_INFO_2  = 0x10,
        GET_THREAD_APPDOMAIN = 0x20,
        GET_THREAD_CONTEXT   = 0x40,
        TEST_ALL             = 0x80,
    } AsynchronousTestFlags;

    typedef map<FunctionID, COR_PRF_CODE_INFO *> CodeInfo2Map;

    typedef struct _FunctionInfo2Struct
    {
        ClassID classId;
	    ModuleID moduleId;
	    mdToken mdtoken;
	    COR_PRF_FRAME_INFO func;
	    ULONG32 numArgs;
	    ClassID* argTypes;
        bool classUnloaded;
    } FunctionInfo2Struct;

    typedef map<FunctionID, FunctionInfo2Struct *> FunctionInfo2Map;

    typedef struct _FunctionInfoStruct
    {
        ClassID classId;
	    ModuleID moduleId;
	    mdToken mdtoken;
        bool classUnloaded;
    } FunctionInfoStruct;

    typedef map<FunctionID, FunctionInfoStruct *> FunctionInfoMap;

    typedef map<ThreadID, AppDomainID> ThreadAppDomainMap;
    typedef vector<AppDomainID> AppDomainStack;

    typedef map<ThreadID, ContextID> ThreadContextMap;

#pragma endregion


///////////////////////////////////////////////////////////////////////////////


class CAsynchronousFuncTest
{
public:

    CAsynchronousFuncTest(IPrfCom *pPrfCom, AsynchronousTestFlags testFunc);
    
    HRESULT Verify(IPrfCom * pPrfCom);

	HRESULT GetCodeInfo2_FunctionEnter3WithInfo(IPrfCom *pPrfCom, FunctionIDOrClientID functionIDOrClientID);
	HRESULT GetFunctionFromIP_FunctionEnter3WithInfo(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID);
	HRESULT GetFunctionInfo_FunctionEnter3WithInfo(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID);
	HRESULT GetFunctionInfo2_FunctionEnter3WithInfo(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID, COR_PRF_FRAME_INFO func);
	HRESULT GetThreadAppDomain_FunctionEnter3WithInfo(IPrfCom * pPrfCom);
	HRESULT GetThreadContext_FunctionEnter3WithInfo(IPrfCom * pPrfCom);

	HRESULT ThreadCreated(IPrfCom * pPrfCom, ThreadID managedThreadId);
    HRESULT ThreadDestroyed(IPrfCom * pPrfCom, ThreadID managedThreadId);

	HRESULT Context_ThreadCreated(IPrfCom * pPrfCom, ThreadID managedThreadId);
    HRESULT Context_ThreadDestroyed(IPrfCom * pPrfCom, ThreadID managedThreadId);
    
    HRESULT AppDomainCreated(IPrfCom * pPrfCom, AppDomainID appDomainId, HRESULT result);
    HRESULT AppDomainShutdown(IPrfCom * pPrfCom, AppDomainID appDomainId, HRESULT result);

    HRESULT ClassUnloadStarted(IPrfCom *  pPrfCom  , ClassID  classId  );

    /*  Success/Failure counters */
    ULONG m_ulSuccess;
    ULONG m_ulFailure;

	CodeInfo2Map       m_CodeInfo2Map;
    FunctionInfoMap    m_FunctionInfoMap;
    FunctionInfo2Map   m_FunctionInfo2Map;
    ThreadAppDomainMap m_threadAppDomainMap;
	AppDomainStack     m_appDomainStack;
    ThreadContextMap   m_threadContextMap;

    AsynchronousTestFlags m_currentFunction;
    
    
    // static methods 
    
    static HRESULT FunctionEnter3WithInfoWrapper(IPrfCom *pPrfCom, FunctionIDOrClientID functionIdOrClientID, COR_PRF_ELT_INFO eltInfo)
	{
		LOCAL_CLASS_POINTER(CAsynchronousFuncTest);

		COR_PRF_FRAME_INFO frameInfo;
		ULONG pcbArgumentInfo = sizeof(COR_PRF_FUNCTION_ARGUMENT_INFO);
		COR_PRF_FUNCTION_ARGUMENT_INFO * pArgumentInfo = new COR_PRF_FUNCTION_ARGUMENT_INFO;

		HRESULT hr = pPrfCom->m_pInfo->GetFunctionEnter3Info(functionIdOrClientID.functionID, eltInfo, &frameInfo, &pcbArgumentInfo, pArgumentInfo);
		if(hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) && hr != S_OK)
		{
			delete pArgumentInfo;

			return hr;
		}
		else if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
		{
			delete pArgumentInfo;
			pArgumentInfo = reinterpret_cast <COR_PRF_FUNCTION_ARGUMENT_INFO *>  (new BYTE[pcbArgumentInfo]); 
			hr = pPrfCom->m_pInfo->GetFunctionEnter3Info(functionIdOrClientID.functionID, eltInfo, &frameInfo, &pcbArgumentInfo, pArgumentInfo);
			if(hr != S_OK)
			{
				delete (BYTE *)pArgumentInfo;
				return E_FAIL;
			}
		}
		delete (BYTE *)pArgumentInfo;

             if (pCAsynchronousFuncTest->m_currentFunction == GET_CODE_INFO_2)      return pCAsynchronousFuncTest->GetCodeInfo2_FunctionEnter3WithInfo(pPrfCom, functionIdOrClientID);
		else if (pCAsynchronousFuncTest->m_currentFunction == GET_FUNCTION_FROM_IP) return pCAsynchronousFuncTest->GetFunctionFromIP_FunctionEnter3WithInfo(pPrfCom, functionIdOrClientID);
		else if (pCAsynchronousFuncTest->m_currentFunction == GET_FUNCTION_INFO)    return pCAsynchronousFuncTest->GetFunctionInfo_FunctionEnter3WithInfo(pPrfCom, functionIdOrClientID);
		else if (pCAsynchronousFuncTest->m_currentFunction == GET_FUNCTION_INFO_2)  return pCAsynchronousFuncTest->GetFunctionInfo2_FunctionEnter3WithInfo(pPrfCom, functionIdOrClientID, frameInfo);
		else if (pCAsynchronousFuncTest->m_currentFunction == GET_THREAD_APPDOMAIN) return pCAsynchronousFuncTest->GetThreadAppDomain_FunctionEnter3WithInfo(pPrfCom);
		else if (pCAsynchronousFuncTest->m_currentFunction == GET_THREAD_CONTEXT)   return pCAsynchronousFuncTest->GetThreadContext_FunctionEnter3WithInfo(pPrfCom);
		else return E_FAIL;
	}


    static HRESULT ThreadCreatedWrapper(IPrfCom * pPrfCom, ThreadID managedThreadId) 
    { 
        LOCAL_CLASS_POINTER(CAsynchronousFuncTest);

             if (pCAsynchronousFuncTest->m_currentFunction == GET_THREAD_APPDOMAIN) return pCAsynchronousFuncTest->ThreadCreated(pPrfCom, managedThreadId);
        else if (pCAsynchronousFuncTest->m_currentFunction == GET_THREAD_CONTEXT)   return pCAsynchronousFuncTest->Context_ThreadCreated(pPrfCom, managedThreadId);
        
        else return E_FAIL;
    }


    static HRESULT ThreadDestroyedWrapper(IPrfCom * pPrfCom, ThreadID managedThreadId) 
    { 
        LOCAL_CLASS_POINTER(CAsynchronousFuncTest);

             if (pCAsynchronousFuncTest->m_currentFunction == GET_THREAD_APPDOMAIN) return pCAsynchronousFuncTest->ThreadDestroyed(pPrfCom, managedThreadId);
        else if (pCAsynchronousFuncTest->m_currentFunction == GET_THREAD_CONTEXT)   return pCAsynchronousFuncTest->Context_ThreadDestroyed(pPrfCom, managedThreadId);
        
        else return E_FAIL;
    }


    static HRESULT AppDomainCreatedWrapper(IPrfCom * pPrfCom, AppDomainID appDomainId, HRESULT result) 
    { 
        return STATIC_CLASS_CALL(CAsynchronousFuncTest)->AppDomainCreated(pPrfCom, appDomainId, result); 
    }


	static HRESULT AppDomainShutdownWrapper(IPrfCom * pPrfCom, AppDomainID appDomainId, HRESULT result) 
    { 
        return STATIC_CLASS_CALL(CAsynchronousFuncTest)->AppDomainShutdown(pPrfCom, appDomainId, result); 
    }


    static HRESULT ClassUnloadStartedWrapper(IPrfCom * pPrfCom,
                                            ClassID classId)
    {
        return STATIC_CLASS_CALL(CAsynchronousFuncTest)->ClassUnloadStarted(pPrfCom, classId);
    }

};
