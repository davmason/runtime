// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "stdafx.h"
using namespace std;

#pragma warning(push)
#pragma warning( disable : 4996)
InjectAssemblyRef::InjectAssemblyRef(IPrfCom * pPrfCom, TestType testType)
    : InjectMetaData(pPrfCom,testType)
{
}

InjectAssemblyRef::~InjectAssemblyRef()
{
}

HRESULT InjectAssemblyRef::Verify()
{
    TEST_DISPLAY(L"Verify called for InjectAssemblyRef.");
    if (m_ulRejitCallCount == 1)
    {
        // This test expects REJIT to get called once.
        return S_OK;
    }
    else
    {
        TEST_FAILURE(L"InjectAssemblyRef::Verify failed. Expected Rejit Count:1 , Current Rejit Count:" << m_ulRejitCallCount);
        return E_FAIL;
    }
}

HRESULT InjectAssemblyRef::GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl)
{
    HRESULT hr;

    TEST_DISPLAY("Entering InjectAssemblyRef::GetReJITParameters");

    hr = AddAssemblyRef();
    if (FAILED(hr))
    {
        FAILURE(L"Adding assemblyref failed.    Return code: " << HEX(hr));
        return hr;
    }

    mdMemberRef tokTargetMethodToCall;
    hr = GetTestMethod(PROFILER_STRING("MDMaster"), PROFILER_STRING("Verify_InjectAssemblyRef"), &tokTargetMethodToCall);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"RewriteIL failed to get methodref.    Return code: " << HEX(hr));
        return hr;
    }

    hr = m_pProfilerInfo->ApplyMetaData(moduleId);
    if (FAILED(hr))
    {
        FAILURE(L"ApplyMetaData failed.    Return code: " << HEX(hr));
        return hr;
    }

    RewriteIL(pFunctionControl, moduleId, methodId, tokTargetMethodToCall);
    return S_OK;
}

HRESULT InjectAssemblyRef::AddAssemblyRef()
{
    HRESULT hr;
    // Generate assemblyRef to MDInjectSource.exe
    BYTE rgbPublicKeyToken[] = { 0xcc, 0xac, 0x92, 0xed, 0x87, 0x3b, 0x18, 0x5c };
    PLATFORM_WCHAR wszLocale[MAX_PATH];
    wcscpy(wszLocale, L"neutral");

    ASSEMBLYMETADATA assemblyMetaData;
    memset(&assemblyMetaData, 0, sizeof(assemblyMetaData));
    assemblyMetaData.usMajorVersion = 1;
    assemblyMetaData.usMinorVersion = 0;
    assemblyMetaData.usBuildNumber = 0;
    assemblyMetaData.usRevisionNumber = 0;
    ConvertPlatformWCharToProfilerWChar(assemblyMetaData.szLocale, MAX_PATH, wszLocale, MAX_PATH);
    assemblyMetaData.cbLocale = dimensionof(wszLocale);

    hr = m_pAssemblyEmit->DefineAssemblyRef(
        (void *)rgbPublicKeyToken,
        sizeof(rgbPublicKeyToken),
        PROFILER_STRING("MDInjectSource"),
        &assemblyMetaData,
        NULL,                   // hash blob
        NULL,                   // cb of hash blob
        0,                      // flags
        &m_tokmdSourceAssemblyRef);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"DefineAssemblyRef failed with HRESULT = " << HEX(hr));
    }

    return hr;
}

HRESULT InjectAssemblyRef::GetTestMethod(LPCWSTR pwzTypeName, LPCWSTR pwzMethodName, mdMemberRef* ptokMethod)
{
    *ptokMethod = mdMemberRefNil;

    COR_SIGNATURE sigFunctionProbe[] =
    {
        IMAGE_CEE_CS_CALLCONV_DEFAULT,      // default calling convention
        0x00,                               // number of arguments == 0
        ELEMENT_TYPE_I4,                    // return type == int
    };

    HRESULT hr;
    mdTypeRef tokTypeRef;

    hr = m_pTargetEmit->DefineTypeRefByName(m_tokmdSourceAssemblyRef, pwzTypeName, &tokTypeRef);
    if (FAILED(hr))
    {
        FAILURE(L"GetTestMethod failed to get the type.    Return code: " << HEX(hr));
        return hr;
    }

    hr = m_pTargetEmit->DefineMemberRef(tokTypeRef, pwzMethodName, sigFunctionProbe, sizeof(sigFunctionProbe), ptokMethod);
    if (FAILED(hr))
    {
        FAILURE(L"GetTestMethod failed to get member ref.    Return code: " << HEX(hr));
        return hr;
    }
    else
    {
        TEST_DISPLAY("MemberRef " << HEX(*ptokMethod) << " found.");
    }

    return S_OK;
}

HRESULT InjectAssemblyRef::RewriteIL(
    ICorProfilerFunctionControl * pICorProfilerFunctionControl,
    ModuleID moduleID,
    mdMethodDef methodDef,
    mdMemberRef methodToCall)
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
#pragma warning(pop)