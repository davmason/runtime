// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// VSW_589248.cpp
//
// 
// 
// ======================================================================================

#include "ProfilerCommon.h"

HRESULT VSW_589248_Verify(IPrfCom * /*pPrfCom*/)
{
    return S_OK;
}


/*
 *  Initialization routine called by TestProfiler
 */
void VSW_589248_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize VSW589248.\n")

    // Initialize MethodTable
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_JIT_COMPILATION  |
                                COR_PRF_MONITOR_CODE_TRANSITIONS ;

    REGISTER_CALLBACK(VERIFY, VSW_589248_Verify);

    return;
}

