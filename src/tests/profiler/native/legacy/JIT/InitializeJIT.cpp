#include "JITCommon.h"
extern PMODULEMETHODTABLE pCallTable; 
extern IPrfCom * g_satellite_pPrfCom;

// Legacy support
#ifndef _INCLUDE_LEGACY_FUNCTIONS_
#define _INCLUDE_LEGACY_FUNCTIONS_
#include "LegacyCompat.h"
extern IPrfCom* g_pPrfCom_LegacyCompat;
#endif
// Legacy support

void FuncIDMapper_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
void FuncIDMapperELT3_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, bool fastPath);
void FuncIDMapper2_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, bool fastPath);
void GetFunctionInfo2_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, bool ELT3 );
void ReJIT_Initialize(IPrfCom * /*pPrfCom*/, PMODULEMETHODTABLE pModuleMethodTable, const PLATFORM_WCHAR * /*TestName*/);
void ShadowStackTest_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, BOOL bEnableDoStackSnapshot, BOOL bEnableSlowEnterLeave, bool useELT3Callbacks);
void CInlineTest_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, bool useELT3Callbacks);
void CNoInlineTest_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, bool useELT3Callbacks);
void CInlinerLocalLimit_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, bool useELT3Callbacks);
void CollectMpgoProfilerData_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, bool useELT3Callbacks);

extern "C" void JIT_Satellite_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& testName)
{
	pPrfCom->FreeOutputFiles(TRUE);
    JITCommon * pJITCommon = new JITCommon(pPrfCom->m_pInfo);
    SET_DERIVED_POINTER(pJITCommon);

    // Legacy support
    g_pPrfCom_LegacyCompat = pModuleMethodTable->IPPRFCOM;
    // Legacy support

         if (testName == L"FuncIDMapper")					FuncIDMapper_Initialize(pJITCommon, pModuleMethodTable);
	else if (testName == L"FuncIDMapper2Fast")              FuncIDMapper2_Initialize(pJITCommon, pModuleMethodTable, true );
	else if (testName == L"FuncIDMapper2Slow")              FuncIDMapper2_Initialize(pJITCommon, pModuleMethodTable, false);
	else if (testName == L"FuncIDMapperELT3Fast")           FuncIDMapperELT3_Initialize(pJITCommon, pModuleMethodTable, true);
	else if (testName == L"FuncIDMapperELT3Slow")           FuncIDMapperELT3_Initialize(pJITCommon, pModuleMethodTable, false);
    else if (testName == L"FunctionReJIT")					ReJIT_Initialize(pJITCommon, pModuleMethodTable, L"");
    else if (testName == L"GetFunctionInfo2")				GetFunctionInfo2_Initialize(pJITCommon, pModuleMethodTable, false);
	else if (testName == L"GetFunctionInfo2ELT3")			GetFunctionInfo2_Initialize(pJITCommon, pModuleMethodTable, true);
    else if (testName == L"ShadowStackFastEnterLeave")		ShadowStackTest_Initialize(pJITCommon, pModuleMethodTable, FALSE, FALSE, false);
    else if (testName == L"ShadowStackSlowEnterLeave")		ShadowStackTest_Initialize(pJITCommon, pModuleMethodTable, FALSE, TRUE, false);
	else if (testName == L"ShadowStackFastEnterLeave3")		ShadowStackTest_Initialize(pJITCommon, pModuleMethodTable, FALSE, FALSE, true);
	else if (testName == L"ShadowStackSlowEnterLeave3")		ShadowStackTest_Initialize(pJITCommon, pModuleMethodTable, FALSE, TRUE, true);
    else if (testName == L"ShadowStackFastWithDSS")			ShadowStackTest_Initialize(pJITCommon, pModuleMethodTable, TRUE, FALSE, false);
    else if (testName == L"ShadowStackSlowWithDSS")			ShadowStackTest_Initialize(pJITCommon, pModuleMethodTable, TRUE, TRUE, false);
	else if (testName == L"ShadowStackFastWithDSSELT3")     ShadowStackTest_Initialize(pJITCommon, pModuleMethodTable, TRUE, FALSE, true);
	else if (testName == L"ShadowStackSlowWithDSSELT3")     ShadowStackTest_Initialize(pJITCommon, pModuleMethodTable, TRUE, TRUE, true);
	else if (testName == L"InlineVerify")					CInlineTest_Initialize(pJITCommon, pModuleMethodTable, false);	
	else if (testName == L"InlineVerifyELT3")				CInlineTest_Initialize(pJITCommon, pModuleMethodTable, true);	
	else if (testName == L"NoInline_Verify")				CNoInlineTest_Initialize(pJITCommon, pModuleMethodTable, false);	
	else if (testName == L"NoInline_VerifyELT3")			CNoInlineTest_Initialize(pJITCommon, pModuleMethodTable, true);	
	else if (testName == L"VerifyInlinerLocalsLimit")		CInlinerLocalLimit_Initialize(pJITCommon, pModuleMethodTable, false);
	else if (testName == L"VerifyInlinerLocalsLimitELT3")	CInlinerLocalLimit_Initialize(pJITCommon, pModuleMethodTable, true);
	else if (testName == L"CollectMpgoProfilerData")		CollectMpgoProfilerData_Initialize(pJITCommon, pModuleMethodTable, false);
	else if (testName == L"CollectMpgoProfilerDataELT3")	CollectMpgoProfilerData_Initialize(pJITCommon, pModuleMethodTable, true);
    else FAILURE(L"Test name not recognized!");

	pCallTable = pModuleMethodTable; 
	g_satellite_pPrfCom = pJITCommon;

	return;
}

