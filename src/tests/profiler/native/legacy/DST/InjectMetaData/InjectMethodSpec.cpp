#include "stdafx.h"

using namespace std;

InjectMethodSpec::InjectMethodSpec(IPrfCom * pPrfCom, TestType testType)
    : InjectMetaData(pPrfCom,testType)
{
}

InjectMethodSpec::~InjectMethodSpec()
{
}

HRESULT InjectMethodSpec::Verify()
{
    TEST_DISPLAY(L"Verify called for InjectMethodSpec.");
    if (m_ulRejitCallCount == 1)
    {
        return S_OK;
    }
    else
    {
        TEST_FAILURE(L"InjectMethodSpec::Verify failed. Expected Rejit Count:1 , Current Rejit Count:" << m_ulRejitCallCount);
        return E_FAIL;
    }
}

HRESULT InjectMethodSpec::GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl)
{
    HRESULT hr;

    TEST_DISPLAY("Entering InjectTypeSpec::GetReJITParameters");

    mdTypeDef tokStaticClassType;
    hr = m_pTargetImport->FindTypeDefByName(PROFILER_STRING("SomeStaticClass"), mdTokenNil, &tokStaticClassType);
    if (FAILED(hr))
    {
        FAILURE(L"Finding typedef failed.    Return code: " << HEX(hr));
        return hr;
    }

    COR_SIGNATURE s1[] = {  // int Method0(<gp>);
        IMAGE_CEE_CS_CALLCONV_GENERICINST,
        1,
        ELEMENT_TYPE_I4,
        ELEMENT_TYPE_I4
    };

    mdMethodDef tokGenericMethod = mdMethodDefNil;
    HCORENUM hEnum = NULL;
    ULONG cTok;
    hr = m_pTargetImport->EnumMethodsWithName(&hEnum, tokStaticClassType, PROFILER_STRING("GenericMethod"), &tokGenericMethod, 1, &cTok);
    if (FAILED(hr) || cTok != 1)
    {
        FAILURE(L"EnumMethods failed.    Return code: " << HEX(hr));
        return hr;
    }
    m_pTargetImport->CloseEnum(hEnum);

    mdGenericParam tokGenericParam = mdGenericParamNil;
    hr = m_pTargetEmit->DefineGenericParam(tokGenericMethod,
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

    mdMethodSpec tokMethodSpec = mdMethodSpecNil;
    hr = m_pTargetEmit->DefineMethodSpec(
        tokGenericMethod,
        (PCCOR_SIGNATURE)s1,
        sizeof(s1),
        &tokMethodSpec);
    if (FAILED(hr))
    {
        FAILURE(L"DefineMethodSpec failed.    Return code: " << HEX(hr));
        return hr;
    }

    hr = m_pProfilerInfo->ApplyMetaData(moduleId);
    if (FAILED(hr))
    {
        FAILURE(L"ApplyMetaData failed.    Return code: " << HEX(hr));
        return hr;
    }

    RewriteIL(pFunctionControl, moduleId, methodId, tokMethodSpec);
    return S_OK;
}

HRESULT InjectMethodSpec::RewriteIL(
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


HRESULT InjectMethodSpec::ModuleLoadFinished(ModuleID moduleID, HRESULT hrStatus)
{
    HRESULT hr = InjectMetaData::ModuleLoadFinished(moduleID, hrStatus);

    if (FAILED(hr))
    {
        return hr;
    }

    LPCBYTE pbBaseLoadAddr;
    PROFILER_WCHAR wszName[DEFAULT_STRING_LENGTH];
    PLATFORM_WCHAR wszNameTemp[DEFAULT_STRING_LENGTH];
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

    ConvertProfilerWCharToPlatformWChar(wszNameTemp, DEFAULT_STRING_LENGTH, wszName, DEFAULT_STRING_LENGTH);
    if (ContainsAtEnd(wszNameTemp, MDINJECTTARGET_WITHEXT) || ContainsAtEnd(wszNameTemp, MDINJECTTARGET_NATIVEIMAGE_WITHEXT))
    {
        mdTypeDef tokTypeDef;
        hr = m_pTargetImport->FindTypeDefByName(PROFILER_STRING("TestMethodSpec"), NULL, &tokTypeDef);
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


HRESULT InjectMethodSpec::JITCompilationStarted(FunctionID functionID, BOOL fIsSafeToBlock)
{
    // REJIT is triggered at module load time rather than the JIT Compilation time
    return S_OK; 
}