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
// the test checks for FunctionEnter2 call back for all methods decorated with "_Inline" in the function name.
// this profiler test is run for all the tests in the JIT\Opt\Inline category to verify the methods were INLINED.
// if we get a FunctionEnter2 call back for any method that is decorated with "_Inline", the test fails.
// ======================================================================================

#include "../ProfilerCommon.h"

class CInlineTest
{
    public:

        CInlineTest(IPrfCom * pPrfCom);
        ~CInlineTest();

        HRESULT Verify(IPrfCom * pPrfCom);

		#pragma region static_wrapper_methods
		static HRESULT FunctionEnter2Wrapper(IPrfCom * pPrfCom,
                                             FunctionID funcId,
                                             UINT_PTR clientData,
                                             COR_PRF_FRAME_INFO func,
                                             COR_PRF_FUNCTION_ARGUMENT_INFO * argumentInfo)
		{
			return STATIC_CLASS_CALL(CInlineTest)->FunctionEnter2(pPrfCom, funcId, clientData, func, argumentInfo);
		}
		static HRESULT FunctionEnter3WithInfoWrapper(IPrfCom * pPrfCom,
                                             FunctionIDOrClientID functionIDOrClientID,
                                             COR_PRF_ELT_INFO eltInfo)
		{
			return STATIC_CLASS_CALL(CInlineTest)->FunctionEnter3WithInfo(pPrfCom, functionIDOrClientID, eltInfo);
		}

		#pragma endregion

		#pragma region callback_handler_prototypes
		HRESULT CInlineTest::FunctionEnter2(IPrfCom * pPrfCom,
                                            FunctionID funcId,
                                            UINT_PTR clientData,
                                            COR_PRF_FRAME_INFO func,
                                            COR_PRF_FUNCTION_ARGUMENT_INFO * argumentInfo);


		HRESULT CInlineTest::FunctionEnter3WithInfo(IPrfCom * pPrfCom,FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo);
        

		#pragma endregion


    private:

        HRESULT NewTestProfilerCallback(IPrfCom *pPrfCom, ClassID classId);

        /*  Success/Failure counters */
        ULONG m_ulSuccess;
        ULONG m_ulFailure;
};


/*
 *  Initialize the CInlineTest class.  
 */
CInlineTest::CInlineTest(IPrfCom * /*pPrfCom*/)
{
    // Initialize result counters
    m_ulSuccess = 0;
    m_ulFailure = 0;
}


/*
 *  Clean up
 */
CInlineTest::~CInlineTest()
{
}


HRESULT CInlineTest::FunctionEnter2(IPrfCom * pPrfCom,
                                    FunctionID funcId,
                                    UINT_PTR /*clientData*/,
                                    COR_PRF_FRAME_INFO func,
                                    COR_PRF_FUNCTION_ARGUMENT_INFO * /*argumentInfo*/)
{
    WCHAR_STR( funcName );
	HRESULT hr =pPrfCom->GetFunctionIDName(funcId, funcName, func, false);
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
			DISPLAY(L"Got FunctionEnter2 Callback for FunctionName:" <<funcName <<":"<< HEX(funcId));
			DISPLAY(L"Method expected to be inlined. InlineVerify Test Failed for function -" <<funcName <<HEX(funcId));
			m_ulFailure+=1;
			//return E_FAIL;

		}

		return S_OK;
		
	}
}

HRESULT CInlineTest::FunctionEnter3WithInfo(IPrfCom * pPrfCom,FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo)
{
	HRESULT hr = S_OK;
	COR_PRF_FRAME_INFO frameInfo;
	ULONG pcbArgumentInfo = sizeof(COR_PRF_FUNCTION_ARGUMENT_INFO);
	COR_PRF_FUNCTION_ARGUMENT_INFO * pArgumentInfo = new COR_PRF_FUNCTION_ARGUMENT_INFO;
	hr = pPrfCom->m_pInfo->GetFunctionEnter3Info(functionIDOrClientID.functionID, eltInfo, &frameInfo, &pcbArgumentInfo, pArgumentInfo);
	if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) && hr != S_OK)
	{
		delete pArgumentInfo;
		FAILURE("\nInvalid HR received in CInlineTest::FunctionEnter3WithInfo call to GetFunctionEnter3Info")
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
	delete (BYTE *)pArgumentInfo;
	WCHAR_STR( funcName );
	// TODO This seems to be a mistake, in the ELT2 implementation, we get COR_PRF_FRAME_INFO to be 0, which should be a bug.
	//hr =pPrfCom->GetFunctionIDName(functionIDOrClientID.functionID, funcName, frameInfo, false);
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
		}
		else
		{
			DISPLAY(L"Got FunctionEnter3WithInfo Callback for FunctionName:" <<funcName <<":"<< HEX(functionIDOrClientID.functionID));
			DISPLAY(L"Method expected to be inlined. InlineVerify Test Failed for function -" <<funcName <<HEX(functionIDOrClientID.functionID));
			m_ulFailure+=1;
		}
		return S_OK;
	}
}

/*
 *  Verify the results of this run.
 */
HRESULT CInlineTest::Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"CInlineTest Callback Verification\n");

    if ((m_ulFailure == 0) && (m_ulSuccess > 0))
    {
        DISPLAY(L"InLineVerify Test Passed!")
        return S_OK;
    }
    else
    {
		DISPLAY(L"Number of checks that failed: "<< m_ulFailure);
        DISPLAY(L"Either some checks failed, or no successful checks were completed. InlineVerify Test Failed")
        return E_FAIL;
    }
}

/*
 *  Verification routine called by TestProfiler
 */
HRESULT CInlineTest_Verify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(CInlineTest);
    HRESULT hr = pCInlineTest->Verify(pPrfCom);

    // Clean up instance of test class
    delete pCInlineTest;
    pPrfCom->m_pTestClassInstance = NULL;

    return hr;
}


/*
 *  Initialization routine called by TestProfiler
 */
void CInlineTest_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, bool useELT3Callbacks)
{
	DISPLAY(L"Initialize CInlineTest.\n")
		// Create and save an instance of test class
	SET_CLASS_POINTER(new CInlineTest(pPrfCom));
	// Initialize MethodTable
	pModuleMethodTable->FLAGS = COR_PRF_MONITOR_ENTERLEAVE | COR_PRF_ENABLE_FUNCTION_ARGS;
	REGISTER_CALLBACK(VERIFY, CInlineTest_Verify);
	if(!useELT3Callbacks)
	{
		REGISTER_CALLBACK(FUNCTIONENTER2, CInlineTest::FunctionEnter2Wrapper);
	}
	else
	{
		REGISTER_CALLBACK(FUNCTIONENTER3WITHINFO, CInlineTest::FunctionEnter3WithInfoWrapper);
	}
}

