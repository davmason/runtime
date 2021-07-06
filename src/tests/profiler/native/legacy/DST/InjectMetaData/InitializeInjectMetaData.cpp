// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "stdafx.h"

// For some reason this is what's called on desktop...
extern "C" void InjectMetaData_Satellite_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& testName)
{
    InjectMetaData::Initialize(pPrfCom, pModuleMethodTable, testName);
}

// This is used for ModuleRef/PInvoke testing
extern "C" int NativeFunction()
{
    return g_pInjectMD->NativeFunction();
}