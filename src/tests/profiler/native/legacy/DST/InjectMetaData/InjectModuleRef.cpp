#include "stdafx.h"

using namespace std;

InjectModuleRef::InjectModuleRef(IPrfCom * pPrfCom, TestType testType)
    : InjectMetaData(pPrfCom,testType),
      m_fNativeFunctionCalled(false)
{
    TEST_DISPLAY(L"InjectModuleRef created.");
}

InjectModuleRef::~InjectModuleRef()
{
}

HRESULT InjectModuleRef::Verify()
{
    TEST_DISPLAY(L"Verify called for InjectModuleRef.");

    HRESULT hr;
    PROFILER_WCHAR szModuleName[DEFAULT_STRING_LENGTH];
    PLATFORM_WCHAR szModuleNameTemp[DEFAULT_STRING_LENGTH];
    ULONG cReceived;

    hr = m_pTargetImport->GetModuleRefProps(m_tokModuleRef, szModuleName, DEFAULT_STRING_LENGTH, &cReceived);

    if (FAILED(hr))
    {
        TEST_FAILURE(L"GetModuleRefProps failed for Module token '" << HEX(m_tokModuleRef) << L"', hr = " << HEX(hr));
        return E_FAIL;
    }

    ConvertProfilerWCharToPlatformWChar(szModuleNameTemp, DEFAULT_STRING_LENGTH, szModuleName, DEFAULT_STRING_LENGTH);

    if (wcscmp(szModuleNameTemp, INJECTMETADATA_DLL) != 0)
    {
        TEST_FAILURE(L"ModuleName '" << szModuleNameTemp << L"' did not match the expected module '" << INJECTMETADATA_DLL << "'");
        return E_FAIL;
    }

    if (m_fNativeFunctionCalled != true)
    {
        TEST_FAILURE(L"InjectModuleRef::Verify failed. NativeFunction call expected.");
        return E_FAIL;
    }

    if (m_ulRejitCallCount == 1)
    {
        // This test expects REJIT to get called zero times.
        return S_OK;
    }
    else
    {
        TEST_FAILURE(L"InjectModuleRef::Verify failed. Expected Rejit Count:1 , Current Rejit Count:" << m_ulRejitCallCount);
        return E_FAIL;
    }
}

HRESULT InjectModuleRef::GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl)
{
    HRESULT hr;
#ifdef WINDOWS
    hr = m_pTargetEmit->DefineModuleRef(PROFILER_STRING("TestProfiler.dll"), &m_tokModuleRef);
#elif defined(LINUX)
    hr = m_pTargetEmit->DefineModuleRef(PROFILER_STRING("libTestProfiler.so"), &m_tokModuleRef);
#else
    hr = m_pTargetEmit->DefineModuleRef(PROFILER_STRING("libTestProfiler.dylib"), &m_tokModuleRef);
#endif
    if (FAILED(hr))
    {
        FAILURE(L"DefineModuleRef failed.    Return code: " << HEX(hr));
        return E_FAIL;
    }
    else
    {
        TEST_DISPLAY(L"DefineModuleRef successful. ModuleRef token:" << HEX(m_tokModuleRef));
    }

    mdTypeDef tokHelperType;
    //hr = m_pTargetEmit->DefineTypeDef(L"Helper", tdPublic | tdClass | tdAutoLayout, mdTokenNil, mdTokenNil, &tokHelperType);
    //if (FAILED(hr))
    //{
    //    FAILURE(L"DefineTypeDef failed.    Return code: " << HEX(hr));
    //    return E_FAIL;
    //}

    hr = m_pTargetImport->GetMethodProps(methodId, &tokHelperType, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL);
    if (FAILED(hr))
    {
        FAILURE(L"GetMethodProps failed.    Return code: " << HEX(hr));
        return E_FAIL;
    }

    mdMethodDef tokTargetMethodToCall;
    hr = AddPInvoke(tokHelperType, PROFILER_STRING("NativeFunction"), m_tokModuleRef, &tokTargetMethodToCall);
    if (FAILED(hr))
    {
        FAILURE(L"Adding PInvoke failed.    Return code: " << HEX(hr));
        return E_FAIL;
    }

    RewriteIL(pFunctionControl, moduleId, methodId, tokTargetMethodToCall);

    hr = m_pProfilerInfo->ApplyMetaData(moduleId);
    if (FAILED(hr))
    {
        FAILURE(L"ApplyMetaData failed.    Return code: " << HEX(hr));
        return E_FAIL;
    }
    return S_OK;
}

int InjectModuleRef::NativeFunction()
{
    TEST_DISPLAY(L"InjectModuleRef::NativeFunction called.");
    m_fNativeFunctionCalled = true;
    return 100;
}

HRESULT InjectModuleRef::AddPInvoke(
    mdTypeDef td,
    LPCWSTR wszName,         // Name of both the P/Invoke method, and the target native method
    mdModuleRef modrefTarget,
    mdMethodDef * pmdPInvoke)
{
    HRESULT hr;

    //COR_SIGNATURE representation
    //   Calling convention
    //   Number of Arguments
    //   Return type
    //   Argument type
    //   ...

    COR_SIGNATURE newMethodSignature[] = { IMAGE_CEE_CS_CALLCONV_DEFAULT,   //__stdcall
        0,
        ELEMENT_TYPE_I4
    };
    
    hr = m_pTargetEmit->DefineMethod(
        mdTokenNil, // TODO: test fails when td is used.. 
        wszName,
        ~mdAbstract & (mdStatic | mdPublic | mdPinvokeImpl),
        newMethodSignature,
        sizeof(newMethodSignature),
        0,
        miNative | miUnmanaged | miPreserveSig,
        pmdPInvoke);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"Failed in DefineMethod when creating P/Invoke method '" << wszName <<
            L"'.  hr=" << HEX(hr));
        return hr;
    }

    hr = m_pTargetEmit->DefinePinvokeMap(
        *pmdPInvoke,
        pmCallConvStdcall | pmNoMangle,
        wszName,
        modrefTarget);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"Failed in DefinePinvokeMap when creating P/Invoke method '" << wszName <<
            L"'.  hr=" << HEX(hr));
        return hr;
    }

    return hr;
}

HRESULT InjectModuleRef::RewriteIL(
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
