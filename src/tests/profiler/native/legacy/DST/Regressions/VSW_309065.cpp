// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "../../ProfilerCommon.h"
#include "../../LegacyCompat.h" // Whidbey Framework Support

#if defined(WINDOWS) && !defined(OSX)
#include <malloc.h>
#endif

/*******************************************************************************
 *
 *  This test serves as a regression case for two similar bugs.
 *
 *      VSWhidbey 309065 - This pointers were not being reported in FunctionEnter2
 *
 *      VSWhidbey 505008 - StartAddress of this pointers reported as NULL in FunctionEnter2
 *
 *  The test registers for the FunctionEnter2 callback, and requestes NGen'd images so all
 *  FunctionEnter2 events will be delivered.
 *
 *  For each FunctionEnter2, it verifies that if a method has no arguments that it is a static
 *  method.  Every non-static method should have at least one argument, the this pointer.
 *
 *  It also verifies that for each function argument, the StartAddress and Length are non Null.
 *
 *  Finally it verifies that the memory pointed to by the startAddress can be read up to the
 *  reported length using IsBadReadPtr(...)
 *
 ******************************************************************************/

class VSW309065
{
public:

    VSW309065()
    {
        VSW309065::This = this;
        m_success = m_failure = 0;
    }

    static HRESULT FuncEnter2Wrapper(
                    IPrfCom *pPrfCom,
                    FunctionID funcId,
                    UINT_PTR clientData,
                    COR_PRF_FRAME_INFO frameInfo,
                    COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo)
    {
        return Instance()->FuncEnter2(pPrfCom, funcId, clientData, frameInfo, argumentInfo);
    }

    HRESULT Verify(IPrfCom * pPrfCom);

    static VSW309065* Instance()
    {
        return This;
    }

private:

    HRESULT FuncEnter2(
                    IPrfCom *pPrfCom,
                    FunctionID funcId,
                    UINT_PTR clientData,
                    COR_PRF_FRAME_INFO frameInfo,
                    COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo);

    static VSW309065* This;

    ULONG m_success;
    ULONG m_failure;

};

VSW309065* VSW309065::This = NULL;

VSW309065* global_VSW309065 = NULL;

HRESULT VSW309065::FuncEnter2(
                                    IPrfCom *pPrfCom,
                                    FunctionID funcId,
                                    UINT_PTR clientData,
                                    COR_PRF_FRAME_INFO frameInfo,
                                    COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo)
{
    HRESULT hr = S_OK;

    BOOL isStatic = FALSE;
    wstring name;

    if ((funcId == 0) || (argumentInfo == 0))
    {
        FAILURE(L"Invalid Arg in FuncEnter2 : FunctionID = 0x" << HEX(funcId) << ", ArgumentInfo = 0x" << HEX(argumentInfo) << "\n");
        return E_FAIL;
    }

    isStatic = PPRFCOM->IsFunctionStatic(funcId, frameInfo);

    // Verify that the only functions with 0 ranges are static (VSWhidbey 309065)
    if (argumentInfo->numRanges == 0)
    {
        if (isStatic == FALSE)
        {
            pPrfCom->GetFunctionIDName(funcId, name);
            FAILURE(name.c_str() << " is not a static function and does not have a this pointer!\n");
            m_failure++;
            hr =  E_FAIL;
        }
    }
    // Else, if we have ranges, verify that no range has a NULL start address or NULL length (VSWhidbey 505008)
    else
    {
        for (ULONG i = 0; i < argumentInfo->numRanges; i++)
        {
            // Verify startAddress
            if (argumentInfo->ranges[i].startAddress == 0)
            {
                pPrfCom->GetFunctionIDName(funcId, name);

                if ((isStatic == FALSE) && (i == 0))
                {
                    FAILURE(L"Bad This Pointer - " << name.c_str() << " : ArgumentInfo->ranges[" << i << "].startAddress == NULL\n");
                }
                else
                {
                    FAILURE(name.c_str() << " : ArgumentInfo->ranges[" << i << "].startAddress == NULL\n");
                }

                m_failure++;
                hr = E_FAIL;
            }

            // Verify length
            if (argumentInfo->ranges[i].length == 0)
            {
                name = L"";
                pPrfCom->GetFunctionIDName(funcId, name);
                FAILURE(name.c_str() << " : ArgumentInfo->ranges[" << i << "].length == NULL\n");
                m_failure++;
                hr = E_FAIL;
            }

            // Make sure what the range points to are valid arguments (on the stack).
            ValidatePtr((PVOID)argumentInfo->ranges[i].startAddress, argumentInfo->ranges[i].length);
        }
    }

    // If we make it this far, we have achieved success for this function.
    if (hr == S_OK)
    {
        m_success++;
    }

    return hr;
}

HRESULT VSW309065::Verify(IPrfCom * pPrfCom)
{
    if ((m_success > 0) && (m_failure == 0))
    {
        DISPLAY( L"\nVSW309065 Passed.\n\n" );
        return S_OK;
    }
    else
    {
        DISPLAY(L"\nVSW309065 Failed.  Success " << m_success << L", Failure " << m_failure << L"\n\n");
        return E_FAIL;
    }
}

HRESULT VSW309065_Verify(IPrfCom * pPrfCom)
{
    HRESULT hr = VSW309065::Instance()->Verify(pPrfCom);

    delete global_VSW309065;

    return hr;
}

void VSW_309065_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize VSW309065 extension\n" );

    global_VSW309065 = new VSW309065();

    pModuleMethodTable->FLAGS =  COR_PRF_MONITOR_ENTERLEAVE |
                                                    COR_PRF_ENABLE_FUNCTION_ARGS |
                                                    COR_PRF_USE_PROFILE_IMAGES;

    pModuleMethodTable->VERIFY = (FC_VERIFY) &VSW309065_Verify;
    pModuleMethodTable->FUNCTIONENTER2 = (FC_FUNCTIONENTER2) &VSW309065::FuncEnter2Wrapper;

    return;
}
