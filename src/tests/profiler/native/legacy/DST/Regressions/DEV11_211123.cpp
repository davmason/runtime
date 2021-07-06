// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "../../ProfilerCommon.h"
#include "../../LegacyCompat.h" // Whidbey Framework Support

struct OneTwoThreeFour
{
    int one;
    int two;
    int three;
    int four;
};

HRESULT DEV11_211123_FuncEnter2(IPrfCom * pPrfCom,
                                       FunctionID mappedFuncId,
                                       UINT_PTR clientData,
                                       COR_PRF_FRAME_INFO func,
                                       COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo)
{
    HRESULT hr = S_OK;

    wstring funcName;
    PPRFCOM->GetFunctionIDName(mappedFuncId, funcName, func, true);

    if (funcName.find(L"My::foo") == string::npos)
    {
        return hr;
    }

    for (ULONG i = 0; i < argumentInfo->numRanges; i++)
    {
        if (argumentInfo->ranges[i].length != sizeof(OneTwoThreeFour))
        {
            FAILURE(L"Invalid length of argument " << i << ": " << argumentInfo->ranges[i].length << L"\n");
            return E_FAIL;
        }

        OneTwoThreeFour *pStruct = (OneTwoThreeFour *)argumentInfo->ranges[i].startAddress;
        if (pStruct->one != 1 || pStruct->two != 2 || pStruct->three != 3 || pStruct->four != 4)
        {
            FAILURE(L"Invalid value of argument " << i << "\n");
            return E_FAIL;
        }
    }

    return hr;
}

HRESULT DEV11_211123_Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"TEST PASSED");
    return S_OK;
}

void DEV11_211123_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY( L"Initialize DEV11_211123 extension\n" );

    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_ENTERLEAVE    | 
                                COR_PRF_ENABLE_FUNCTION_ARGS  |
                                COR_PRF_DISABLE_INLINING      |
                                COR_PRF_DISABLE_OPTIMIZATIONS ;

    pModuleMethodTable->VERIFY = (FC_VERIFY) &DEV11_211123_Verify;
    pModuleMethodTable->FUNCTIONENTER2 = (FC_FUNCTIONENTER2) &DEV11_211123_FuncEnter2;
    
    return;
}




