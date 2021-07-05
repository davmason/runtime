#include "../ProfilerCommon.h"

extern PMODULEMETHODTABLE pCallTable; 
extern IPrfCom * g_satellite_pPrfCom;


// Test Initialization Function Declarations
void TieredCompilation_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);


extern "C" void Tiered_Compilation_Satellite_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& testName)
{
    pCallTable = pModuleMethodTable; 
    g_satellite_pPrfCom = pPrfCom;

    DISPLAY(L"Tiered Jitting Tests Initialize : " << testName);

    if (testName == L"NewProfilerAPIS")             
	{
		TieredCompilation_Initialize(pPrfCom, pModuleMethodTable);
	}
    else 
	{	
		FAILURE(L"Test name [" << testName << L"] not recognized!");
	}

	return;
}
