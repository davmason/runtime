// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// InlineVerify.cpp
//
// new profiler test that using the RunProf/TestProfiler 
// framework.
// the test checks for FunctionEnter2 call back for all methods decorated with "_NoInline" in the function name.
// this profiler test is run for all the tests in the JIT\Opt\Inline category to verify the methods are NOT INLINED.
// if we GET a FunctionEnter2 call back for methods that are decorated with "_NoInline", the test PASSES.
// ======================================================================================

#include "../ProfilerCommon.h"

class CNoInlineTest
{
    public:

        CNoInlineTest(IPrfCom * pPrfCom);
        ~CNoInlineTest();


        HRESULT Verify(IPrfCom * pPrfCom);

		#pragma region static_wrapper_methods
		static HRESULT FunctionEnter2Wrapper(IPrfCom * pPrfCom,
                                             FunctionID funcId,
                                             UINT_PTR clientData,
                                             COR_PRF_FRAME_INFO func,
                                             COR_PRF_FUNCTION_ARGUMENT_INFO * argumentInfo)
		{
			return STATIC_CLASS_CALL(CNoInlineTest)->FunctionEnter2(pPrfCom, funcId, clientData, func, argumentInfo);
		}

		static HRESULT FunctionEnter3WithInfoWrapper(IPrfCom * pPrfCom,
                                             FunctionIDOrClientID functionIDOrClientID,
                                             COR_PRF_ELT_INFO eltInfo)
		{
			return STATIC_CLASS_CALL(CNoInlineTest)->FunctionEnter3WithInfo(pPrfCom, functionIDOrClientID, eltInfo);
		}

		#pragma endregion

		#pragma region callback_handler_prototypes
		HRESULT CNoInlineTest::FunctionEnter2(IPrfCom * pPrfCom,
                                              FunctionID funcId,
                                              UINT_PTR clientData,
                                              COR_PRF_FRAME_INFO func,
                                              COR_PRF_FUNCTION_ARGUMENT_INFO * argumentInfo);

		HRESULT CNoInlineTest::FunctionEnter3WithInfo(IPrfCom * pPrfCom,
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
 *  Initialize the CNoInlineTest class.  
 */
CNoInlineTest::CNoInlineTest(IPrfCom * /*pPrfCom*/)
{
    // Initialize result counters
    m_ulSuccess = 0;
    m_ulFailure = 0;
}


/*
 *  Clean up
 */
CNoInlineTest::~CNoInlineTest()
{
}


HRESULT CNoInlineTest::FunctionEnter2(IPrfCom * pPrfCom,
                                      FunctionID funcId,
                                      UINT_PTR /*clientData*/,
                                      COR_PRF_FRAME_INFO func,
                                      COR_PRF_FUNCTION_ARGUMENT_INFO * /*argumentInfo*/)
{
    WCHAR_STR ( funcName );
	HRESULT hr =pPrfCom->GetFunctionIDName(funcId, funcName, func, false);
	if(FAILED(hr))
	{
		return E_FAIL;
	}
	else
	{
		       
		if (funcName.find(L"_NoInline") != string::npos)
		{
			m_ulSuccess+=1;
			DISPLAY(L"Got FunctionEnter2 Callback for FunctionName:" <<funcName<<":" << HEX(funcId));
			DISPLAY(L"Method was not expected to be inlined. NoInline_Verify Test Passed for function -" <<funcName);
        }
		return S_OK;
				
	}
}

HRESULT CNoInlineTest::FunctionEnter3WithInfo(IPrfCom * pPrfCom,
											  FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo)
{
	HRESULT hr = S_OK;
	COR_PRF_FRAME_INFO frameInfo;
	ULONG pcbArgumentInfo = sizeof(COR_PRF_FUNCTION_ARGUMENT_INFO);
	COR_PRF_FUNCTION_ARGUMENT_INFO * pArgumentInfo = new COR_PRF_FUNCTION_ARGUMENT_INFO;
	hr = pPrfCom->m_pInfo->GetFunctionEnter3Info(functionIDOrClientID.functionID, eltInfo, &frameInfo, &pcbArgumentInfo, pArgumentInfo);
	if(hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) && hr != S_OK)
	{
		delete pArgumentInfo;
		FAILURE("\nGetFunctionEnter3Info call inside CNoInlineTest::FunctionEnter3WithInfo failed with invalid HR code")
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
	delete[] (BYTE *)pArgumentInfo;
	WCHAR_STR( funcName );
	// TODO This seems to be a mistake, in the ELT2 implementation, we get COR_PRF_FRAME_INFO to be 0, which should be a bug. To confirm with shaney
	// hr =pPrfCom->GetFunctionIDName(functionIDOrClientID.functionID, funcName, frameInfo, false);
	hr =pPrfCom->GetFunctionIDName(functionIDOrClientID.functionID, funcName, 0, false);
	if(FAILED(hr))
	{
		return E_FAIL;
	}
	else
	{
		if (funcName.find(L"_NoInline") != string::npos)
		{
			m_ulSuccess+=1;
			DISPLAY(L"Got FunctionEnter3WithInfo Callback for FunctionName:" <<funcName<<":" << HEX(functionIDOrClientID.functionID));
			DISPLAY(L"Method was not expected to be inlined. NoInline_Verify Test Passed for function -" <<funcName);
		}
		return S_OK;
	}
}


/*
 *  Verify the results of this run.
 */
HRESULT CNoInlineTest::Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"CNoInlineTest Callback Verification\n");
	if(m_ulSuccess >0)
    {
		DISPLAY(L"No of checks that passed:" <<m_ulSuccess);
        DISPLAY(L"NoInline_Verify Test Passed!")
        return S_OK;
    }
    else
    {
        DISPLAY(L"Either some checks failed, or no successful checks were completed. NoInline_Verify Test Failed")
        return E_FAIL;
    }
}


/*
 *  Verification routine called by TestProfiler
 */
HRESULT CNoInlineTest_Verify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(CNoInlineTest);
    HRESULT hr = pCNoInlineTest->Verify(pPrfCom);

    // Clean up instance of test class
    delete pCNoInlineTest;
    pPrfCom->m_pTestClassInstance = NULL;

    return hr;
}


/*
 *  Initialization routine called by TestProfiler
 */
void CNoInlineTest_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, bool useELT3Callbacks)
{
    DISPLAY(L"Initialize CNoInlineTest.\n")
    // Create and save an instance of test class
    SET_CLASS_POINTER(new CNoInlineTest(pPrfCom));
    // Initialize MethodTable
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_ENTERLEAVE | COR_PRF_ENABLE_FUNCTION_ARGS;
    REGISTER_CALLBACK(VERIFY, CNoInlineTest_Verify);
	if(!useELT3Callbacks)
	{
		REGISTER_CALLBACK(FUNCTIONENTER2, CNoInlineTest::FunctionEnter2Wrapper);
	}
	else
	{
		REGISTER_CALLBACK(FUNCTIONENTER3WITHINFO, CNoInlineTest::FunctionEnter3WithInfoWrapper);
	}
}

