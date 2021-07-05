#include "../../ProfilerCommon.h"
#include "ReJITScript.h"


extern "C" void __cdecl NtvEnteredFunction(
    ModuleID moduleIDCur,
    mdMethodDef mdCur,
    int nVersionCur);

// For some reason this is what's called on desktop...
extern "C" void Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& testName)
{
    DISPLAY(L"ReJIT Test Module : " << testName);

    ReJITScript::Initialize(pPrfCom, pModuleMethodTable, testName);

    // 
    printf("Address of NtvEnteredFunction is %p \r\n", NtvEnteredFunction);
}

extern "C" void ReJIT_Satellite_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& testName)
{
	DISPLAY("Initializes ReJIT module");
    Initialize(pPrfCom, pModuleMethodTable, testName);
}
