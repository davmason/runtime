// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "stdafx.h"

using namespace std;

InjectGenericTypes::InjectGenericTypes(IPrfCom * pPrfCom, TestType testType)
    : InjectTypes(pPrfCom,testType)
{
}

InjectGenericTypes::~InjectGenericTypes()
{
}

HRESULT InjectGenericTypes::Verify()
{
    TEST_DISPLAY(L"Verify called for InjectGenericTypes.");
    if (m_ulRejitCallCount == 1)
    {
        // This test expects REJIT to get called once.
        return S_OK;
    }
    else
    {
        TEST_FAILURE(L"InjectGenericTypes::Verify failed. Expected Rejit Count:1 , Current Rejit Count:" << m_ulRejitCallCount);
        return E_FAIL;
    }
}

HRESULT InjectGenericTypes::GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl)
{
    HRESULT hr;

    vector<const PLATFORM_WCHAR*> types;
    types.push_back(MDMASTER_TYPE);
    types.push_back(MDSOURCEGENERICCLASS);

    InjectAssembly(moduleId, types);
    hr = m_pProfilerInfo->ApplyMetaData(moduleId);
    if (FAILED(hr))
    {
        FAILURE(L"ApplyMetaData failed.    Return code: " << HEX(hr));
    }

    mdMethodDef tokTargetMethodToCall;
    hr = GetTestMethod(PROFILER_STRING("MDMaster"), PROFILER_STRING("Verify_InjectGenericTypes"), &tokTargetMethodToCall);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"RewriteIL failed to get methoddef.    Return code: " << HEX(hr));
        return hr;
    }

    RewriteIL(pFunctionControl, moduleId, methodId, tokTargetMethodToCall);
    return S_OK;
}
