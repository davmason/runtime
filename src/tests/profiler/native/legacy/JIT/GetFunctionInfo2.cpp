// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "JITCommon.h"

class GetFunctionInfo2
{
  public:
    GetFunctionInfo2(IPrfCom *pPrfCom);
    ~GetFunctionInfo2();

    HRESULT FuncEnter2(IPrfCom *pPrfCom,
                       FunctionID mappedFuncId,
                       UINT_PTR clientData,
                       COR_PRF_FRAME_INFO func,
                       COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo);

    HRESULT FunctionEnter3WithInfo(IPrfCom *pPrfCom,
                                   FunctionIDOrClientID functionIdOrClientID,
                                   COR_PRF_ELT_INFO eltInfo);

    HRESULT FunctionLeave3WithInfo(IPrfCom *pPrfCom,
                                   FunctionIDOrClientID functionIdOrClientID,
                                   COR_PRF_ELT_INFO eltInfo);

    HRESULT FunctionTailCall3WithInfo(IPrfCom *pPrfCom,
                                      FunctionIDOrClientID functionIdOrClientID,
                                      COR_PRF_ELT_INFO eltInfo);

    static HRESULT FuncEnter2Wrapper(IPrfCom *pPrfCom,
                                     FunctionID mappedFuncId,
                                     UINT_PTR clientData,
                                     COR_PRF_FRAME_INFO func,
                                     COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo)
    {
        return Instance()->FuncEnter2(pPrfCom, mappedFuncId, clientData, func, argumentInfo);
    }

    static HRESULT FunctionEnter3WithInfoWrapper(IPrfCom *pPrfCom,
                                                 FunctionIDOrClientID functionIdOrClientID,
                                                 COR_PRF_ELT_INFO eltInfo)
    {
        return Instance()->FunctionEnter3WithInfo(pPrfCom, functionIdOrClientID, eltInfo);
    }

    static HRESULT FunctionLeave3WithInfoWrapper(IPrfCom *pPrfCom,
                                                 FunctionIDOrClientID functionIdOrClientID,
                                                 COR_PRF_ELT_INFO eltInfo)
    {
        return Instance()->FunctionLeave3WithInfo(pPrfCom, functionIdOrClientID, eltInfo);
    }

    static HRESULT FunctionTailCall3WithInfoWrapper(IPrfCom *pPrfCom,
                                                    FunctionIDOrClientID functionIdOrClientID,
                                                    COR_PRF_ELT_INFO eltInfo)
    {
        return Instance()->FunctionTailCall3WithInfo(pPrfCom, functionIdOrClientID, eltInfo);
    }

    HRESULT FuncTailcall2(IPrfCom *pPrfCom,
                          FunctionID mappedFuncId,
                          UINT_PTR clientData,
                          COR_PRF_FRAME_INFO func);

    static HRESULT FuncTailcall2Wrapper(IPrfCom *pPrfCom,
                                        FunctionID mappedFuncId,
                                        UINT_PTR clientData,
                                        COR_PRF_FRAME_INFO func)
    {
        return Instance()->FuncTailcall2(pPrfCom, mappedFuncId, clientData, func);
    }

    HRESULT FuncLeave2(IPrfCom *pPrfCom,
                       FunctionID mappedFuncId,
                       UINT_PTR clientData,
                       COR_PRF_FRAME_INFO func,
                       COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange);

    static HRESULT FuncLeave2Wrapper(IPrfCom *pPrfCom,
                                     FunctionID mappedFuncId,
                                     UINT_PTR clientData,
                                     COR_PRF_FRAME_INFO func,
                                     COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange)
    {
        return Instance()->FuncLeave2(pPrfCom, mappedFuncId, clientData, func, retvalRange);
    }

    HRESULT Verify(IPrfCom *pPrfCom);

    static GetFunctionInfo2 *Instance()
    {
        return This;
    }

    static GetFunctionInfo2 *This;
};

GetFunctionInfo2 *GetFunctionInfo2::This = NULL;

GetFunctionInfo2::GetFunctionInfo2(IPrfCom * /*pPrfCom*/)
{
    // Static this pointer
    GetFunctionInfo2::This = this;
}

GetFunctionInfo2::~GetFunctionInfo2()
{
    // Static this pointer
    GetFunctionInfo2::This = NULL;
}

HRESULT GetFunctionInfo2::FuncEnter2(IPrfCom *pPrfCom,
                                     FunctionID funcId,
                                     UINT_PTR /*clientData*/,
                                     COR_PRF_FRAME_INFO func,
                                     COR_PRF_FUNCTION_ARGUMENT_INFO * /*argumentInfo*/)
{
    WCHAR_STR(funcName);
    PPRFCOM->GetFunctionIDName(funcId, funcName, func);

    if (funcName.find(L"TestFunction") == string::npos)
    {
        return S_OK;
    }

    DISPLAY(L"\nFunctionID 0x" << funcId << L" - " << funcName << L"\n");

    ModuleID modID;
    ClassID classID;
    mdToken token;
    ClassID typeArgs[SHORT_LENGTH];
    ULONG32 nTypeArgs;

    PINFO->GetFunctionInfo2(
        funcId,
        func,
        &classID,
        &modID,
        &token,
        SHORT_LENGTH,
        &nTypeArgs,
        typeArgs);

    DISPLAY(L"ClassID 0x" << classID << L", ModuleID 0x" << modID << L", MD Token 0x" << token << L"\n");
    DISPLAY(L"Number of Type Args = " << nTypeArgs << L"\n");
    for (ULONG32 i = 0; i < nTypeArgs; i++)
    {
        WCHAR_STR(className);
        PPRFCOM->GetClassIDName(typeArgs[(INT)i], className);
        DISPLAY(L"\t Arg " << i << L" - ClassID 0x" << typeArgs[(INT)i] << L" - " << className << L"%s\n");

        if (typeArgs[(INT)i] == 0)
        {
            FAILURE(L"Null class ID returned as a Type Arg for a generic function.\n");
        }
    }
    return S_OK;
}

HRESULT GetFunctionInfo2::FunctionEnter3WithInfo(IPrfCom *pPrfCom, FunctionIDOrClientID functionIdOrClientID, COR_PRF_ELT_INFO eltInfo)
{
    COR_PRF_FRAME_INFO frameInfo;
    ULONG pcbArgumentInfo = sizeof(COR_PRF_FUNCTION_ARGUMENT_INFO);
    COR_PRF_FUNCTION_ARGUMENT_INFO *pArgumentInfo = new COR_PRF_FUNCTION_ARGUMENT_INFO;
    HRESULT hr = pPrfCom->m_pInfo->GetFunctionEnter3Info(functionIdOrClientID.functionID, eltInfo, &frameInfo, &pcbArgumentInfo, pArgumentInfo);
    if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) && hr != S_OK)
    {
	    delete pArgumentInfo;

        FAILURE(L"\nInvalid HR received at GetFunctionInfo2::FunctionEnter3WithInfo");
        return hr;
    }
    else if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
    {
        delete pArgumentInfo;

        pArgumentInfo = reinterpret_cast<COR_PRF_FUNCTION_ARGUMENT_INFO *>(new BYTE[pcbArgumentInfo]);
        hr = pPrfCom->m_pInfo->GetFunctionEnter3Info(functionIdOrClientID.functionID, eltInfo, &frameInfo, &pcbArgumentInfo, pArgumentInfo);
        if (hr != S_OK)
        {
            delete[](BYTE *) pArgumentInfo;
            return E_FAIL;
        }
    }
    ModuleID modID;
    ClassID classID;
    mdToken token;
    ClassID typeArgs[SHORT_LENGTH];
    ULONG32 nTypeArgs;
    PINFO->GetFunctionInfo2(functionIdOrClientID.functionID, frameInfo, &classID, &modID, &token, SHORT_LENGTH, &nTypeArgs, typeArgs);
    DISPLAY(L"ClassID 0x" << classID << L", ModuleID 0x" << modID << L", MD Token 0x" << token << L"\n");
    DISPLAY(L"Number of Type Args = " << nTypeArgs << L"\n");
    for (ULONG32 i = 0; i < nTypeArgs; i++)
    {
        WCHAR_STR(className);
        PPRFCOM->GetClassIDName(typeArgs[(INT)i], className);
        DISPLAY(L"\t Arg " << i << L" - ClassID 0x" << typeArgs[(INT)i] << L" - " << className.c_str() << L"\n");

        if (typeArgs[(INT)i] == 0)
        {
            FAILURE(L"Null class ID returned as a Type Arg for a generic function.\n");
        }
    }
    return S_OK;
}

HRESULT GetFunctionInfo2::FunctionLeave3WithInfo(IPrfCom * /*pPrfCom*/, FunctionIDOrClientID /*functionIdOrClientID*/, COR_PRF_ELT_INFO /*eltInfo*/)
{
    return S_OK;
}

HRESULT GetFunctionInfo2::FunctionTailCall3WithInfo(IPrfCom * /*pPrfCom*/, FunctionIDOrClientID /*functionIdOrClientID*/, COR_PRF_ELT_INFO /*eltInfo*/)
{
    return S_OK;
}

HRESULT GetFunctionInfo2::FuncTailcall2(IPrfCom * /*pPrfCom*/,
                                        FunctionID /*mappedFuncId*/,
                                        UINT_PTR /*clientData*/,
                                        COR_PRF_FRAME_INFO /*func*/)
{
    return S_OK;
}

HRESULT GetFunctionInfo2::FuncLeave2(IPrfCom * /*pPrfCom*/,
                                     FunctionID /*mappedFuncId*/,
                                     UINT_PTR /*clientData*/,
                                     COR_PRF_FRAME_INFO /*func*/,
                                     COR_PRF_FUNCTION_ARGUMENT_RANGE * /*retvalRange*/)
{
    return S_OK;
}

HRESULT GetFunctionInfo2::Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"Test Passed!\n");

    return S_OK;
}

/*
 *  Straight function callback used by TestProfiler
 */

GetFunctionInfo2 *global_GetFunctionInfo2;

HRESULT GetFunctionInfo2_Verify(IPrfCom *pPrfCom)
{
    HRESULT hr = GetFunctionInfo2::Instance()->Verify(pPrfCom);

    delete global_GetFunctionInfo2;

    return hr;
}

void GetFunctionInfo2_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, bool ELT3)
{
    DISPLAY(L"Profiler API Test: GetFunctionInfo2 API\n");

    // Create new test object
    global_GetFunctionInfo2 = new GetFunctionInfo2(pPrfCom);

    pModuleMethodTable->FLAGS =
        COR_PRF_MONITOR_ENTERLEAVE |
        COR_PRF_ENABLE_FUNCTION_ARGS |
        COR_PRF_ENABLE_FUNCTION_RETVAL |
        COR_PRF_ENABLE_FRAME_INFO;

    pModuleMethodTable->VERIFY = (FC_VERIFY)&GetFunctionInfo2_Verify;
    if (!ELT3)
    {
        pModuleMethodTable->FUNCTIONENTER2 = (FC_FUNCTIONENTER2)&GetFunctionInfo2::FuncEnter2Wrapper;
        pModuleMethodTable->FUNCTIONLEAVE2 = (FC_FUNCTIONLEAVE2)&GetFunctionInfo2::FuncLeave2Wrapper;
        pModuleMethodTable->FUNCTIONTAILCALL2 = (FC_FUNCTIONTAILCALL2)&GetFunctionInfo2::FuncTailcall2Wrapper;
    }
    else
    {
        pModuleMethodTable->FUNCTIONENTER3WITHINFO = (FC_FUNCTIONENTER3WITHINFO)&GetFunctionInfo2::FunctionEnter3WithInfoWrapper;
        pModuleMethodTable->FUNCTIONLEAVE3WITHINFO = (FC_FUNCTIONLEAVE3WITHINFO)&GetFunctionInfo2::FunctionLeave3WithInfoWrapper;
        pModuleMethodTable->FUNCTIONTAILCALL3WITHINFO = (FC_FUNCTIONTAILCALL3WITHINFO)&GetFunctionInfo2::FunctionTailCall3WithInfoWrapper;
    }
    return;
}
