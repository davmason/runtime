// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "../../ProfilerCommon.h"
#include "../../LegacyCompat.h" // Whidbey Framework Support

HRESULT VSW455322_hr;

HRESULT VSW455322_FuncEnter2(IPrfCom * pPrfCom,
                                       FunctionID mappedFuncId,
                                       UINT_PTR clientData,
                                       COR_PRF_FRAME_INFO func,
                                       COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo)
{
    HRESULT hr = S_OK;

    WCHAR_STR( funcName );
    PPRFCOM->GetFunctionIDName(mappedFuncId, funcName, func, true);

    if ((funcName.find(L"System.Threading.Thread::Join") != string::npos)
        || funcName.find(L"Internal.Runtime.Augments.RuntimeThread::Join") != string::npos)
    {
        VSW455322_hr = S_OK;
    }

    return hr;
}



HRESULT VSW455322_Verify(IPrfCom * pPrfCom)
{
    if (VSW455322_hr == E_FAIL)
    {
        DISPLAY( L"VSW455322 Test Failed.\n" );
    }
    else
    {
        DISPLAY( L"VSW455322 Test Passed.\n" );
    }

    return VSW455322_hr;

}

void VSW_455322_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY( L"Initialize VSW455322 extension\n" );

    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_ENTERLEAVE    | 
                                COR_PRF_ENABLE_FUNCTION_ARGS  |
                                COR_PRF_DISABLE_INLINING      |
                                COR_PRF_DISABLE_OPTIMIZATIONS ;

    pModuleMethodTable->VERIFY = (FC_VERIFY) &VSW455322_Verify;
    pModuleMethodTable->FUNCTIONENTER2 = (FC_FUNCTIONENTER2) &VSW455322_FuncEnter2;

    VSW455322_hr = E_FAIL;

    return;
}
