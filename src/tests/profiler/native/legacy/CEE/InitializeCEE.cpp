#include "ProfilerCommon.h"
extern PMODULEMETHODTABLE pCallTable; 
extern IPrfCom * g_satellite_pPrfCom;

void ThreadNameChangedInit(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
void ex_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, BOOL testASP);

extern "C" void CEE_Satellite_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& testName)
{
    DISPLAY(L"Initialize CEE Module : " << testName);

         if (testName == L"ThreadNameChanged") ThreadNameChangedInit(pPrfCom, pModuleMethodTable);
    else if (testName == L"Exceptions")        ex_Initialize(pPrfCom, pModuleMethodTable, FALSE);
    else if (testName == L"ExceptionsASP")     ex_Initialize(pPrfCom, pModuleMethodTable, TRUE);
    else FAILURE(L"Test name not recognized!");

	pCallTable = pModuleMethodTable; 
	g_satellite_pPrfCom = pPrfCom;

	return;
}
