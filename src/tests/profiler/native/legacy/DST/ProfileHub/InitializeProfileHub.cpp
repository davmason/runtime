// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "../../ProfilerCommon.h"
extern PMODULEMETHODTABLE pCallTable; 
extern IPrfCom * g_satellite_pPrfCom;

#pragma region Init_Func_Declaraions

    // CorPrfProfileFlags
    void CorPrfProfileFlags_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, BOOL useProfileImageFlag, BOOL useExpectCallbacks, BOOL useCodeGenFlags, BOOL useMonitorEnterLeaveFlag, bool useELT3Callbacks);
    void CorPrfFlagsOnly_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);

    // Asynchronous
    typedef enum _AsynchronousTestFlags
    {
        TEST_NONE            = 0x00,
        GET_CODE_INFO        = 0x01,
        GET_CODE_INFO_2      = 0x02,
        GET_FUNCTION_FROM_IP = 0x04,
        GET_FUNCTION_INFO    = 0x08,
        GET_FUNCTION_INFO_2  = 0x10,
        GET_THREAD_APPDOMAIN = 0x20,
        GET_THREAD_CONTEXT   = 0x40,
        TEST_ALL             = 0x80,
    } AsynchronousTestFlags;

    void CAsynchronousFuncTest_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, AsynchronousTestFlags testFunc);

    void AllCallbacks_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void AllCallbacksWithDrilldown_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
	void ILToNativeMap_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);

    enum SynchronousCallMethod { NO_SYNCH, CURRENT_THREAD, CROSS_THREAD, SYNC_ASP };
    enum AsynchronousSampleMethod { NO_ASYNCH, SAMPLE, SAMPLE_AND_TEST_DEADLOCK, HIJACK, HIJACKANDINSPECT, ASYNC_AVAILABILITY };
    
    void DSS_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, SynchronousCallMethod requestedCallMethod, AsynchronousSampleMethod requestedSampleMethod);

#pragma endregion


extern "C" void ProfileHub_Satellite_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& testName)
{
    DISPLAY(L"Profile Hub Test Module : " << testName);

#pragma region CorPrfProfileFlags

         if (testName == L"UseCorPrfProfileImage")        CorPrfProfileFlags_Initialize(pPrfCom, pModuleMethodTable, TRUE,  TRUE,  FALSE, FALSE, false);
	else if (testName == L"NoCorPrfProfileImageNoNGEN")   CorPrfProfileFlags_Initialize(pPrfCom, pModuleMethodTable, FALSE, TRUE,  FALSE, FALSE, false);
	else if (testName == L"NoCorPrfProfileImageWithNGEN") CorPrfProfileFlags_Initialize(pPrfCom, pModuleMethodTable, FALSE, FALSE, FALSE, FALSE, false);
	else if (testName == L"UseCodeGenFlags")              CorPrfProfileFlags_Initialize(pPrfCom, pModuleMethodTable, FALSE, FALSE, TRUE,  FALSE, false);
	else if (testName == L"UseMonitorEnterLeaveFlag")     CorPrfProfileFlags_Initialize(pPrfCom, pModuleMethodTable, FALSE, TRUE,  FALSE, TRUE , false);
	else if (testName == L"UseMonitorEnterLeaveFlagELT3")     CorPrfProfileFlags_Initialize(pPrfCom, pModuleMethodTable, FALSE, TRUE,  FALSE, TRUE , true);
    else if (testName == L"FlagOnlyRepro")                CorPrfFlagsOnly_Initialize(pPrfCom, pModuleMethodTable);

#pragma endregion // CorPrfProfileFlags

#pragma region Asynchronous

    else if (testName == L"GetCodeInfo2")       CAsynchronousFuncTest_Initialize(pPrfCom, pModuleMethodTable, GET_CODE_INFO_2);
    else if (testName == L"GetFunctionFromIP")  CAsynchronousFuncTest_Initialize(pPrfCom, pModuleMethodTable, GET_FUNCTION_FROM_IP);
    else if (testName == L"GetFunctionInfo")    CAsynchronousFuncTest_Initialize(pPrfCom, pModuleMethodTable, GET_FUNCTION_INFO);
    else if (testName == L"GetFunctionInfo2")   CAsynchronousFuncTest_Initialize(pPrfCom, pModuleMethodTable, GET_FUNCTION_INFO_2);
    else if (testName == L"GetThreadAppDomain") CAsynchronousFuncTest_Initialize(pPrfCom, pModuleMethodTable, GET_THREAD_APPDOMAIN);
    else if (testName == L"GetThreadContext")   CAsynchronousFuncTest_Initialize(pPrfCom, pModuleMethodTable, GET_THREAD_CONTEXT);

#pragma endregion // Asynchronous

#pragma region DoStackSnapshot

    else if (testName == L"SynchronousSelf")        DSS_Initialize(pPrfCom, pModuleMethodTable, CURRENT_THREAD, NO_ASYNCH);
    else if (testName == L"SynchronousCrossThread") DSS_Initialize(pPrfCom, pModuleMethodTable, CROSS_THREAD, NO_ASYNCH);
    else if (testName == L"Sample")                 DSS_Initialize(pPrfCom, pModuleMethodTable, NO_SYNCH, SAMPLE);
    else if (testName == L"SampleAndTestDeadlock")  DSS_Initialize(pPrfCom, pModuleMethodTable, NO_SYNCH, SAMPLE_AND_TEST_DEADLOCK);
    else if (testName == L"HiJack")                 DSS_Initialize(pPrfCom, pModuleMethodTable, NO_SYNCH, HIJACK);
    else if (testName == L"HiJackAndInspect")       DSS_Initialize(pPrfCom, pModuleMethodTable, NO_SYNCH, HIJACKANDINSPECT);
    else if (testName == L"Availability")           DSS_Initialize(pPrfCom, pModuleMethodTable, NO_SYNCH, ASYNC_AVAILABILITY);
    else if (testName == L"ASP")                    DSS_Initialize(pPrfCom, pModuleMethodTable, SYNC_ASP, NO_ASYNCH);

#pragma endregion // DoStackSnapshot

    else if (testName == L"AllCallbacks") AllCallbacks_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"AllCallbacksWithDrilldown") AllCallbacksWithDrilldown_Initialize(pPrfCom, pModuleMethodTable);
	else if (testName == L"ILToNativeMap") ILToNativeMap_Initialize(pPrfCom, pModuleMethodTable);

    else FAILURE(L"Test name not recognized!");

	pCallTable = pModuleMethodTable; 
	g_satellite_pPrfCom = pPrfCom;

	return;
}

