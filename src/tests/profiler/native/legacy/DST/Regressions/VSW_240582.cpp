// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "../../ProfilerCommon.h"
#include "../../LegacyCompat.h" // Whidbey Framework Support

HRESULT VSW240582_FuncEnter2(IPrfCom * pPrfCom,
                 FunctionID mappedFuncId,
                 UINT_PTR clientData,
                 COR_PRF_FRAME_INFO func,
                 COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo)
{
    HRESULT hr = S_OK;

    ClassID classId;
    ModuleID moduleId;
    mdToken token;
    ULONG32 nTypeArgs;
    ClassID typeArgs[SHORT_LENGTH];

    hr = pPrfCom->m_pInfo->GetFunctionInfo2(mappedFuncId,
                                    func,
                                        &classId,
                                        &moduleId,
                                        &token,
                                        SHORT_LENGTH,
                                        &nTypeArgs,
                                        typeArgs);
    if (hr != S_OK)
    {
		FAILURE(L"GetFunctionInfo2 returned 0x" << HEX(hr) << L"\n");
        return hr;
    }

    WCHAR_STR( name );
    pPrfCom->GetClassIDName(classId, name);

    if ( name.find(L"B!1") != string::npos || name.find(L"A!1") != string::npos)
    {
        DISPLAY(name.c_str());

        ClassID classId2 = 5;
        ModuleID moduleId2 = 5;
        mdToken token2;
        hr = pPrfCom->m_pInfo->GetFunctionInfo(mappedFuncId,
                                           &classId2,
                                           &moduleId2,
                                           &token2);

        if (classId2 != 0)
        {
            FAILURE(name.c_str() << L" ClassID 0x" << HEX(classId2) << L"\n");
        }
    }

    return hr;
}



HRESULT VSW240582_Verify(IPrfCom * pPrfCom)
{
    DISPLAY( L"Verify VSW240582\n" );

    return S_OK;

}

void VSW_240582_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY( L"Initialize VSW240582 extension\n" );

    pModuleMethodTable->FLAGS =     COR_PRF_MONITOR_ENTERLEAVE |
                                COR_PRF_ENABLE_FUNCTION_ARGS |
                                COR_PRF_ENABLE_FUNCTION_RETVAL |
                                COR_PRF_ENABLE_FRAME_INFO;

    pModuleMethodTable->VERIFY = (FC_VERIFY) &VSW240582_Verify;
    pModuleMethodTable->FUNCTIONENTER2 = (FC_FUNCTIONENTER2) &VSW240582_FuncEnter2;


    return;
}



