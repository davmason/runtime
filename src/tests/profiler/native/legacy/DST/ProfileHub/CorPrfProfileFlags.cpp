#include "../../ProfilerCommon.h"

class CorPrfProfileFlags
{
    public:

    CorPrfProfileFlags(BOOL useProfileImageFlag, 
                       BOOL useExpectCallbacks, 
                       BOOL useCodeGenFlags, 
                       BOOL useMonitorEnterLeaveFlag);

    static HRESULT ClassLoadStartedWrapper(IPrfCom * pPrfCom, ClassID classId)
    {
        classLoadStartedReceived++;
        
        if (callbacksExpected == FALSE)
        {
            STATIC_CLASS_CALL(CorPrfProfileFlags)->DebugClassLoad(pPrfCom, classId);
        }
        
        return S_OK;
    }

    static HRESULT FuncEnter2Wrapper(IPrfCom * pPrfCom, FunctionID functionId, UINT_PTR , COR_PRF_FRAME_INFO frameInfo, COR_PRF_FUNCTION_ARGUMENT_INFO *)
    {
        funcEnterLeaveReceived++;
        
        if (callbacksExpected == FALSE)
        {
            STATIC_CLASS_CALL(CorPrfProfileFlags)->DebugFunctionEnter(pPrfCom, functionId, frameInfo);
        }

        return S_OK;
    }

	static HRESULT FunctionEnter3WithInfoWrapper(IPrfCom * pPrfCom, FunctionIDOrClientID functionIdOrClientID, COR_PRF_ELT_INFO eltInfo)
	{
		funcEnterLeaveReceived++;

		if (callbacksExpected == FALSE)
		{
			COR_PRF_FRAME_INFO frameInfo;
			ULONG pcbArgumentInfo = sizeof(COR_PRF_FUNCTION_ARGUMENT_INFO);
			COR_PRF_FUNCTION_ARGUMENT_INFO * pArgumentInfo = new COR_PRF_FUNCTION_ARGUMENT_INFO;
			HRESULT hr = pPrfCom->m_pInfo->GetFunctionEnter3Info(functionIdOrClientID.functionID, eltInfo, &frameInfo, &pcbArgumentInfo, pArgumentInfo);
			if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) && hr != S_OK)
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
			STATIC_CLASS_CALL(CorPrfProfileFlags)->DebugFunctionEnter(pPrfCom, functionIdOrClientID.functionID, frameInfo);
		}
		return S_OK;
	}

    VOID DebugClassLoad(IPrfCom * pPrfCom, ClassID classId);
    VOID DebugFunctionEnter(IPrfCom * pPrfCom, FunctionID funcId, COR_PRF_FRAME_INFO frameInfo);

    HRESULT Verify(IPrfCom * pPrfCom);

    static BOOL callbacksExpected;
    BOOL useClassLoad;
    BOOL usePrfImage;
    BOOL useEnterLeave;
    BOOL useCodeGen;

    static ULONG classLoadStartedReceived;
    static ULONG funcEnterLeaveReceived;
};

// Static Variables
BOOL  CorPrfProfileFlags::callbacksExpected;
ULONG CorPrfProfileFlags::classLoadStartedReceived;
ULONG CorPrfProfileFlags::funcEnterLeaveReceived;

CorPrfProfileFlags::CorPrfProfileFlags(BOOL useProfileImageFlag, 
                                       BOOL useExpectCallbacks, 
                                       BOOL useCodeGenFlags, 
                                       BOOL useMonitorEnterLeaveFlag)
{
    usePrfImage       = useProfileImageFlag;
    callbacksExpected = useExpectCallbacks;
    useCodeGen        = useCodeGenFlags;
    useEnterLeave     = useMonitorEnterLeaveFlag;

    if (TRUE == useCodeGenFlags || TRUE == useMonitorEnterLeaveFlag)
    {
        useClassLoad = FALSE;
    }
    else
    {
        useClassLoad = TRUE;
    }

    classLoadStartedReceived = 0;
    funcEnterLeaveReceived = 0;
}

VOID CorPrfProfileFlags::DebugClassLoad(IPrfCom * pPrfCom, ClassID classId)
{
    WCHAR_STR( className );
    
    DISPLAY(L"Unexpected ClassLoad event received.  Investigating ClassID=" << HEX(classId) << L"...");
    MUST_PASS(PPRFCOM->GetClassIDName(classId, className, true));
    DISPLAY(L"ClassLoad event received for ' " << className << L" '");
}

VOID CorPrfProfileFlags::DebugFunctionEnter(IPrfCom * pPrfCom, FunctionID funcId, COR_PRF_FRAME_INFO frameInfo)
{
    WCHAR_STR( funcName );
    
    DISPLAY(L"Unexpected FunctionEnter event received.  Investigating FunctionID=" << HEX(funcId) << L"...");
    MUST_PASS(PPRFCOM->GetFunctionIDName(funcId, funcName, frameInfo, true));
    DISPLAY(L"ClassLoad event received for ' " << funcName << L" '");
}

HRESULT CorPrfProfileFlags::Verify(IPrfCom * pPrfCom)
{
    if (TRUE == callbacksExpected)
    {
        if (TRUE == useClassLoad && 0 == classLoadStartedReceived)
        {
            FAILURE(L"We expected to receive ClassLoad events and did not!\n");
            return E_FAIL;
        }

        if (TRUE == useEnterLeave && 0 == funcEnterLeaveReceived)
        {
            FAILURE(L"We expected to receive Function Enter/Leave events and did not!\n");
            return E_FAIL;
        }
    }
    else
    {
        if (TRUE == useClassLoad && 0 != classLoadStartedReceived)
        {
            FAILURE(L"We did NOT expected to receive ClassLoad events and received " << classLoadStartedReceived);
            return E_FAIL;
        }

        if (TRUE == useEnterLeave && 0 != funcEnterLeaveReceived)
        {
            FAILURE(L"We did NOT expected to receive Function Enter/Leave events and received " << funcEnterLeaveReceived);
            return E_FAIL;
        }
    }

    DISPLAY(L"Test Passed!\n");
    return S_OK;
}


/*
 *  Straight function callback used by TestProfiler
 */
HRESULT CorPrfProfileFlags_Verify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(CorPrfProfileFlags);
    
    MUST_PASS(pCorPrfProfileFlags->Verify(pPrfCom))

    delete pCorPrfProfileFlags;
    pPrfCom->m_pTestClassInstance = NULL;

    return S_OK;
}


void CorPrfProfileFlags_Initialize (IPrfCom * pPrfCom, 
                                    PMODULEMETHODTABLE pModuleMethodTable,
                                    BOOL useProfileImageFlag, 
                                    BOOL useExpectCallbacks, 
                                    BOOL useCodeGenFlags, 
                                    BOOL useMonitorEnterLeaveFlag,
                                    bool useELT3Callbacks)
{
    DISPLAY(L"Profiler API Test: CorPrfProfileFlags testing with ClassLoad* Callbacks\n");

    // Create new test object
    SET_CLASS_POINTER(new CorPrfProfileFlags(useProfileImageFlag, 
                                             useExpectCallbacks, 
                                             useCodeGenFlags, 
                                             useMonitorEnterLeaveFlag));

    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_CLASS_LOADS;
    if (TRUE == useProfileImageFlag)      pModuleMethodTable->FLAGS |= COR_PRF_USE_PROFILE_IMAGES;
    if (TRUE == useCodeGenFlags)          pModuleMethodTable->FLAGS  = COR_PRF_DISABLE_INLINING | COR_PRF_DISABLE_OPTIMIZATIONS | COR_PRF_MONITOR_CODE_TRANSITIONS;
    if (TRUE == useMonitorEnterLeaveFlag) pModuleMethodTable->FLAGS  = COR_PRF_MONITOR_ENTERLEAVE | COR_PRF_ENABLE_FUNCTION_ARGS | COR_PRF_DISABLE_INLINING | COR_PRF_DISABLE_OPTIMIZATIONS;

    REGISTER_CALLBACK(VERIFY,           CorPrfProfileFlags_Verify);
    REGISTER_CALLBACK(CLASSLOADSTARTED, CorPrfProfileFlags::ClassLoadStartedWrapper);
    if(!useELT3Callbacks)
    {
        REGISTER_CALLBACK(FUNCTIONENTER2,   CorPrfProfileFlags::FuncEnter2Wrapper);
    }
    else
    {
        REGISTER_CALLBACK(FUNCTIONENTER3WITHINFO,   CorPrfProfileFlags::FunctionEnter3WithInfoWrapper);
    }
}




// Debug repo test case for when the cor_prf_* flags are all it takes to cause a failure.
HRESULT CorPrfFlagsOnly_Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"\nSurvived to the end of the profiling run.\n");
    return S_OK;
}


void CorPrfFlagsOnly_Initialize(IPrfCom * pPrfCom, 
                                PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"\nPaul's COR_PRF_* Flag Only Failure Repro.\n");

    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_CODE_TRANSITIONS;
    REGISTER_CALLBACK(VERIFY, CorPrfFlagsOnly_Verify);
    return;
}
