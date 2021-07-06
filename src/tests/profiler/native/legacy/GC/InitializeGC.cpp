// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "GCCommon.h"
extern PMODULEMETHODTABLE pCallTable; 
extern IPrfCom * g_satellite_pPrfCom;

#pragma region legacy_includes

    #ifndef _INCLUDE_LEGACY_FUNCTIONS_
    #define _INCLUDE_LEGACY_FUNCTIONS_
    #include "../LegacyCompat.h"
    extern IPrfCom* g_pPrfCom_LegacyCompat;
    #endif

#pragma endregion // legacy_includes

void GCAttach_Initialize(IGCCom * pGCCom, PMODULEMETHODTABLE pModuleMethodTable);
void GC_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
void EnumObjectsInitialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
void GCStatics_Initialize(IGCCom * pGCCom, PMODULEMETHODTABLE pModuleMethodTable);

extern "C" void GC_Satellite_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& testName)
{
    DISPLAY(L"Initialize GC : " << testName);

    GCCommon * pGCCommon = new GCCommon(pPrfCom->m_pInfo);
    SET_DERIVED_POINTER(pGCCommon);

#pragma region legacy_pointer

    g_pPrfCom_LegacyCompat = pModuleMethodTable->IPPRFCOM;

#pragma endregion // legacy_pointer



         if (testName == L"GCCallbacks") GC_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"GCAttach")    GCAttach_Initialize(pGCCommon, pModuleMethodTable);
    else if (testName == L"EnumObjects") EnumObjectsInitialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"GCStatics") GCStatics_Initialize(pGCCommon, pModuleMethodTable);
    else FAILURE(L"Test name not recognized!");


	pCallTable = pModuleMethodTable; 
	g_satellite_pPrfCom = pGCCommon;

	return;
}
