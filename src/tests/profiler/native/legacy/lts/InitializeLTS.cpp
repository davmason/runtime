#include "LTSCommon.h"
#include "InspectArgAndRetVal.h"
extern PMODULEMETHODTABLE pCallTable; 
extern IPrfCom * g_satellite_pPrfCom;

#ifndef _INCLUDE_LEGACY_FUNCTIONS_
#define _INCLUDE_LEGACY_FUNCTIONS_
#include "../LegacyCompat.h"
extern IPrfCom * g_pPrfCom_LegacyCompat;
#endif 

// Test Initialization Function Declarations
void GetClassRoundTrip_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
void Modules_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, BOOL);
void GenericsInspection_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
void InspectObjectID_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
void DynamicModules_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
void GetAppdomains_Initialize(ILTSCom * pLTSCom, PMODULEMETHODTABLE pModuleMethodTable);
void CallSequence_Initialize(ILTSCom * pLTSCom, PMODULEMETHODTABLE pModuleMethodTable);
void InspectArgAndRetVal_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, InspectionTestFlags testFlags);


extern "C" void LTS_Satellite_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& testName)
{
    pPrfCom->FreeOutputFiles(TRUE);
    LTSCommon * pLTSCommon = new LTSCommon(pPrfCom->m_pInfo);
    SET_DERIVED_POINTER(pLTSCommon);

    // Legacy support
    g_pPrfCom_LegacyCompat = pLTSCommon;

    DISPLAY(L"Loader Type System Initialize : " << testName);

         if (testName == L"Modules")            Modules_Initialize(pLTSCommon, pModuleMethodTable, FALSE);
    else if (testName == L"ModuleEnums")        Modules_Initialize(pLTSCommon, pModuleMethodTable, TRUE);
    else if (testName == L"DynamicModules")     DynamicModules_Initialize(pLTSCommon, pModuleMethodTable);
    else if (testName == L"GetAppdomains")      GetAppdomains_Initialize(pLTSCommon, pModuleMethodTable);
    else if (testName == L"CallSequence")       CallSequence_Initialize(pLTSCommon, pModuleMethodTable);
    else if (testName == L"GetClassRoundTrip")  GetClassRoundTrip_Initialize(pLTSCommon, pModuleMethodTable);
    else if (testName == L"GenericsInspection") GenericsInspection_Initialize(pLTSCommon, pModuleMethodTable);
    else if (testName == L"InspectObjectIDs")   InspectObjectID_Initialize(pLTSCommon, pModuleMethodTable);

    // Object Inspection Unit Tests
    else if (testName == L"UnitArgInt")                 InspectArgAndRetVal_Initialize(pLTSCommon, pModuleMethodTable, UNIT_TEST_ARG_INT);
    else if (testName == L"UnitArgClass")               InspectArgAndRetVal_Initialize(pLTSCommon, pModuleMethodTable, UNIT_TEST_ARG_CLASS);
    else if (testName == L"UnitArgClassStatic")         InspectArgAndRetVal_Initialize(pLTSCommon, pModuleMethodTable, UNIT_TEST_ARG_CLASS_STATIC);
    else if (testName == L"UnitArgStruct")              InspectArgAndRetVal_Initialize(pLTSCommon, pModuleMethodTable, UNIT_TEST_ARG_STRUCT);
    else if (testName == L"UnitArgStructStatic")        InspectArgAndRetVal_Initialize(pLTSCommon, pModuleMethodTable, UNIT_TEST_ARG_STRUCT_STATIC);
    else if (testName == L"UnitArgComplexStatic")       InspectArgAndRetVal_Initialize(pLTSCommon, pModuleMethodTable, UNIT_TEST_ARG_COMPLEX_STATIC);
    else if (testName == L"UnitArgGenericClassInt")     InspectArgAndRetVal_Initialize(pLTSCommon, pModuleMethodTable, UNIT_TEST_ARG_GENERIC_CLASS_INT);
    else if (testName == L"UnitArgGenericMethodInt")    InspectArgAndRetVal_Initialize(pLTSCommon, pModuleMethodTable, UNIT_TEST_ARG_GENERIC_METHOD_INT);
    else if (testName == L"UnitRetValInt")              InspectArgAndRetVal_Initialize(pLTSCommon, pModuleMethodTable, UNIT_TEST_RETVAL_INT);
    else if (testName == L"UnitRetValClass")            InspectArgAndRetVal_Initialize(pLTSCommon, pModuleMethodTable, UNIT_TEST_RETVAL_CLASS);
    else if (testName == L"UnitRetValClassStatic")      InspectArgAndRetVal_Initialize(pLTSCommon, pModuleMethodTable, UNIT_TEST_RETVAL_CLASS_STATIC);
    else if (testName == L"UnitRetValStruct")           InspectArgAndRetVal_Initialize(pLTSCommon, pModuleMethodTable, UNIT_TEST_RETVAL_STRUCT);
    else if (testName == L"UnitRetValStructStatic")     InspectArgAndRetVal_Initialize(pLTSCommon, pModuleMethodTable, UNIT_TEST_RETVAL_STRUCT_STATIC);
    else if (testName == L"UnitRetValComplexStatic")    InspectArgAndRetVal_Initialize(pLTSCommon, pModuleMethodTable, UNIT_TEST_RETVAL_COMPLEX_STATIC);
    else if (testName == L"UnitRetValGenericClassInt")  InspectArgAndRetVal_Initialize(pLTSCommon, pModuleMethodTable, UNIT_TEST_RETVAL_GENERIC_CLASS_INT);
    else if (testName == L"UnitRetValGenericMethodInt") InspectArgAndRetVal_Initialize(pLTSCommon, pModuleMethodTable, UNIT_TEST_RETVAL_GENERIC_METHOD_INT);

    else FAILURE(L"Test name [" << testName << L"] not recognized!");

	pCallTable = pModuleMethodTable; 
	g_satellite_pPrfCom = pLTSCommon;

	return;
}
