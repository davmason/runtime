#include "../../ProfilerCommon.h"
extern PMODULEMETHODTABLE pCallTable; 
extern IPrfCom * g_satellite_pPrfCom;

#pragma region init_declarations

    // unit tests
    void Unit_CCW_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void Unit_EnumModules_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void Unit_EnumJittedFunctions_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void Unit_EnumThreads_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
#pragma endregion



extern "C" void UnitTests_Satellite_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& testName)
{
    DISPLAY(L"Initialize Unit-test Module, running test : " << testName);


    if (testName == L"Unit_CCW") { 
        Unit_CCW_Initialize(pPrfCom, pModuleMethodTable);    
    }
    else if (testName == L"Unit_EnumModules") { 
        Unit_EnumModules_Initialize(pPrfCom, pModuleMethodTable);    
    }
    else if (testName == L"Unit_EnumJittedFunctions") { 
        Unit_EnumJittedFunctions_Initialize(pPrfCom, pModuleMethodTable);    
    }
    else if (testName == L"Unit_EnumThreads") {
        Unit_EnumThreads_Initialize(pPrfCom, pModuleMethodTable);
    }
    else {
        FAILURE(L"Test name not recognized! " << testName);
    }

	pCallTable = pModuleMethodTable; 
	g_satellite_pPrfCom = pPrfCom;

	return;
}
