 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// VerifyInlinerLocalsLimit.cpp
//
// new profiler test that using the RunProf/TestProfiler 
// framework.
// the test is verify that when the inliner's limit of 512 locals is reached, inlining stops for any 
// other inlining candidates.
// the test checks for FunctionEnter2 call back for all methods decorated with "_Inline" in the function name.
// this profiler test is run to check if the inliner stops inlining methods if the internal limit of 512K locals is reached.
// the test is run against the exe jit\Regression\CLR-x86-JIT\v2.1\b602004\ to verify 31 methods were INLINED.
// if we get a FunctionEnter2 call back for more than 31 method that is decorated with "_Inline", the test fails.
// ======================================================================================

#include "ProfilerCommon.h"

class CInlinerLocalLimit
{
    public:

        CInlinerLocalLimit(IPrfCom * pPrfCom);
        ~CInlinerLocalLimit();

        static HRESULT NewTestProfilerCallbackWrapper(IPrfCom * pPrfCom, ClassID classId) 
        { 
            return STATIC_CLASS_CALL(CInlinerLocalLimit)->NewTestProfilerCallback(pPrfCom, classId); 
        }

        HRESULT Verify(IPrfCom * pPrfCom);

		#pragma region static_wrapper_methods
		static HRESULT FunctionEnter2Wrapper(IPrfCom * pPrfCom,
												FunctionID mappedFuncId,
												UINT_PTR clientData,
												COR_PRF_FRAME_INFO func)
		{
			return STATIC_CLASS_CALL(CInlinerLocalLimit)->FunctionEnter2(pPrfCom, mappedFuncId, clientData, func);
		}

		static HRESULT FunctionEnter3WithInfoWrapper(IPrfCom * pPrfCom,
												FunctionIDOrClientID functionIDOrClientID,
                                             COR_PRF_ELT_INFO eltInfo)
		{
			return STATIC_CLASS_CALL(CInlinerLocalLimit)->FunctionEnter3WithInfo(pPrfCom, functionIDOrClientID, eltInfo);
		}

		#pragma endregion

		#pragma region callback_handler_prototypes
		HRESULT CInlinerLocalLimit::FunctionEnter2(
                                        IPrfCom *  pPrfCom  ,
                                        FunctionID  mappedFuncId  ,
                                        UINT_PTR /* clientData */ ,
                                        COR_PRF_FRAME_INFO  func  );


		HRESULT CInlinerLocalLimit::FunctionEnter3WithInfo(
                                        IPrfCom *  pPrfCom  ,
                                        FunctionIDOrClientID functionIDOrClientID,
                                             COR_PRF_ELT_INFO eltInfo);

		#pragma endregion




    private:

        HRESULT NewTestProfilerCallback(IPrfCom *pPrfCom, ClassID classId);

        /*  Success/Failure counters */
        ULONG m_ulSuccess;
        ULONG m_ulFailure;
};


/*
 *  Initialize the CInlinerLocalLimit class.  
 */
CInlinerLocalLimit::CInlinerLocalLimit(IPrfCom * /*pPrfCom*/)
{
    // Initialize result counters
    m_ulSuccess = 0;
    m_ulFailure = 0;
}


/*
 *  Clean up
 */
CInlinerLocalLimit::~CInlinerLocalLimit()
{
}


HRESULT CInlinerLocalLimit::FunctionEnter2(
                                        IPrfCom *  pPrfCom  ,
                                        FunctionID  mappedFuncId  ,
                                        UINT_PTR /* clientData */ ,
                                        COR_PRF_FRAME_INFO  func  )
{
    WCHAR_STR (funcName);
	HRESULT hr =pPrfCom->GetFunctionIDName(mappedFuncId, funcName, func, false);
	if(FAILED(hr))
	{
		return E_FAIL;
	}
	else
	{
		       
        if (funcName.find(L"_Inline") == string::npos)
		{
			m_ulSuccess+=1;
			//return S_OK;
        }
		else
		{
			DISPLAY(L"Got FunctionEnter2 Callback for FunctionName:" <<funcName <<":"<< HEX(mappedFuncId)); 
			m_ulFailure+=1;
		}

		return S_OK;
		
	}
}
HRESULT CInlinerLocalLimit::FunctionEnter3WithInfo(
                                        IPrfCom *  pPrfCom  , FunctionIDOrClientID functionIDOrClientID,
                                             COR_PRF_ELT_INFO eltInfo)
{
	HRESULT hr = S_OK;
	COR_PRF_FRAME_INFO frameInfo = NULL;
	ULONG pcbArgumentInfo = sizeof(COR_PRF_FUNCTION_ARGUMENT_INFO);
	COR_PRF_FUNCTION_ARGUMENT_INFO * pArgumentInfo = new COR_PRF_FUNCTION_ARGUMENT_INFO;
	hr = pPrfCom->m_pInfo->GetFunctionEnter3Info(functionIDOrClientID.functionID, eltInfo, &frameInfo, &pcbArgumentInfo, pArgumentInfo);
	if(hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) && hr != S_OK)
	{
		delete pArgumentInfo;

		FAILURE("\nCall to GetFunctionEnter3Info failed inside CInlinerLocalLimit::FunctionEnter3WithInfo with invalid HR")
		return hr;
	}
	else if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
	{
		delete pArgumentInfo;
		pArgumentInfo = reinterpret_cast <COR_PRF_FUNCTION_ARGUMENT_INFO *>  (new BYTE[pcbArgumentInfo]); 

		hr = pPrfCom->m_pInfo->GetFunctionEnter3Info(functionIDOrClientID.functionID, eltInfo, &frameInfo, &pcbArgumentInfo, pArgumentInfo);
		if(hr != S_OK)
		{
			delete (BYTE *)pArgumentInfo;
			printf("\nGetFunctionEnter3Info failed with E_FAIL");
			return E_FAIL;
		}
	}
	delete[] (BYTE *) pArgumentInfo;
	WCHAR_STR(funcName);
	// TODO This seems to be a mistake, in the ELT2 implementation, we get COR_PRF_FRAME_INFO to be 0, which should be a bug. To confirm with shaney
	// hr =pPrfCom->GetFunctionIDName(functionIDOrClientID.functionID, funcName, frameInfo, false);
	printf ("\nFunctionEnter3WithInfo frameInfo = %p", (void *)frameInfo);
	hr =pPrfCom->GetFunctionIDName(functionIDOrClientID.functionID, funcName, 0, false);
	if(FAILED(hr))
	{
		return E_FAIL;
	}
	else
	{
		       
        if (funcName.find(L"_Inline") == string::npos)
		{
			m_ulSuccess+=1;
			//return S_OK;
        }
		else
		{
			DISPLAY(L"Got FunctionEnter3WithInfo Callback for FunctionName:" <<funcName <<":"<< HEX(functionIDOrClientID.functionID)); 
			m_ulFailure+=1;
		}

		return S_OK;
		
	}

	
}

/*
 *  Verify the results of this run.
 */
HRESULT CInlinerLocalLimit::Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"CInlinerLocalLimit Callback Verification\n");
	//check if the number of functions with "_Inline" that did not get Inlined is equal to 
	//168(Total number of functions - 31 functions that we expect to be Inlined.)
    if (m_ulFailure ==168)
    {
        DISPLAY(L"VerifyInlineLocalsLimit Test Passed!\n"); 
        return S_OK;
	}
    else
    {
		DISPLAY(L"Number of checks that failed: "<<(m_ulFailure-168));
        DISPLAY(L"Either some checks failed, or no successful checks were completed. VerifyInlineLocalsLimit Test Failed")
        return E_FAIL;
    }
}


/*
 *  Verification routine called by TestProfiler
 */
HRESULT CInlinerLocalLimit_Verify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(CInlinerLocalLimit);
    HRESULT hr = pCInlinerLocalLimit->Verify(pPrfCom);

    // Clean up instance of test class
    delete pCInlinerLocalLimit;
    pPrfCom->m_pTestClassInstance = NULL;

    return hr;
}


/*
 *  Initialization routine called by TestProfiler
 */
void CInlinerLocalLimit_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, bool useELT3Callbacks)
{
    DISPLAY(L"Initialize CInlinerLocalLimit.\n")

    // Create and save an instance of test class
    SET_CLASS_POINTER(new CInlinerLocalLimit(pPrfCom));
    
    // Initialize MethodTable
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_ENTERLEAVE |
		                        COR_PRF_ENABLE_FUNCTION_ARGS;

    REGISTER_CALLBACK(VERIFY, CInlinerLocalLimit_Verify);
	if(!useELT3Callbacks)
	{
		REGISTER_CALLBACK(FUNCTIONENTER2, CInlinerLocalLimit::FunctionEnter2Wrapper);
	}
	else
	{
		REGISTER_CALLBACK(FUNCTIONENTER3WITHINFO, CInlinerLocalLimit::FunctionEnter3WithInfoWrapper);
	}

    return;
}

