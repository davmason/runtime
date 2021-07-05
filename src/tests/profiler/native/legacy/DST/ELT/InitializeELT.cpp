#include "ProfilerCommon.h"
extern PMODULEMETHODTABLE pCallTable; 
extern IPrfCom * g_satellite_pPrfCom;

#pragma region Init_Func_Declaraions
void SlowPathELT2_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
void FastPathELT3_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
void SlowPathELT3_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
void SlowPathELT3IncorrectFlags_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
void FastPathELT3IncorrectFlags_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
void SlowPathELT3IncorrectFlagsDSS_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
#pragma endregion

extern "C" void ELT_Satellite_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& testName)
{
	DISPLAY(L"ELT Test Module : " << testName);

	if (testName == L"SlowPathELT2")        							    SlowPathELT2_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"FastPathELT3")        							FastPathELT3_Initialize(pPrfCom, pModuleMethodTable);
	else if (testName == L"SlowPathELT3")        							SlowPathELT3_Initialize(pPrfCom, pModuleMethodTable);
	else if (testName == L"SlowPathELT3IncorrectFlags")						SlowPathELT3IncorrectFlags_Initialize(pPrfCom, pModuleMethodTable);
	else if (testName == L"FastPathELT3IncorrectFlags")						FastPathELT3IncorrectFlags_Initialize(pPrfCom, pModuleMethodTable);
	else if (testName == L"SlowPathELT3IncorrectFlagsDSS")					SlowPathELT3IncorrectFlagsDSS_Initialize(pPrfCom, pModuleMethodTable);
    else 
		FAILURE(L"Test name not recognized!");
	pCallTable = pModuleMethodTable; 
	g_satellite_pPrfCom = pPrfCom;

	return;
}
