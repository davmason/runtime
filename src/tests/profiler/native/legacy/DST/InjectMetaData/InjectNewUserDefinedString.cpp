#include "stdafx.h"


InjectNewUserDefinedString::InjectNewUserDefinedString(IPrfCom * pPrfCom, TestType testType)
    : InjectMetaData(pPrfCom,testType)
{
}

InjectNewUserDefinedString::~InjectNewUserDefinedString()
{

}

HRESULT InjectNewUserDefinedString::Verify()
{
    TEST_DISPLAY(L"Verify called for InjectNewUserDefinedString.");
    if (m_ulRejitCallCount == 1)
    {
        // This test expects REJIT to get called once.
        return S_OK;
    }
    else
    {
        TEST_FAILURE(L"InjectNewUserDefinedString::Verify failed. Expected Rejit Count:1 , Current Rejit Count:" << m_ulRejitCallCount);
        return E_FAIL;
    }
}

HRESULT InjectNewUserDefinedString::JITCompilationStarted(FunctionID functionID, BOOL fIsSafeToBlock)
{
    // When jitting happens for ControlFunction, rejit is triggered for PrintHelloWorld

    HRESULT hr;
    mdToken methodDef;
    ClassID classID;
    ModuleID moduleID;

    hr = m_pProfilerInfo->GetFunctionInfo(
        functionID,
        &classID,
        &moduleID,
        &methodDef);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"GetFunctionInfo for FunctionID '" << HEX(functionID) << L"' failed.  HRESULT = " << HEX(hr) << L".");
        return S_OK;
    }

    if (m_modidTarget == moduleID)
    {
        // Get function name and rejit
        PROFILER_WCHAR wszTypeDefName[DEFAULT_STRING_LENGTH];
        PROFILER_WCHAR wszMethodDefName[DEFAULT_STRING_LENGTH];
        PLATFORM_WCHAR wszMethodDefNamePlatform[DEFAULT_STRING_LENGTH];
        GetClassAndFunctionNamesFromMethodDef(
            m_pTargetImport,
            m_modidTarget,
            methodDef,
            wszTypeDefName,
            dimensionof(wszTypeDefName),
            wszMethodDefName,
            dimensionof(wszMethodDefName));

        ConvertProfilerWCharToPlatformWChar(wszMethodDefNamePlatform, DEFAULT_STRING_LENGTH, wszMethodDefName, DEFAULT_STRING_LENGTH);

        if (wcscmp(wszMethodDefNamePlatform, CONTROLFUNCTION) == 0)
        {
            CallRequestReJIT(1, &m_modidTarget, &m_tokmdTargetMethod);
        }
        else if (wcscmp(wszMethodDefNamePlatform, PRINTHELLOWORLD) == 0)
        {
            m_tokmdTargetMethod = methodDef;
        }
    }

    return S_OK;
}

HRESULT InjectNewUserDefinedString::GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl)
{
    RewriteIL(pFunctionControl, moduleId, methodId);
    return S_OK;
}

HRESULT InjectNewUserDefinedString::RewriteIL(
    ICorProfilerFunctionControl * pICorProfilerFunctionControl,
    ModuleID moduleID,
    mdMethodDef methodDef)
{
    mdString tokmdsUserDefined = mdStringNil;
    PLATFORM_WCHAR wszNewUserDefinedStringTemp[20] = L"abcdefghij"; 
    PROFILER_WCHAR wszNewUserDefinedString[20];
    ConvertPlatformWCharToProfilerWChar(wszNewUserDefinedString, 20, wszNewUserDefinedStringTemp, 20);

    if (!SUCCEEDED(m_pTargetEmit->DefineUserString(wszNewUserDefinedString, (ULONG)wcslen(wszNewUserDefinedStringTemp), &tokmdsUserDefined)))
    {
        TEST_FAILURE(L"RewriteIL DefineUserString failed");
        return S_OK;
    }

    ILRewriter rewriter(m_pProfilerInfo, pICorProfilerFunctionControl, moduleID, methodDef);

    IfFailRet(rewriter.Initialize());
    IfFailRet(rewriter.Import());

    ILInstr * pInstr = rewriter.GetILList()->m_pNext;

    while (pInstr != NULL && pInstr->m_opcode != CEE_LDSTR)
    {
        pInstr = pInstr->m_pNext;
    }

    if (pInstr != NULL)
    {
        pInstr->m_Arg32 = tokmdsUserDefined;
    }
    else
    {
        TEST_FAILURE("LDSTR opcode was not found.")
    }

    IfFailRet(rewriter.Export());

    return S_OK;
}
