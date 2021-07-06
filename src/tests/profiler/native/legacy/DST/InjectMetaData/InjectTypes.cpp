// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "stdafx.h"

using namespace std;

InjectTypes::InjectTypes(IPrfCom * pPrfCom, TestType testType)
    : InjectMetaData(pPrfCom,testType)
{
}

InjectTypes::~InjectTypes()
{
}

HRESULT InjectTypes::Verify()
{
    TEST_DISPLAY(L"Verify called for InjectTypes.");
    if (m_ulRejitCallCount == 1)
    {
        // This test expects REJIT to get called once.
        return S_OK;
    }
    else
    {
        TEST_FAILURE(L"InjectTypes::Verify failed. Expected Rejit Count:1 , Current Rejit Count:" << m_ulRejitCallCount);
        return E_FAIL;
    }
}

HRESULT InjectTypes::GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl)
{
    HRESULT hr;
    
    vector<const PLATFORM_WCHAR*> types;
    types.push_back(MDMASTER_TYPE);
    types.push_back(MDSOURCESIMPLECLASS);

    InjectAssembly(moduleId, types);
    hr = m_pProfilerInfo->ApplyMetaData(moduleId);
    if (FAILED(hr))
    {
        FAILURE(L"ApplyMetaData failed.    Return code: " << HEX(hr));
    }

    mdMethodDef tokTargetMethodToCall;
    hr = GetTestMethod(PROFILER_STRING("MDMaster"), PROFILER_STRING("Verify_InjectTypes"), &tokTargetMethodToCall);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"RewriteIL failed to get methoddef.    Return code: " << HEX(hr));
        return hr;
    }

    RewriteIL(pFunctionControl, moduleId, methodId, tokTargetMethodToCall);
    return S_OK;
}

HRESULT InjectTypes::GetTestMethod(LPCWSTR pwzTypeName, LPCWSTR pwzMethodName, mdMethodDef* ptokMethod)
{
    *ptokMethod = mdMemberRefNil;

    HRESULT hr;
    mdTypeDef tokTypeDef;
    hr = m_pTargetImport->FindTypeDefByName(pwzTypeName, NULL, &tokTypeDef);
    if (FAILED(hr))
    {
        FAILURE(L"GetTestMethod failed to get the type: " << pwzTypeName << ".    Return code: " << HEX(hr));
        return hr;
    }

    HCORENUM hEnum = NULL;
    mdMethodDef tokMethodDef = mdMethodDefNil;
    ULONG cReceived;
    hr = m_pTargetImport->EnumMethodsWithName(&hEnum, tokTypeDef, pwzMethodName, &tokMethodDef, 1, &cReceived);
    if (FAILED(hr) || cReceived != 1)
    {
        FAILURE(L"GetTestMethod failed to get method: " << pwzMethodName << ".    Return code: " << HEX(hr));
        return hr;
    }
    m_pTargetImport->CloseEnum(hEnum);

    *ptokMethod = tokMethodDef;

    return S_OK;
}

HRESULT InjectTypes::RewriteIL(
    ICorProfilerFunctionControl * pICorProfilerFunctionControl,
    ModuleID moduleID,
    mdMethodDef methodDef,
    mdMethodDef methodToCall)
{
    ILRewriter rewriter(m_pProfilerInfo, pICorProfilerFunctionControl, moduleID, methodDef);

    IfFailRet(rewriter.Initialize());
    IfFailRet(rewriter.Import());

    ILInstr * pInstr = rewriter.GetILList()->m_pNext;

    while (pInstr != NULL && pInstr->m_opcode != CEE_CALL)
    {
        pInstr = pInstr->m_pNext;
    }

    if (pInstr != NULL)
    {
        pInstr->m_Arg32 = methodToCall;
    }
    else
    {
        TEST_FAILURE("CALL opcode was not found.")
    }

    IfFailRet(rewriter.Export());

    return S_OK;
}

void InjectTypes::InjectAssembly(ModuleID targetModuleId, const vector<const PLATFORM_WCHAR*> &types)
{
    HRESULT hr;
    HCORENUM hEnum = NULL;
    mdTypeDef typeDef = NULL;
    DWORD dwTypeDefFlags;
    mdToken tkExtends;
    mdToken rtkImplements[] = { mdTokenNil };

    PROFILER_WCHAR szTypeDef[STRING_LENGTH];

    m_pSourceImport->GetTypeDefProps(typeDef, szTypeDef, STRING_LENGTH, NULL, &dwTypeDefFlags, &tkExtends);
    IMethodMalloc* pTargetMethodMalloc = NULL;
    hr = m_pPrfCom->m_pInfo->GetILFunctionBodyAllocator(targetModuleId, &pTargetMethodMalloc);
    if (FAILED(hr))
    {
        FAILURE(L"Failure in ICorProfilerInfo::GetILFunctionBodyAllocator: " << HEX(hr));
        return;
    }
    AssemblyInjector injector(m_pPrfCom, m_modidSource, m_modidTarget, m_pSourceImport, m_pTargetImport, m_pTargetEmit, pTargetMethodMalloc);
    hr = injector.InjectTypes(types);
}