#include "../../ProfilerCommon.h"

INT VSW256310_fail;
INT VSW256310_pass;

HRESULT VSW256310_FuncEnter2(IPrfCom * pPrfCom,
                             FunctionID functionId,
                             UINT_PTR clientData,
                             COR_PRF_FRAME_INFO func,
                             COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo)
{
    HRESULT hr = S_OK;

    ClassID classId   = NULL;
    ModuleID moduleId = NULL;
    mdToken token     = NULL;
    ULONG32 nTypeArgs = NULL;
    ClassID typeArgs[SHORT_LENGTH];

    MUST_PASS(PINFO->GetFunctionInfo2(functionId, func, &classId, &moduleId, &token, SHORT_LENGTH, &nTypeArgs, typeArgs));

    if (classId == 0)
    {
        FAILURE(L"GetFunctionInfo2 returned a NULL ClassID for FunctionID " << HEX(functionId));
    }

    COR_FIELD_OFFSET corFieldOffset[SHORT_LENGTH];
    ULONG fieldOffset = NULL;
    ULONG classSize   = NULL;

    WCHAR_STR( name );
    PPRFCOM->GetClassIDName(classId, name);

    hr = PINFO->GetClassLayout(classId, corFieldOffset, SHORT_LENGTH, &fieldOffset, &classSize);
    if (FAILED(hr))
    {
#ifdef WINDOWS
         if ( _wcsicmp(name.c_str(), L"System.String")==0 )
#else
         if (CompareCaseInsensitiveWString(name.c_str(), L"System.String")==0 )
#endif
         {
            //Breaking change in v4, GetClassLayout returns error for string class.
            return S_OK;
         }
         else
         {
            FAILURE(L"GetClassLayout returned "<< HEX(hr) << L" for class " << name );
         }
    }
    
    if (name.find(L"A`1") != string::npos ||
        name.find(L"B`1") != string::npos ||
        name.find(L"C`2") != string::npos ||
        name.find(L"D`3") != string::npos ||
        name.find(L"E`1") != string::npos ||
        name.find(L"F`2") != string::npos ||
        name.find(L"Z")   != string::npos)
    {
        // DISPLAY(name.c_str() << L" ClassID " << HEX(classId));
        for (ULONG i = 0; i < fieldOffset; i++)
        {
            // DISPLAY(L"Offset " << HEX(corFieldOffset[i].ulOffset) << L" for Token " << HEX(corFieldOffset[i].ridOfField));
            if (corFieldOffset[i].ulOffset == 0)
            {
                VSW256310_fail++;
                FAILURE(L"Reference type has a field offset of 0!\n" << name.c_str() << L" ClassID " << HEX(classId) << L" Offset " << HEX(corFieldOffset[i].ulOffset))
            }
            else
            {
                VSW256310_pass++;
            }
        }
    }

    return hr;
}



HRESULT VSW256310_Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"Verify VSW256310");

    if (VSW256310_fail != 0 || VSW256310_pass == 0)
    {
        FAILURE(HEX(VSW256310_fail) << L" Classes with offset 0 found.\n" << HEX(VSW256310_pass) << L" successes.\nTEST FAILED.");
        return E_FAIL;
    }

    return S_OK;

}

void VSW_256310_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"\nVSWhidbey 256310\n\nThe field offsets reported by ICPInfo2::GetClassLayout are off by "
            << L"the size of the MethodTable pointer for reference types. There should be no field "
            << L"with offset 0, since the MT pointer is at offset 0.\n");
    
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_ENTERLEAVE |
                                COR_PRF_ENABLE_FUNCTION_ARGS |
                                COR_PRF_ENABLE_FRAME_INFO;

    REGISTER_CALLBACK(VERIFY, VSW256310_Verify);
    REGISTER_CALLBACK(FUNCTIONENTER2, VSW256310_FuncEnter2);

    VSW256310_fail = 0;
    VSW256310_pass = 0;

    return;
}


