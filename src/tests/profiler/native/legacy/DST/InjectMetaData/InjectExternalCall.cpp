#include "stdafx.h"

const PROFILER_WCHAR *INJECTED_TYPENAME = PROFILER_STRING("InjectType");
const PROFILER_WCHAR *INJECTED_METHOD = PROFILER_STRING("InjectMethod");
using namespace std;

InjectExternalCall::InjectExternalCall(IPrfCom * pPrfCom, TestType testType)
    : InjectMetaData(pPrfCom,testType),
    m_pCoreLibImport(),
    m_bExternalMethodJitted(false),
    m_bInjectCoreLib(testType == kInjectExternalMethodCallCoreLib),
    m_modidCoreLib(mdTokenNil),
    m_modidSystemRuntime(mdTokenNil)
{

}

InjectExternalCall::~InjectExternalCall()
{
}

HRESULT InjectExternalCall::Verify()
{
    TEST_DISPLAY(L"Verify called for InjectExternalCall.");
    if (m_ulRejitCallCount == 1 && m_bExternalMethodJitted)
    {
        return S_OK;
    }
    else
    {
        TEST_FAILURE(L"InjectExternalCall::Verify failed. Expected Rejit Count:1 , Current Rejit Count:" << m_ulRejitCallCount);
        TEST_FAILURE(L"m_bExternalMethodJitted: " << m_bExternalMethodJitted);
        return E_FAIL;
    }
}

HRESULT InjectExternalCall::GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl)
{
    HRESULT hr;

    TEST_DISPLAY("Entering InjectExternalCall::GetReJITParameters");

    mdMemberRef methodToCall;
    if (m_bInjectCoreLib)
    {
        hr = InjectTypeInCoreLib(&methodToCall);
    }
    else
    {
        hr = InjectTypeInSource(&methodToCall);
    }

    if (FAILED(hr))
    {
        FAILURE(L"Failed to inject type");
        return hr;
    }

    // Rewrite the method with a call to InjectType.InjectMethod
    return RewriteIL(pFunctionControl, m_modidTarget, m_tokmdTargetMethod, methodToCall);
}

HRESULT InjectExternalCall::RewriteIL(
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

BOOL InjectExternalCall::MethodNameMatches(IMetaDataImport2 *pImport, ModuleID moduleID, mdMethodDef methodDef, wstring methodName)
{
    PROFILER_WCHAR wszTypeDefName[DEFAULT_STRING_LENGTH];
    PROFILER_WCHAR wszMethodDefName[DEFAULT_STRING_LENGTH];
    PLATFORM_WCHAR wszMethodDefNamePlatform[DEFAULT_STRING_LENGTH];
    GetClassAndFunctionNamesFromMethodDef(
        pImport,
        moduleID,
        methodDef,
        wszTypeDefName,
        dimensionof(wszTypeDefName),
        wszMethodDefName,
        dimensionof(wszMethodDefName));

    ConvertProfilerWCharToPlatformWChar(wszMethodDefNamePlatform, DEFAULT_STRING_LENGTH, wszMethodDefName, DEFAULT_STRING_LENGTH);

    return wcscmp(wszMethodDefNamePlatform, methodName.c_str()) == 0;
}

HRESULT InjectExternalCall::JITCompilationStarted(FunctionID functionID, BOOL fIsSafeToBlock)
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
        if (MethodNameMatches(m_pTargetImport, moduleID, methodDef, CONTROLFUNCTION))
        {
            CallRequestReJIT(1, &m_modidTarget, &m_tokmdTargetMethod);
        }
        else if (MethodNameMatches(m_pTargetImport, moduleID, methodDef, L"CallMethod"))
        {
            m_tokmdTargetMethod = methodDef;
        }
    }

    if (!m_bInjectCoreLib && moduleID == m_modidSource)
    {
        if (MethodNameMatches(m_pSourceImport, moduleID, methodDef, L"InjectMethod"))
        {
            TEST_DISPLAY(L"Injected method jitting started");
            m_bExternalMethodJitted = TRUE;
        }
    }

    if (m_bInjectCoreLib && moduleID == m_modidCoreLib)
    {
        if (MethodNameMatches(m_pCoreLibImport, moduleID, methodDef, L"InjectMethod"))
        {
            TEST_DISPLAY(L"Injected method jitting started");
            m_bExternalMethodJitted = TRUE;
        }
    }

    return S_OK;
}

HRESULT InjectExternalCall::ModuleLoadFinished(ModuleID moduleID, HRESULT hrStatus)
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

    if (ContainsAtEnd(wszNameTemp, L"System.Private.CoreLib.dll"))
    {
        m_modidCoreLib = moduleID;
        TEST_DISPLAY(L"System.Private.CoreLib found on ModuleLoadFinished");

        hr = m_pProfilerInfo->GetModuleMetaData(m_modidCoreLib, ofWrite, IID_IMetaDataImport2, (IUnknown **)&m_pCoreLibImport);
        if (FAILED(hr))
        {
            TEST_FAILURE(L"GetModuleMetaData failed for IID_IMetaDataImport2 in System.Private.CoreLib");
        }
    }

    if (ContainsAtEnd(wszNameTemp, L"System.Runtime.dll"))
    {
        m_modidSystemRuntime = moduleID;
        TEST_DISPLAY(L"System.Runtime found on ModuleLoadFinished");
    }

    return S_OK;
}

HRESULT InjectExternalCall::GetMemberRefFromAssemblyRef(mdAssemblyRef assemblyRef, mdMemberRef *memberRef)
{
    TEST_DISPLAY("Entering InjectExternalCall::GetMemberRefFromTypeRef");

    HRESULT hr;
    *memberRef = mdTokenNil;

    // Use the assemblyRef to get a typeRef to InjectType we just injected
    mdTypeRef classToCall = mdTokenNil;
    hr = m_pTargetEmit->DefineTypeRefByName(assemblyRef, INJECTED_TYPENAME, &classToCall);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"Unable to define type ref to type injected in MDInjectSource.dll");
        return hr;
    }
    
    COR_SIGNATURE s1[] = {                      // int Method()
        IMAGE_CEE_CS_CALLCONV_DEFAULT,          // Default calling convention
        0,                                      // 0 arguments
        ELEMENT_TYPE_I4                         // int return type
    };

    // Use the typeRef to get a memberRef to InjectType.InjectMethod we just injected
    mdMemberRef methodToCall = mdTokenNil;
    hr = m_pTargetEmit->DefineMemberRef(classToCall, INJECTED_METHOD, (PCOR_SIGNATURE)&s1, sizeof(s1), &methodToCall);
    if (FAILED(hr))
    {
        FAILURE(L"Unable to define member ref to NewInjectedSourceType.NewMethod");
        return hr;
    }

    *memberRef = methodToCall;

    return S_OK;
}

HRESULT InjectExternalCall::GetAssemblyRef(COMPtrHolder<IMetaDataAssemblyImport> pImport, wstring refName, mdAssemblyRef *assemblyRef)
{
    TEST_DISPLAY("Entering InjectExternalCall::GetAssemblyRef");

    HRESULT hr;
    *assemblyRef = mdTokenNil;

    // Look for System.Private.CoreLib assembly ref
    HCORENUM hEnum = NULL;
    ULONG cAssemblyRefsReturned = 0;
    while (TRUE)
    {
        hr = pImport->EnumAssemblyRefs(
            &hEnum,
            assemblyRef,
            1,
            &cAssemblyRefsReturned);
        if (FAILED(hr))
        {
            TEST_FAILURE(L"EnumAssemblyRefs failed with HRESULT = " << HEX(hr));
            return hr;
        }

        if (cAssemblyRefsReturned == 0)
        {
            TEST_FAILURE(L"Could not find an AssemblyRef");
            return E_FAIL;
        }

        const void * pvPublicKeyOrToken;
        ULONG cbPublicKeyOrToken;
        PROFILER_WCHAR wszNameTemp[STRING_LENGTH];
        PLATFORM_WCHAR wszName[STRING_LENGTH];
        ULONG cchNameReturned;
        ASSEMBLYMETADATA asmMetaData;
        memset(&asmMetaData, 0, sizeof(asmMetaData));
        const void * pbHashValue;
        ULONG cbHashValue;
        DWORD asmRefFlags;

        hr = pImport->GetAssemblyRefProps(
            *assemblyRef,
            &pvPublicKeyOrToken,
            &cbPublicKeyOrToken,
            wszNameTemp,
            STRING_LENGTH,
            &cchNameReturned,
            &asmMetaData,
            &pbHashValue,
            &cbHashValue,
            &asmRefFlags);
        if (FAILED(hr))
        {
            TEST_FAILURE(L"GetAssemblyRefProps failed with hr=0x" << std::hex << hr);
            return hr;
        }

        ConvertProfilerWCharToPlatformWChar(wszName, STRING_LENGTH, wszNameTemp, STRING_LENGTH);

        if (ContainsAtEnd(wszName, refName.c_str()))
        {
            TEST_DISPLAY(L"Found assemblyRef for " << refName << L" (" << HEX(*assemblyRef) << L")");
            break;
        }

    }

    pImport->CloseEnum(hEnum);
    hEnum = NULL;

    return S_OK;
}

HRESULT InjectExternalCall::InjectTypeInCoreLib(mdMemberRef *memberRef)
{
    TEST_DISPLAY("Entering InjectExternalCall::InjectTypeInCoreLib");

    if (memberRef == NULL)
    {
        return E_FAIL;
    }

    HRESULT hr = S_OK;
    COMPtrHolder<IMetaDataEmit2> pCoreLibEmit;
    hr = m_pProfilerInfo->GetModuleMetaData(m_modidCoreLib, ofWrite, IID_IMetaDataEmit2, (IUnknown **)&pCoreLibEmit);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"Failed to QI for IMetaDataEmit2 in System.Private.CoreLib");
        return hr;
    }
    
    IMethodMalloc* pTargetMethodMalloc = NULL;
    hr = m_pProfilerInfo->GetILFunctionBodyAllocator(m_modidCoreLib, &pTargetMethodMalloc);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"Failed to get IMethodMalloc for System.Private.CoreLib.dll");
        return hr;
    }

    // Inject from MDInjectTarget in to System.Private.CoreLib
    const vector<const PLATFORM_WCHAR*> types = { L"InjectType" };
    AssemblyInjector injector(m_pPrfCom, m_modidTarget, m_modidCoreLib, m_pTargetImport, m_pCoreLibImport, pCoreLibEmit, pTargetMethodMalloc);
    hr = injector.InjectTypes(types);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"Failed to inject InjectType in to System.Private.CoreLib.dll");
        return hr;
    }

    // Now add type forwarders to System.Runtime
    COMPtrHolder<IMetaDataAssemblyImport> pSystemRuntimeAssemblyImport;
    hr = m_pProfilerInfo->GetModuleMetaData(m_modidSystemRuntime, ofWrite, IID_IMetaDataAssemblyImport, (IUnknown **)&pSystemRuntimeAssemblyImport);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"Failed to QI for IID_IMetaDataAssemblyImport on System.Runtime");
        return hr;
    }

    COMPtrHolder<IMetaDataAssemblyEmit> pSystemRuntimeAssemblyEmit;
    hr = m_pProfilerInfo->GetModuleMetaData(m_modidSystemRuntime, ofWrite, IID_IMetaDataAssemblyEmit, (IUnknown **)&pSystemRuntimeAssemblyEmit);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"Failed to QI for IID_IMetaDataAssemblyEmit on System.Runtime");
        return hr;
    }

    mdAssemblyRef assemblyRef = mdTokenNil;
    hr = GetAssemblyRef(pSystemRuntimeAssemblyImport, L"System.Private.CoreLib", &assemblyRef);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"Unable to find System.Private.CoreLib assembly reference");
    }

    mdExportedType classToCall;
    hr = pSystemRuntimeAssemblyEmit->DefineExportedType(INJECTED_TYPENAME, assemblyRef, 0, tdForwarder, &classToCall);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"Failed to create forwarder in System.Runtime to InjectType");
        return hr;
    }

    mdAssemblyRef systemRuntimeRef = mdTokenNil;
    hr = GetAssemblyRef(m_pAssemblyImport, L"System.Runtime", &systemRuntimeRef);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"Unable to find System.Runtime assembly reference");
    }

    mdMemberRef methodToCall = mdTokenNil;
    hr = GetMemberRefFromAssemblyRef(systemRuntimeRef, &methodToCall);
    if (FAILED(hr))
    {
        TEST_FAILURE("Failed to get member ref");
        return hr;
    }

    *memberRef = methodToCall;

    // Apply injected types
    m_pProfilerInfo->ApplyMetaData(m_modidCoreLib);
    m_pProfilerInfo->ApplyMetaData(m_modidSource);
    m_pProfilerInfo->ApplyMetaData(m_modidTarget);
    m_pProfilerInfo->ApplyMetaData(m_modidSystemRuntime);

    return S_OK;
}

HRESULT InjectExternalCall::InjectTypeInSource(mdMemberRef *memberRef)
{
    TEST_DISPLAY("Entering InjectExternalCall::InjectTypeInSource");

    if (memberRef == NULL)
    {
        return E_FAIL;
    }

    HRESULT hr = S_OK;
    // Need to write metadata in to MDInjectSource.dll to make a cross assembly call
    COMPtrHolder<IMetaDataEmit2> pSourceEmit;
    hr = m_pSourceImport->QueryInterface(IID_IMetaDataEmit2, (LPVOID *)&pSourceEmit);
    if (FAILED(hr))
    {
        FAILURE(L"Failed to QI for pSourceEmit");
        return hr;
    }
    
    IMethodMalloc* pTargetMethodMalloc = NULL;
    hr = m_pProfilerInfo->GetILFunctionBodyAllocator(m_modidSource, &pTargetMethodMalloc);
    if (FAILED(hr))
    {
        FAILURE(L"Failed to get IMethodMalloc for m_modidSource");
        return hr;
    }

    // Purposefully reversing source and target here, need to have a cross assembly call. 
    const vector<const PLATFORM_WCHAR*> types = { L"InjectType" };
    AssemblyInjector injector(m_pPrfCom, m_modidTarget, m_modidSource, m_pTargetImport, m_pSourceImport, pSourceEmit, pTargetMethodMalloc);
    hr = injector.InjectTypes(types);
    if (FAILED(hr))
    {
        FAILURE(L"Failed to inject InjectType in to MDInjectSource.dll");
    }

    // Now get an assemblyRef to MDInjectSource
    PROFILER_WCHAR wszLocale[MAX_PATH] = PROFILER_STRING("neutral");
    ASSEMBLYMETADATA assemblyMetaData;
    memset(&assemblyMetaData, 0, sizeof(assemblyMetaData));
    assemblyMetaData.usMajorVersion = 0;
    assemblyMetaData.usMinorVersion = 0;
    assemblyMetaData.usBuildNumber = 0;
    assemblyMetaData.usRevisionNumber = 0;
    assemblyMetaData.szLocale = wszLocale;
    assemblyMetaData.cbLocale = dimensionof(wszLocale);

    mdAssemblyRef assemblyRef = mdTokenNil;
    hr = m_pAssemblyEmit->DefineAssemblyRef(
        NULL,
        0,
        PROFILER_STRING("MDInjectSource"),
        &assemblyMetaData,
        NULL,                   // hash blob
        NULL,                   // cb of hash blob
        0,                      // flags
        &assemblyRef);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"DefineAssemblyRef failed with HRESULT = 0x" << std::hex << hr);
    }

    mdMemberRef methodToCall = mdTokenNil;
    hr = GetMemberRefFromAssemblyRef(assemblyRef, &methodToCall);
    if (FAILED(hr))
    {
        TEST_FAILURE("Failed to get member ref");
        return hr;
    }

    *memberRef = methodToCall;

    // Apply injected types
    m_pProfilerInfo->ApplyMetaData(m_modidSource);
    m_pProfilerInfo->ApplyMetaData(m_modidTarget);

    return S_OK;
}