// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "stdafx.h"

using namespace std;

InjectTypeSpec::InjectTypeSpec(IPrfCom * pPrfCom, TestType testType)
    : InjectMethodSpec(pPrfCom,testType)
{
}

InjectTypeSpec::~InjectTypeSpec()
{
}

HRESULT InjectTypeSpec::Verify()
{
    TEST_DISPLAY(L"Verify called for InjectTypeSpec.");
    if (m_ulRejitCallCount == 1)
    {
        return S_OK;
    }
    else
    {
        TEST_FAILURE(L"InjectTypeSpec::Verify failed. Expected Rejit Count:1 , Current Rejit Count:" << m_ulRejitCallCount);
        return E_FAIL;
    }
}

HRESULT InjectTypeSpec::GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl)
{
    HRESULT hr;

    TEST_DISPLAY("Entering InjectMethodSpec::GetReJITParameters");

    mdTypeDef tokTypeDefGenericClass;
    hr = m_pTargetImport->FindTypeDefByName(PROFILER_STRING("MyGenericClass`1"), mdTokenNil, &tokTypeDefGenericClass);
    if (FAILED(hr))
    {
        FAILURE(L"Finding typedef failed.    Return code: " << HEX(hr));
        return hr;
    }

    mdGenericParam tokGenericParam = mdGenericParamNil;
    hr = m_pTargetEmit->DefineGenericParam(tokTypeDefGenericClass,
        0,
        0,
        PROFILER_STRING("T"),
        mdTokenNil,
        NULL,
        &tokGenericParam);
    if (FAILED(hr))
    {
        FAILURE(L"DefineGenericParam failed.    Return code: " << HEX(hr));
        return hr;
    }

    mdTypeSpec tokTypeSpecGenericClass = mdTypeSpecNil;
    MetadataSignatureBuilder sig;
    sig.WriteElemType(ELEMENT_TYPE_GENERICINST);
    sig.WriteElemType(ELEMENT_TYPE_CLASS);
    sig.WriteToken(tokTypeDefGenericClass);
    sig.WriteData(1);
    sig.WriteElemType(ELEMENT_TYPE_I4);

    hr = m_pTargetEmit->GetTokenFromTypeSpec(sig.GetSignature(), sig.GetCbSignature(), &tokTypeSpecGenericClass);
    if (FAILED(hr))
    {
        FAILURE(L"GetTokenFromTypeSpec failed.    Return code: " << HEX(hr));
        return hr;
    }

    MetadataSignatureBuilder sigMethod;
    sigMethod.WriteCallingConvInfo(IMAGE_CEE_CS_CALLCONV_DEFAULT);
    sigMethod.WriteData(1);
    sigMethod.WriteElemType(ELEMENT_TYPE_I4);
    sigMethod.WriteElemType(ELEMENT_TYPE_VAR);
    sigMethod.WriteData(0);
    
    mdMemberRef tokMethod = mdMemberRefNil;
    hr = m_pTargetEmit->DefineMemberRef(tokTypeSpecGenericClass, PROFILER_STRING("Method"), sigMethod.GetSignature(), sigMethod.GetCbSignature(), &tokMethod);
    if (FAILED(hr))
    {
        FAILURE(L"DefineMemberRef failed.    Return code: " << HEX(hr));
        return hr;
    }

    hr = m_pProfilerInfo->ApplyMetaData(moduleId);
    if (FAILED(hr))
    {
        FAILURE(L"ApplyMetaData failed.    Return code: " << HEX(hr));
        return hr;
    }

    RewriteIL(pFunctionControl, moduleId, methodId, tokMethod);
    return S_OK;
}

HRESULT InjectTypeSpec::RewriteIL(
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


HRESULT InjectTypeSpec::ModuleLoadFinished(ModuleID moduleID, HRESULT hrStatus)
{
    HRESULT hr = InjectMetaData::ModuleLoadFinished(moduleID, hrStatus);

    if (FAILED(hr))
    {
        return hr;
    }

    LPCBYTE pbBaseLoadAddr;
    PROFILER_WCHAR wszName[DEFAULT_STRING_LENGTH];
    PLATFORM_WCHAR wszNamePlatform[DEFAULT_STRING_LENGTH];
    ULONG cchNameIn = dimensionof(wszName);
    ULONG cchNameOut;
    AssemblyID assemblyID;
    DWORD dwModuleFlags;
    hr = m_pProfilerInfo->GetModuleInfo2(
        moduleID,
        &pbBaseLoadAddr,
        cchNameIn,
        &cchNameOut,
        wszName,
        &assemblyID,
        &dwModuleFlags);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"GetModuleInfo2 failed for ModuleID '" << HEX(moduleID) << L"', hr = " << HEX(hr));
        return S_OK;
    }
    ConvertProfilerWCharToPlatformWChar(wszNamePlatform, DEFAULT_STRING_LENGTH, wszName, DEFAULT_STRING_LENGTH);
    if (ContainsAtEnd(wszNamePlatform, MDINJECTTARGET_WITHEXT) || ContainsAtEnd(wszNamePlatform, MDINJECTTARGET_NATIVEIMAGE_WITHEXT))
    {
        mdTypeDef tokTypeDef;
        hr = m_pTargetImport->FindTypeDefByName(PROFILER_STRING("TestTypeSpec"), NULL, &tokTypeDef);
        if (FAILED(hr))
        {
            FAILURE(L"FindTypeDefByName failed.    Return code: " << HEX(hr));
            return hr;
        }

        mdMethodDef tokMethod = mdMethodDefNil;
        HCORENUM hEnum = NULL;
        ULONG cTok;
        hr = m_pTargetImport->EnumMethodsWithName(&hEnum, tokTypeDef, PROFILER_STRING("CallMethod"), &tokMethod, 1, &cTok);
        if (FAILED(hr) || cTok != 1)
        {
            FAILURE(L"EnumMethods failed.    Return code: " << HEX(hr));
            return hr;
        }
        m_pTargetImport->CloseEnum(hEnum);

        CallRequestReJIT(1, &m_modidTarget, &tokMethod);
    }

    return S_OK;
}


HRESULT InjectTypeSpec::JITCompilationStarted(FunctionID functionID, BOOL fIsSafeToBlock)
{
    // REJIT is triggered at module load time rather than the JIT Compilation time
    return S_OK; 
}