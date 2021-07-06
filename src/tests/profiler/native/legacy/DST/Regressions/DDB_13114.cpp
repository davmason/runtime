// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// DDB_13114.cpp
//
// 
// 
// ======================================================================================

#include "../../ProfilerCommon.h"

HRESULT DDB_13114_Verify(IPrfCom * /*pPrfCom*/)
{
    return S_OK;
}


/*
 *  Initialization routine called by TestProfiler
 */
void DDB_13114_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize DDB13114.\n")

    // Initialize MethodTable
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_CCW;

    REGISTER_CALLBACK(VERIFY, DDB_13114_Verify);

    return;
}

