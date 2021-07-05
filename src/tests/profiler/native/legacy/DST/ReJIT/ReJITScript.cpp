#include "ProfilerCommon.h"
#include "ProfilerCommonHelper.h"
#include "ReJITScript.h"
#include <algorithm>
#include <thread>

#pragma warning(push)
#pragma warning( disable : 4996)

#if defined(_M_X64) || defined(__amd64__) || defined (__ARM_ARCH_ISA_A64) || defined (_M_ARM64)
    wstring k_wszEnteredFunctionProbeName = L"MgdEnteredFunction64";
    const PROFILER_WCHAR * k_wszEnteredFunctionProbeNameProfiler = PROFILER_STRING("MgdEnteredFunction64");
    wstring k_wszExitedFunctionProbeName = L"MgdExitedFunction64";
    const PROFILER_WCHAR * k_wszExitedFunctionProbeNameProfiler = PROFILER_STRING("MgdExitedFunction64");
#else //  ! defined(_M_X64) || defined(__amd64__) || defined(__ARM_ARCH_ISA_A64) || defined (_M_ARM64)
    wstring k_wszEnteredFunctionProbeName = L"MgdEnteredFunction32";
    const PROFILER_WCHAR * k_wszEnteredFunctionProbeNameProfiler = PROFILER_STRING("MgdEnteredFunction32");
    wstring k_wszExitedFunctionProbeName = L"MgdExitedFunction32";
    const PROFILER_WCHAR * k_wszExitedFunctionProbeNameProfiler = PROFILER_STRING("MgdExitedFunction32");
#endif //  ! defined(_M_X64) || defined(__amd64__) || defined(__ARM_ARCH_ISA_A64) || defined (_M_ARM64)

// When pumping managed helpers into mscorlib, stick them into this
// new mscorlib type
wstring k_wszHelpersContainerType = L"ProfilingHelpers";
const PROFILER_WCHAR *k_wszHelpersContainerTypeProfiler = PROFILER_STRING("ProfilingHelpers");

ReJITScript * g_pReJITScript = NULL;

HRESULT ReJITScript_Verify(IPrfCom * pPrfCom)
{
    return reinterpret_cast<ReJITScript *>(pPrfCom->m_pTestClassInstance)->Verify();
}

HRESULT ReJITScript_ReJITCompilationStarted(IPrfCom * pPrfCom, FunctionID functionID, ReJITID rejitId, BOOL fIsSafeToBlock)
{
    return reinterpret_cast<ReJITScript *>(pPrfCom->m_pTestClassInstance)->ReJITCompilationStarted(functionID, rejitId, fIsSafeToBlock);
}

HRESULT ReJITScript_GetReJITParameters(IPrfCom * pPrfCom, ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl)
{
    return reinterpret_cast<ReJITScript *>(pPrfCom->m_pTestClassInstance)->GetReJITParameters(moduleId, methodId, pFunctionControl);
}

HRESULT ReJITScript_ReJITCompilationFinished(IPrfCom * pPrfCom, FunctionID functionId, ReJITID rejitId, HRESULT hrStatus, BOOL fIsSafeToBlock)
{
    return reinterpret_cast<ReJITScript *>(pPrfCom->m_pTestClassInstance)->ReJITCompilationFinished(functionId, rejitId, hrStatus, fIsSafeToBlock);
}

HRESULT ReJITScript_ReJITError(IPrfCom * pPrfCom, ModuleID moduleId, mdMethodDef methodId, FunctionID functionId, HRESULT hrStatus)
{
    return reinterpret_cast<ReJITScript *>(pPrfCom->m_pTestClassInstance)->ReJITError(moduleId, methodId, functionId, hrStatus);
}

HRESULT ReJITScript_AppDomainCreationStarted(IPrfCom * pPrfCom, AppDomainID appDomainID)
{
    return reinterpret_cast<ReJITScript *>(pPrfCom->m_pTestClassInstance)->AppDomainCreationStarted(appDomainID);
}

HRESULT ReJITScript_AppDomainShutdownFinished(IPrfCom * pPrfCom, AppDomainID appDomainID, HRESULT hrStatus)
{
    return reinterpret_cast<ReJITScript *>(pPrfCom->m_pTestClassInstance)->AppDomainShutdownFinished(appDomainID, hrStatus);
}

HRESULT ReJITScript_ModuleLoadFinished(IPrfCom * pPrfCom, ModuleID moduleID, HRESULT hrStatus)
{
    return reinterpret_cast<ReJITScript *>(pPrfCom->m_pTestClassInstance)->ModuleLoadFinished(moduleID, hrStatus);
}

HRESULT ReJITScript_ModuleUnloadStarted(IPrfCom * pPrfCom, ModuleID moduleID)
{
    return reinterpret_cast<ReJITScript *>(pPrfCom->m_pTestClassInstance)->ModuleUnloadStarted(moduleID);
}

HRESULT ReJITScript_JITCompilationStarted(IPrfCom * pPrfCom, FunctionID functionID, BOOL fIsSafeToBlock)
{
    return reinterpret_cast<ReJITScript *>(pPrfCom->m_pTestClassInstance)->JITCompilationStarted(functionID, fIsSafeToBlock);
}

HRESULT ReJITScript_JITCompilationFinished(IPrfCom * pPrfCom, FunctionID functionID, HRESULT hrStatus, BOOL fIsSafeToBlock)
{
    return reinterpret_cast<ReJITScript *>(pPrfCom->m_pTestClassInstance)->JITCompilationFinished(functionID, hrStatus, fIsSafeToBlock);
}

HRESULT ReJITScript_JITInlining(IPrfCom * pPrfCom, FunctionID caller, FunctionID callee, BOOL * pfShouldInline)
{
    return reinterpret_cast<ReJITScript *>(pPrfCom->m_pTestClassInstance)->JITInlining(caller, callee, pfShouldInline);
}


// static
void ReJITScript::Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& testName)
{
    TestType testType;
    BOOL isAttach;
    if (testName == L"ReJITDeterministicScript")
    {
        testType = kRejitTestType_Deterministic;
        isAttach = FALSE;
    }
    else if (testName == L"ReJITDeterministicScript_Attach")
    {
        testType = kRejitTestType_Deterministic;
        isAttach = TRUE;
    }
    else if (testName == L"ReJITNondeterministicScript")
    {
        testType = kRejitTestType_Nondeterministic;
        isAttach = FALSE;
    }
    else if (testName == L"ReJITNondeterministicScript_Attach")
    {
        testType = kRejitTestType_Nondeterministic;
        isAttach = TRUE;
    }
    else
    {
        FAILURE(L"Test name '" << testName << L"' not recognized!");
        return;
    }



    // Create new test object
    ReJITScript * pReJITScript = new ReJITScript(pPrfCom, testType, isAttach);
    SET_CLASS_POINTER(pReJITScript);
    pReJITScript->InitializeCommon();

    pModuleMethodTable->FLAGS = 
              COR_PRF_MONITOR_MODULE_LOADS      
            | COR_PRF_MONITOR_ASSEMBLY_LOADS    
            | COR_PRF_MONITOR_APPDOMAIN_LOADS   
            | COR_PRF_MONITOR_JIT_COMPILATION   
            | COR_PRF_MONITOR_THREADS           
            | COR_PRF_ENABLE_REJIT              
            | COR_PRF_ENABLE_STACK_SNAPSHOT
            ;

#ifndef CORECLRTESTBUILD
    REGISTER_CALLBACK(VERIFY,                       ReJITScript_Verify);
#endif
    REGISTER_CALLBACK(REJITCOMPILATIONSTARTED,      ReJITScript_ReJITCompilationStarted);
    REGISTER_CALLBACK(GETREJITPARAMETERS,           ReJITScript_GetReJITParameters);
    REGISTER_CALLBACK(REJITCOMPILATIONFINISHED,     ReJITScript_ReJITCompilationFinished);
    REGISTER_CALLBACK(REJITERROR,                   ReJITScript_ReJITError);

    REGISTER_CALLBACK(APPDOMAINCREATIONSTARTED,     ReJITScript_AppDomainCreationStarted);
    REGISTER_CALLBACK(APPDOMAINSHUTDOWNFINISHED,    ReJITScript_AppDomainShutdownFinished);
    REGISTER_CALLBACK(MODULELOADFINISHED,           ReJITScript_ModuleLoadFinished);
    REGISTER_CALLBACK(MODULEUNLOADSTARTED,          ReJITScript_ModuleUnloadStarted);
    REGISTER_CALLBACK(JITCOMPILATIONSTARTED,        ReJITScript_JITCompilationStarted);
    REGISTER_CALLBACK(JITCOMPILATIONFINISHED,       ReJITScript_JITCompilationFinished);
    REGISTER_CALLBACK(JITINLINING,                  ReJITScript_JITInlining);
}


HRESULT RewriteIL(
    ICorProfilerInfo * pICorProfilerInfo,
    ICorProfilerFunctionControl * pICorProfilerFunctionControl,
    ModuleID moduleID,
    mdMethodDef methodDef,
    int nVersion,
    mdToken mdEnterProbeRef,
    mdToken mdExitProbeRef);

HRESULT SetILForManagedHelper(
    ICorProfilerInfo * pICorProfilerInfo,
    ModuleID moduleID,
    mdMethodDef mdHelperToAdd,
    mdMethodDef mdIntPtrExplicitCast,
    mdMethodDef mdPInvokeToCall);


// Return TRUE iff wszContainer ends with wszProspectiveEnding (case-insensitive)
BOOL ContainsAtEnd(wstring wszContainer, wstring wszProspectiveEnding);

// Quicker form of ContainsAtEnd when prospective ending is a constant string
#define ENDS_WITH(str, constant) \
    (StrCmpIns(constant, &str[wcslen(str) - ((sizeof(constant) / sizeof(constant[0])) - 1)]))


const int kcFunctionsToBatchForRejit = 1;


//---------------------------------------------------------------------------------------
//
// ReJITScript implementation
//

ReJITScript::ReJITScript(IPrfCom * pPrfCom, TestType testType, BOOL isAttach) :
    m_testType(testType),
    m_pProfilerInfo(NULL),
    m_pPrfCom(pPrfCom),
    m_functionIDToVersionMap(pPrfCom),
    m_moduleIDToInfoMap(pPrfCom),
    m_cShadowStackFrameInfos(0),
    m_cScriptEntries(0),
    m_iScriptEntryCur(0),
    m_iScriptLineNumberCur(0),
    m_fAtLeastOneSuccessfulRejitFinished(FALSE),
    m_cAppDomains(0),
    m_fInstrumentationHooksInSeparateAssembly(TRUE),
    m_fIsAttachScenario(isAttach),
    m_mdEnterPInvoke(mdTokenNil),
    m_mdExitPInvoke(mdTokenNil),
    m_mdEnter(mdTokenNil),
    m_mdExit(mdTokenNil),
    m_modidMscorlib(NULL),
    m_fStopAllThreadLoops(FALSE),
    m_cLoopingThreads(0),
    m_cRejitStarted(0),
    m_getReJITParametersDelayMs(0)
{

    HRESULT hr = pPrfCom->m_pInfo->QueryInterface(IID_ICorProfilerInfo6, (void**)&m_pProfilerInfo);
    if (FAILED(hr))
    {
        FAILURE(L"QueryInterface() for ICorProfilerInfo6 failed.");
    }

    memset(&m_rgScriptEntries, 0, sizeof(m_rgScriptEntries));
    memset(&m_rgShadowStackFrameInfo, 0, sizeof(m_rgShadowStackFrameInfo));

    // This is how the instrumentation calls that P/Invoke back into this
    // native DLL can find us.
    g_pReJITScript = this;
}


ReJITScript::~ReJITScript()
{
    //Log(L"ReJITScript::~ReJITScript called\n");
    //g_pCallbackObject = NULL;
}

HRESULT ReJITScript::InitializeCommon()
{
    HRESULT hr;

    wstring separate = ReadEnvironmentVariable(L"bpd_instrumentationhooksinseparateassembly");
	m_fInstrumentationHooksInSeparateAssembly = separate == L"1";
    TEST_DISPLAY(
        L"Instrumented code will call into hooks located in " <<
        (m_fInstrumentationHooksInSeparateAssembly ? L"ProfilerHelper.dll" : L"MSCORLIB.DLL"));

    wstring delay = ReadEnvironmentVariable(L"Rejit_GetReJITParameterDelay");
    if (delay.size() > 0)
    {
        m_getReJITParametersDelayMs = std::stoi(delay);
        TEST_DISPLAY(L"GetReJITParameterDelay " << m_getReJITParametersDelayMs);
    }

    hr = ReadScript();
    if (FAILED(hr))
    {
        TEST_FAILURE(L"Failed to read rejit script.  HRESULT=0x0x" << std::hex << hr);
        return hr;
    }

    if (m_fIsAttachScenario)
    {
        // On attach, System.Private.Corelib will already be loaded, so we can inject a
        // fake module loaded callback here.
        if (FAILED(AddCoreLib()))
        {
            TEST_FAILURE(L"Couldn't process System.Private.CoreLib after attach.");
        }
    }

    return S_OK;
}


HRESULT ReJITScript::Verify()
{
    if (m_iScriptEntryCur < m_cScriptEntries)
    {
        TEST_FAILURE(L"Did not finish executing all lines in rejit script.  Next 0-based command index to exec: '" << m_iScriptEntryCur << L"'; Total count of commands: '" << m_cScriptEntries << L"'");
    }

    if (!m_fAtLeastOneSuccessfulRejitFinished)
    {
        TEST_FAILURE(L"No successful rejits were completed.");
    }

    // For CoreCLR, we're on our own to announce whether the test passed or failed.  (On
    // desktop, BPD does this for us.)
#ifdef CORECLRTESTBUILD
    m_pPrfCom->m_wosDisplayStream << L"\n";
    if (m_pPrfCom->m_ulError == 0)
    {
        m_pPrfCom->m_wosDisplayStream << L"TEST PASSED";
    }    
    else
    {
        m_pPrfCom->m_wosDisplayStream << L"TEST FAILED\n";
        m_pPrfCom->m_wosDisplayStream << L"Search \"Test Failure\" in the log file for details.";
    }
    m_pPrfCom->Display(m_pPrfCom->m_wosDisplayStream);
#endif

    return S_OK;
} 



HRESULT ReJITScript::TestCatchUp()
{
    //printf("Testing EnumJITedFunctions.\n");

    // Try the new APIs...
    ICorProfilerFunctionEnum * funcEnum = NULL;
    ULONG cnt = 0;
    ULONG cFuncs = 0;
    COR_PRF_FUNCTION funcs[13];
    ULONG j = 0;

    HRESULT hr = m_pProfilerInfo->EnumJITedFunctions(&funcEnum);
    if (FAILED(hr))
    {
        printf("EnumJITedFunctions failed with HRESULT 0x%08x\n", hr);
        return S_FALSE;
    }

    if (funcEnum == NULL)
    {
        printf("EnumJITedFunctions returned a NULL pointer\n");
        return S_FALSE;
    }

    if (hr != S_OK)
    {
        printf("EnumJITedFunctions returned unexpected HRESULT 0x%08x\n", hr);
        goto FAILED;
    }

    hr = funcEnum->Next(1, NULL, NULL);
    if (hr != E_INVALIDARG)
    {
        printf("ICorProfilerFunctionEnum::Next returned unexpected HRESULT 0x%08x\n", hr);
        goto FAILED;
    }

    hr = funcEnum->Next(0, NULL, NULL);
    if (hr != S_OK)
    {
        printf("ICorProfilerFunctionEnum::Next returned unexpected HRESULT 0x%08x\n", hr);
        goto FAILED;
    }

    hr = funcEnum->GetCount(&cnt);
    if (hr != S_OK)
    {
        printf("ICorProfilerFunctionEnum::GetCount returned unexpected HRESULT 0x%08x\n", hr);
        goto FAILED;
    }

    do {
        hr = funcEnum->Next(13, funcs, &cFuncs);
        if (FAILED(hr))
        {
            printf("ICorProfilerFunctionEnum::Next returned unexpected HRESULT 0x%08x\n", hr);
            goto FAILED;
        }

        for (ULONG i = 0; i < cFuncs; ++i) {
            printf("Method #%d: %d of %d FunctionID: %p, ReJITID: %p\n", ++j, i + 1, cFuncs, reinterpret_cast<void *>(funcs[i].functionId), reinterpret_cast<void *>(funcs[i].reJitId));
            if (funcs[i].reJitId == 0)
            {
                TestGetReJITIDs(funcs[i].functionId);
            }
        }
    } while (hr == S_OK);

    if (j != cnt)
    {
        printf("ICorProfilerJITedFunctionsEnum returned %d functions instead of %d\n", j, cnt);
    }
    else
    {
        printf("ICorProfilerJITedFunctionsEnum succeeded with %d functions\n", j);
    }

    funcEnum->Release();
    return S_OK;

FAILED:
    funcEnum->Release();
    return S_FALSE;
} 

HRESULT ReJITScript::AppDomainCreationStarted(AppDomainID appDomainID)
{
//    Log(L"AppDomainCreationStarted '%x'\n", appDomainID);
    ++m_cAppDomains;
    return S_OK;
} 



HRESULT ReJITScript::AppDomainShutdownFinished(AppDomainID appDomainID,
                                                      HRESULT hrStatus)
{
//    Log(L"AppDomainShutdownFinished '%x'\n", appDomainID);
    --m_cAppDomains;
    return S_OK;
} 



HRESULT ReJITScript::ModuleLoadFinished(ModuleID moduleID,
                                               HRESULT hrStatus)
{   
    LPCBYTE pbBaseLoadAddr;
    PROFILER_WCHAR wszNameTemp[300];
    PLATFORM_WCHAR wszName[300];
    ULONG cchNameIn = dimensionof(wszName);
    ULONG cchNameOut;
    AssemblyID assemblyID;
    DWORD dwModuleFlags;
    HRESULT hr = m_pProfilerInfo->GetModuleInfo2(
        moduleID,
        &pbBaseLoadAddr,
        cchNameIn,
        &cchNameOut,
        wszNameTemp,
        &assemblyID,
        &dwModuleFlags);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"GetModuleInfo2 failed for ModuleID '0x" << std::hex << moduleID << L"', hr = 0x" << std::hex << hr);
        return S_OK;
    }

    ConvertProfilerWCharToPlatformWChar(wszName, 300, wszNameTemp, 300);

    AppDomainID appDomainID;
    ModuleID modIDDummy;
    hr = m_pProfilerInfo->GetAssemblyInfo(
        assemblyID,
        0,          // cchName,
        NULL,       // pcchName,
        NULL,       // szName[] ,
        &appDomainID,
        &modIDDummy);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"GetAssemblyInfo failed for assemblyID '0x" << std::hex << assemblyID << L"', hr = 0x" << std::hex << hr);
        return S_OK;
    }

    PROFILER_WCHAR wszAppDomainNameTemp[200];
    PLATFORM_WCHAR wszAppDomainName[200];
    ULONG cchAppDomainName;
    ProcessID areYouKiddingMe;
    BOOL fShared = FALSE;

    hr = m_pProfilerInfo->GetAppDomainInfo(
        appDomainID,
        dimensionof(wszAppDomainName),
        &cchAppDomainName,
        wszAppDomainNameTemp,
        &areYouKiddingMe);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"GetAppDomainInfo on '0x" << std::hex << appDomainID << L"' failed with 0x" << std::hex << hr);
        return S_OK;
    }

    ConvertProfilerWCharToPlatformWChar(wszAppDomainName, 200, wszAppDomainNameTemp, 200);

    fShared = (wcscmp(wszAppDomainName, L"EE Shared Assembly Repository") == 0);

    TEST_DISPLAY(L"ModuleLoadFinished: '" << wszName << L"', ModuleID='0x" << std::hex << moduleID << L"', LoadAddress='0x" << std::hex << pbBaseLoadAddr << 
        L"', AppDomainID='0x" << std::hex << appDomainID << L"', ADName='" << wszAppDomainName << L"', " << fShared ? L"SHARED" : L"UNSHARED");
    /*
    if ((dwModuleFlags & COR_PRF_MODULE_DISK) != 0)
        wprintf(L"DISK ");
    if ((dwModuleFlags & COR_PRF_MODULE_NGEN) != 0)
        wprintf(L"NGEN ");
    if ((dwModuleFlags & COR_PRF_MODULE_FLAT_LAYOUT) != 0)
        wprintf(L"FLAT ");
    if ((dwModuleFlags & COR_PRF_MODULE_DYNAMIC) != 0)
        wprintf(L"DYNAMIC ");
    if ((dwModuleFlags & COR_PRF_MODULE_COLLECTIBLE) != 0)
        wprintf(L"COLLECTIBLE ");
    if ((dwModuleFlags & COR_PRF_MODULE_RESOURCE) != 0)
        wprintf(L"RESOURCE ");
    wprintf(L"'\n");
    */

    BOOL fModuleInScript = IsModuleInScript(wszName);
    BOOL fPumpHelperMethodsIntoThisModule = FALSE;

    if (ContainsAtEnd(wszName, L"mscorlib.dll") || ContainsAtEnd(wszName, L"mscorlib.ni.dll") ||
        ContainsAtEnd(wszName, L"System.Private.CoreLib.dll"))
    {
        TEST_DISPLAY(L"Found mscorlib library (" << wszName << ") with moduleId = " << moduleID);
        m_modidMscorlib = moduleID;
        if (!m_fInstrumentationHooksInSeparateAssembly)
        {
            fPumpHelperMethodsIntoThisModule = TRUE;
        }
    }

    
    // Grab metadata interfaces 

    COMPtrHolder<IMetaDataEmit> pEmit;
    {
        COMPtrHolder<IUnknown> pUnk;
        hr = m_pProfilerInfo->GetModuleMetaData(moduleID, ofWrite, IID_IMetaDataEmit, &pUnk);
        if (FAILED(hr))
        {
            TEST_FAILURE(L"GetModuleMetaData failed for IID_IMetaDataEmit in ModuleID '0x" << std::hex << moduleID << L"' (" << wszName << L")");
        }
        hr = pUnk->QueryInterface(IID_IMetaDataEmit, (LPVOID *) &pEmit);
        if (FAILED(hr))
        {
            TEST_FAILURE(L"Failed QI'ing for IID_IMetaDataEmit for ModuleID '0x" << std::hex << moduleID << L"' (" << wszName << L")");
        }
    }

    COMPtrHolder<IMetaDataImport> pImport;
    {
        COMPtrHolder<IUnknown> pUnk;
        hr = m_pProfilerInfo->GetModuleMetaData(moduleID, ofRead, IID_IMetaDataImport, &pUnk);
        if (FAILED(hr))
        {
            TEST_FAILURE(L"GetModuleMetaData failed for ModuleID '" << HEX(moduleID) << L"' (" << wszName << L")");
        }
        hr = pUnk->QueryInterface(IID_IMetaDataImport, (LPVOID *)&pImport);
        if (FAILED(hr))
        {
            TEST_FAILURE(L"Failed QI'ing for IID_IMetaDataImport for ModuleID '" << HEX(moduleID) << L"' (" << wszName << L")");
        }
    }

    COMPtrHolder<IMetaDataAssemblyImport> pAssemblyImport;
    hr = m_pProfilerInfo->GetModuleMetaData(moduleID, ofRead, IID_IMetaDataAssemblyImport, (IUnknown**)&pAssemblyImport);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"GetModuleMetaData failed for ModuleID '" << HEX(moduleID) << L"' (" << wszName << L")");
    }
    COMPtrHolder<IMetaDataAssemblyEmit> pAssemblyEmit;
    hr = m_pProfilerInfo->GetModuleMetaData(moduleID, ofRead, IID_IMetaDataAssemblyEmit, (IUnknown**)&pAssemblyEmit);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"GetModuleMetaData failed for ModuleID '" << HEX(moduleID) << L"' (" << wszName << L")");
    }

    if (fPumpHelperMethodsIntoThisModule)
    {
        AddHelperMethodDefs(pImport, pEmit);
    }

    if (ContainsAtEnd(wszName, L"System.Runtime.dll"))
    {
        if (!m_fInstrumentationHooksInSeparateAssembly)
        {
            AddHelperTypeForwarders(pAssemblyImport, pAssemblyEmit);
        }
    }

    if (!fModuleInScript)
    {
        // Not rejitting methods in this module, so no need to add assembly
        // references
        return S_OK;
    }

    // Store module info in our list

    TEST_DISPLAY(L"Adding module to list...");

    ModuleInfo moduleInfo = {{0}};
    wcscpy(moduleInfo.m_wszModulePath, wszName);

    if (dimensionof(moduleInfo.m_wszModulePath) < wcslen(wszName))
    {
        TEST_FAILURE(L"Failed to store module path '" << wszName << L"'");
    }

    // Store metadata reader alongside the module in the list.
    moduleInfo.m_pImport = pImport;
    moduleInfo.m_pImport->AddRef();

    moduleInfo.m_pMethodDefToLatestVersionMap = new MethodDefToLatestVersionMap(m_pPrfCom);

    if (fPumpHelperMethodsIntoThisModule)
    {
        // We're operating on mscorlib and the helper methods are being
        // pumped directly into it.  So we reference the helpers via
        // methodDefs, not memberRefs
        assert(m_mdEnter != mdTokenNil);
        assert(m_mdExit != mdTokenNil);
        moduleInfo.m_mdEnterProbeRef = m_mdEnter;
        moduleInfo.m_mdExitProbeRef = m_mdExit;
    }
    else
    {
        // Add the references to our helper methods.

        COMPtrHolder<IMetaDataAssemblyEmit> pAssemblyEmit;
        {
            COMPtrHolder<IUnknown> pUnk;
            hr = m_pProfilerInfo->GetModuleMetaData(moduleID, ofWrite, IID_IMetaDataAssemblyEmit, &pUnk);
            if (FAILED(hr))
            {
                TEST_FAILURE(L"GetModuleMetaData failed for IID_IMetaDataAssemblyEmit in ModuleID '0x" << std::hex << moduleID << L"' (" << wszName << L")");
            }
            hr = pUnk->QueryInterface(IID_IMetaDataAssemblyEmit, (LPVOID *) &pAssemblyEmit);
            if (FAILED(hr))
            {
                TEST_FAILURE(L"Failed QI'ing for IID_IMetaDataAssemblyEmit for ModuleID '0x" << std::hex << moduleID << L"' (" << wszName << L")");
            }
        }
        COMPtrHolder<IMetaDataAssemblyImport> pAssemblyImport;
        {
            COMPtrHolder<IUnknown> pUnk;
            hr = m_pProfilerInfo->GetModuleMetaData(moduleID, ofRead, IID_IMetaDataAssemblyImport, &pUnk);
            if (FAILED(hr))
            {
                TEST_FAILURE(L"GetModuleMetaData failed for IID_IMetaDataAssemblyImport in ModuleID '0x" << std::hex << moduleID << L"' (" << wszName << L")");
            }
            hr = pUnk->QueryInterface(IID_IMetaDataAssemblyImport, (LPVOID *) &pAssemblyImport);
            if (FAILED(hr))
            {
                TEST_FAILURE(L"Failed QI'ing for IID_IMetaDataAssemblyImport for ModuleID '0x" << std::hex << moduleID << L"' (" << wszName << L")");
            }
        }
        AddMemberRefs(pAssemblyImport, pAssemblyEmit, pEmit, &moduleInfo);
    }

    // Append to the list!
    m_moduleIDToInfoMap.Update(moduleID, moduleInfo);
    TEST_DISPLAY(L"Successfully added module to list.");

    // If we already rejitted functions in other modules with a matching path, then
    // pre-rejit those functions in this module as well.  This takes care of the case
    // where we rejitted functions in a module loaded in one domain, and we just now
    // loaded the same module (unshared) into another domain.  We must explicitly ask to
    // rejit those functions in this domain's copy of the module, since it's technically
    // a different ModuleID.
    
    // Estimate the max amount of space we'll need by counting how many script entries
    // we've already executed times number of modules loaded. This will also count the
    // non-REJIT script entries, so we should be overestimating.
    UINT cFunctionsToRejitMax = (m_iScriptEntryCur + 1) * (UINT) m_moduleIDToInfoMap.GetCount();
    ModuleID * rgModuleIDs  = new ModuleID[cFunctionsToRejitMax];
    mdToken *  rgMethodDefs = new mdToken [cFunctionsToRejitMax];
    UINT iFunctionsToRejit = 0;    
    
    // Find all modules matching the name in this script entry
    {
        ModuleIDToInfoMap::LockHolder lockHolder(&m_moduleIDToInfoMap);

        // Get the methodDef map for the Module just loaded handy
        MethodDefToLatestVersionMap * pMethodDefToLatestVersionMap = 
            m_moduleIDToInfoMap.Lookup(moduleID).m_pMethodDefToLatestVersionMap;
        assert(pMethodDefToLatestVersionMap != NULL);

        ModuleIDToInfoMap::Const_Iterator iterator;
        for (iterator = m_moduleIDToInfoMap.Begin();
             iterator != m_moduleIDToInfoMap.End();
             iterator++)
        {
            // Skip the entry we just added for this module
            if (iterator->first == moduleID)
            {
                continue;
            }

            const ModuleInfo * pModInfo = &(iterator->second);
            wstring wszModulePathCur = &(pModInfo->m_wszModulePath[0]);

            // We only care if the full path of the module from our internal
            // module list == full path of module just loaded
            wstring nameTemp(wszName);
            transform(nameTemp.begin(), nameTemp.end(), nameTemp.begin(), towlower);
            transform(wszModulePathCur.begin(), wszModulePathCur.end(), wszModulePathCur.begin(), towlower);

            if (wszModulePathCur != nameTemp)
            {
                continue;
            }

            // The module is a match!
            MethodDefToLatestVersionMap::Const_Iterator iterMethodDef;
            for (iterMethodDef = pModInfo->m_pMethodDefToLatestVersionMap->Begin();
                 iterMethodDef != pModInfo->m_pMethodDefToLatestVersionMap->End();
                 iterMethodDef++)
            {
                if (iterMethodDef->second == 0)
                {
                    // We have reverted this method (and not rejitted it
                    // since).  So do not set up a pre-rejit for it in this
                    // module.
                    continue;
                }

                // NOTE: We may have already added this methodDef if it was rejitted in
                // multiple modules.  That means the array will have dupes.  This will
                // test out how the CLR deals with dupes passed to RequesteReJIT().
                rgModuleIDs[iFunctionsToRejit] = moduleID;
                rgMethodDefs[iFunctionsToRejit] = iterMethodDef->first;
            
                // Remember the latest version number for this mdMethodDef
                pMethodDefToLatestVersionMap->Update(iterMethodDef->first, iterMethodDef->second);
                
                iFunctionsToRejit++;
                
                // If this assert fires, we somehow need to pre-rejit more functions in
                // this module than all the functions we've rejitted in other modules so
                // far...?
                assert(iFunctionsToRejit < cFunctionsToRejitMax);
            }
        }
    }
    
    // iFunctionsToRejit is pointing at the next 0-based index to fill in, in the
    // module/method arrays, so it equals the count of the array entries already filled
    // in.
    UINT cFunctionsToRejit = iFunctionsToRejit;
    assert(cFunctionsToRejit <= cFunctionsToRejitMax);

    if (cFunctionsToRejit > 0)
    {
        CallRequestReJIT(cFunctionsToRejit, rgModuleIDs, rgMethodDefs);
    }

    FinishWaitIfAppropriate(moduleID, mdTokenNil, mdTokenNil, 0, fShared, 0, ScriptEntry::kWaitModule);

    return S_OK;
}

void ReJITScript::AddHelperTypeForwarders(IMetaDataAssemblyImport * pAssemblyImport, IMetaDataAssemblyEmit * pAssemblyEmit)
{
    assert(!m_fInstrumentationHooksInSeparateAssembly);
    TEST_DISPLAY(L"Adding type forwarders to System.Runtime metadata for managed helper probes");

    // Find existing assemblyRef to mscorlib
    HRESULT hr = S_OK;
    HCORENUM hEnum = NULL;
    mdAssemblyRef rgAssemblyRefs[20];
    ULONG cAssemblyRefsReturned = 0;
    mdAssemblyRef assemblyRef = mdTokenNil;

    while (TRUE)
    {
        hr = pAssemblyImport->EnumAssemblyRefs(
            &hEnum,
            rgAssemblyRefs,
            20,
            &cAssemblyRefsReturned);
        if (FAILED(hr))
        {
            TEST_FAILURE(L"EnumAssemblyRefs failed with HRESULT = " << HEX(hr));
            return;
        }

        if (cAssemblyRefsReturned == 0)
        {
            TEST_FAILURE(L"Could not find an AssemblyRef to mscorlib");
            return;
        }

        if (FindMscorlibReference(pAssemblyImport, rgAssemblyRefs, cAssemblyRefsReturned, &assemblyRef))
            break;
    }

    pAssemblyImport->CloseEnum(hEnum);
    hEnum = NULL;

    mdExportedType tdExport;
    hr = pAssemblyEmit->DefineExportedType(k_wszHelpersContainerTypeProfiler, assemblyRef, 0, tdForwarder, &tdExport);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"DefineExportedType(" << k_wszHelpersContainerType << L") failed with hr=" << HEX(hr));
        return;
    }
}

void ReJITScript::AddHelperMethodDefs(IMetaDataImport * pImport, IMetaDataEmit * pEmit)
{
    assert(!m_fInstrumentationHooksInSeparateAssembly);
    assert(m_testType == kRejitTestType_Deterministic);

    TEST_DISPLAY(L"Adding methodDefs to mscorlib metadata for managed helper probes");

    HRESULT hr;

    // The helpers will need to call into System.IntPtr::op_Explicit(int64),
    // so get its methodDef now
    mdTypeDef tdSystemIntPtr;
    hr = pImport->FindTypeDefByName(
        PROFILER_STRING("System.IntPtr"),
        mdTypeDefNil,
        &tdSystemIntPtr);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"FindTypeDefByName(System.IntPtr) failed with hr=0x" << std::hex << hr);
        return;
    }
    COR_SIGNATURE intPtrOpExplicitSignature[] = 
    {
        IMAGE_CEE_CS_CALLCONV_DEFAULT,
        1,                      // 1 argument
        ELEMENT_TYPE_I,         // return type == native int
        ELEMENT_TYPE_I8,        // argument type = int64
    };
    hr = pImport->FindMethod(
        tdSystemIntPtr,
        PROFILER_STRING("op_Explicit"),
        intPtrOpExplicitSignature,
        sizeof(intPtrOpExplicitSignature),
        &m_mdIntPtrExplicitCast);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"FindMethod(System.IntPtr.op_Explicit(int64)) failed with hr=0x" << std::hex << hr);
        return;
    }

    mdTypeDef tdObject;
    hr = pImport->FindTypeDefByName(
        PROFILER_STRING("System.Object"),
        mdTypeDefNil,
        &tdObject);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"FindTypeDefByName( System.Object" <<
            L") failed with hr=0x" << std::hex << hr);
        return;
    }

    // Put the managed helpers into this pre-existing mscorlib type
    mdTypeDef tdHelpersContainer;
    mdToken tkImplements = NULL;
    hr = pEmit->DefineTypeDef(
        k_wszHelpersContainerTypeProfiler,
        tdPublic,
        tdObject,
        &tkImplements,
        &tdHelpersContainer
        );
    if (FAILED(hr))
    {
        TEST_FAILURE(L"DefineTypeDef(" << k_wszHelpersContainerType <<
            L") failed with hr=0x" << std::hex << hr);
        return;
    }

    // Get a dummy method implementation RVA (CLR doesn't like you passing 0).  Pick a
    // ctor on the same type.
    COR_SIGNATURE ctorSignature[] = 
    {
        IMAGE_CEE_CS_CALLCONV_HASTHIS, //__stdcall
        0,
        ELEMENT_TYPE_VOID 
    };

    mdMethodDef mdCtor = NULL;
    hr = pImport->FindMethod(
        tdObject,
        PROFILER_STRING(".ctor"),
        ctorSignature,
        sizeof(ctorSignature),
        &mdCtor);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"FindMethod(" << k_wszHelpersContainerType <<
            L"..ctor) failed with hr=0x" << std::hex << hr);
        return;
    }

    ULONG rvaCtor;
    hr = pImport->GetMethodProps(
        mdCtor,
        NULL,           // Put method's class here. 
        NULL,           // Put method's name here.  
        0,              // Size of szMethod buffer in wide chars.   
        NULL,           // Put actual size here 
        NULL,           // Put flags here.  
        NULL,           // [OUT] point to the blob value of meta data   
        NULL,           // [OUT] actual size of signature blob  
        &rvaCtor,
        NULL);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"GetMethodProps(" << k_wszHelpersContainerType <<
            L"..ctor) failed with hr=0x" << std::hex << hr);
        return;
    }

    // Generate reference to unmanaged profiler extension DLL
    mdModuleRef modrefNativeExtension;
    hr = pEmit->DefineModuleRef(
#ifdef _WIN32
        PROFILER_STRING("TestProfiler.dll"),
#elif defined(LINUX)
        PROFILER_STRING("libTestProfiler.so"),
#else
        PROFILER_STRING("libTestProfiler.dylib"),
#endif
        &modrefNativeExtension);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"DefineModuleRef against the native extension DLL failed with hr=0x" << std::hex << hr);
        return;
    }

    // Generate the PInvokes into the profiler extension DLL
    AddPInvoke(pEmit, tdHelpersContainer, L"NtvEnteredFunction", modrefNativeExtension, &m_mdEnterPInvoke);
    AddPInvoke(pEmit, tdHelpersContainer, L"NtvExitedFunction", modrefNativeExtension, &m_mdExitPInvoke);

    // Generate the SafeCritical managed methods which call the PInvokes
    mdMethodDef mdSafeCritical;
    GetSecuritySafeCriticalAttributeToken(pImport, &mdSafeCritical);
    AddManagedHelperMethod(pEmit, tdHelpersContainer, k_wszEnteredFunctionProbeName.c_str(), m_mdEnterPInvoke, rvaCtor, mdSafeCritical, &m_mdEnter);
    AddManagedHelperMethod(pEmit, tdHelpersContainer, k_wszExitedFunctionProbeName.c_str(), m_mdExitPInvoke, rvaCtor, mdSafeCritical, &m_mdExit);
}

void ReJITScript::AddPInvoke(
    IMetaDataEmit * pEmit, 
    mdTypeDef td, 
    const PLATFORM_WCHAR *wszName,         // Name of both the P/Invoke method, and the target native method
    mdModuleRef modrefTarget,
    mdMethodDef * pmdPInvoke)
{
    assert(m_testType == kRejitTestType_Deterministic);
    HRESULT hr;

    //COR_SIGNATURE representation
    //   Calling convention
    //   Number of Arguments
    //   Return type
    //   Argument type
    //   ...

    COR_SIGNATURE newMethodSignature[] = { IMAGE_CEE_CS_CALLCONV_DEFAULT,   //__stdcall
                                           3,
                                           ELEMENT_TYPE_VOID,
                                           ELEMENT_TYPE_I,                  // ModuleID
                                           ELEMENT_TYPE_U4,                 // mdMethodDef token
                                           ELEMENT_TYPE_I4                  // Rejit version number
                                        };

    PROFILER_WCHAR wszNameTemp[LONG_LENGTH];
    ConvertPlatformWCharToProfilerWChar(wszNameTemp, LONG_LENGTH, wszName, wcslen(wszName) + 1);

    hr = pEmit->DefineMethod(
        td,
        wszNameTemp,
        ~mdAbstract & (mdStatic | mdPublic | mdPinvokeImpl),
        newMethodSignature,
        sizeof(newMethodSignature),
        0,
        miPreserveSig,
        pmdPInvoke);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"Failed in DefineMethod when creating P/Invoke method '" << wszName << 
            L"'.  hr=0x" << std::hex << hr);
        return;
    }

    hr = pEmit->DefinePinvokeMap(
        *pmdPInvoke,
        pmCallConvStdcall | pmNoMangle,
        wszNameTemp,
        modrefTarget);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"Failed in DefinePinvokeMap when creating P/Invoke method '" << wszName <<
            L"'.  hr=0x" << std::hex << hr);
        return;
    }
}

void ReJITScript::GetSecuritySafeCriticalAttributeToken(
    IMetaDataImport * pImport,
    mdMethodDef * pmdSafeCritical)
{
    assert(m_testType == kRejitTestType_Deterministic);
    HRESULT hr;

    mdTypeDef tdSafeCritical;
    hr = pImport->FindTypeDefByName(
        PROFILER_STRING("System.Security.SecuritySafeCriticalAttribute"),
        mdTokenNil,
        &tdSafeCritical);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"FindTypeDefByName(System.Security.SecuritySafeCriticalAttribute) failed with hr=0x" << std::hex << hr);
        return;
    }

    COR_SIGNATURE sigSafeCriticalCtor[] = 
    {
        IMAGE_CEE_CS_CALLCONV_HASTHIS,
        0x00,                               // number of arguments == 0
        ELEMENT_TYPE_VOID,                  // return type == void
    };

    hr = pImport->FindMember(
        tdSafeCritical,
        PROFILER_STRING(".ctor"),
        sigSafeCriticalCtor,
        sizeof(sigSafeCriticalCtor),
        pmdSafeCritical);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"FindMember(System.Security.SecuritySafeCriticalAttribute..ctor) failed with hr=0x" << std::hex << hr);
        return;
    }
}

void ReJITScript::AddManagedHelperMethod(
    IMetaDataEmit * pEmit, 
    mdTypeDef td, 
    const PLATFORM_WCHAR *wszName,
    mdMethodDef mdTargetPInvoke,
    ULONG rvaDummy,
    mdMethodDef mdSafeCritical,
    mdMethodDef * pmdHelperMethod)
{
    assert(m_testType == kRejitTestType_Deterministic);
    HRESULT hr;

    COR_SIGNATURE newMethodSignature[] = 
    {
        IMAGE_CEE_CS_CALLCONV_DEFAULT,  //__stdcall
        3,
        ELEMENT_TYPE_VOID,              // returns void
#ifdef _X86_
        ELEMENT_TYPE_U4,                // ModuleID
#elif defined(_AMD64_) || defined(__ARM_ARCH_ISA_A64) || defined (_M_ARM64)
        ELEMENT_TYPE_U8,                // ModuleID
#else
#error REJIT TEST NOT IMPLEMENTED FOR YOUR ARCHITECTURE
#endif
        ELEMENT_TYPE_U4,                // mdMethodDef token
        ELEMENT_TYPE_I4,                // Rejit version number
    };

    PROFILER_WCHAR wszNameTemp[STRING_LENGTH];
    ConvertPlatformWCharToProfilerWChar(wszNameTemp, STRING_LENGTH, wszName, wcslen(wszName) + 1);

    hr = pEmit->DefineMethod(
        td,
        wszNameTemp,
        mdStatic | mdPublic,
        newMethodSignature,
        sizeof(newMethodSignature),
        rvaDummy,
        miIL | miNoInlining,
        pmdHelperMethod);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"Failed in DefineMethod when creating managed helper method '" << wszName << 
            L"'.  hr=0x" << std::hex << hr);
        return;
    }

    mdToken tkCustomAttribute;
    hr = pEmit->DefineCustomAttribute(
        *pmdHelperMethod,
        mdSafeCritical,
        NULL,          //Blob, contains constructor params in this case none
        0,             //Size of the blob
        &tkCustomAttribute);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"Failed in DefineCustomAttribute when applying SecuritySafeCritical to new managed helper method '" << wszName << 
            L"'.  hr=0x" << std::hex << hr);
        return;
    }
}

HRESULT ReJITScript::ModuleUnloadStarted(ModuleID moduleID)
{
    TEST_DISPLAY(L"ModuleUnloadStarted: ModuleID='0x" << std::hex << moduleID << L"'.");

    {
        ModuleIDToInfoMap::LockHolder lockHolder(&m_moduleIDToInfoMap);

        ModuleInfo moduleInfo;
        if (m_moduleIDToInfoMap.LookupIfExists(moduleID, &moduleInfo))
        {
            TEST_DISPLAY(L"Module found in list.  Removing...");
            m_moduleIDToInfoMap.Erase(moduleID);
        }
        else
        {
            TEST_DISPLAY(L"Module not found in list.  Do nothing.");
        }
    }

    return S_OK;
}


BOOL ReJITScript::IsModuleInScript(const PLATFORM_WCHAR *wszModulePath)
{
    for (DWORD i=0; i < m_cScriptEntries; i++)
    {
        if (ContainsAtEnd(wszModulePath, m_rgScriptEntries[i].wszModule))
            return TRUE;
    }

    return FALSE;
}


HRESULT ReJITScript::JITCompilationStarted(FunctionID functionID, BOOL fIsSafeToBlock)
{
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
        TEST_FAILURE(L"GetFunctionInfo for FunctionID '0x" << std::hex << functionID << L"' failed.  HRESULT = 0x" << std::hex << hr << L".");
        return S_OK;
    }

    if ((moduleID == m_modidMscorlib) &&
        ((methodDef == m_mdEnter) || (methodDef == m_mdExit)))
    {
        SetILFunctionBodyForManagedHelper(moduleID, methodDef);
    }

    return S_OK;
}

void ReJITScript::SetILFunctionBodyForManagedHelper(ModuleID moduleID, mdMethodDef methodDef)
{
    assert(m_testType == kRejitTestType_Deterministic);
    assert(!m_fInstrumentationHooksInSeparateAssembly);
    assert(moduleID == m_modidMscorlib);
    assert((methodDef == m_mdEnter) || (methodDef == m_mdExit));

    HRESULT hr = SetILForManagedHelper(
        m_pProfilerInfo,
        moduleID,
        methodDef,
        m_mdIntPtrExplicitCast,
        (methodDef == m_mdEnter) ? m_mdEnterPInvoke : m_mdExitPInvoke);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"SetILForManagedHelper(0x" << std::hex << methodDef << L"--" <<
            ((methodDef == m_mdEnter) ? L"enter" : L"exit") << 
            L" mgd helper) failed with hr=0x" << std::hex << hr);
        return;
    }
}


HRESULT ReJITScript::JITCompilationFinished(FunctionID functionID, HRESULT hrStatus, BOOL fIsSafeToBlock)
{
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
        TEST_FAILURE(L"GetFunctionInfo for FunctionID '0x" << std::hex << functionID << L"' failed.  HRESULT = 0x" << std::hex << hr << L".");
    }

    FinishWaitIfAppropriate(moduleID, methodDef, mdTokenNil, 0, FALSE, 0, ScriptEntry::kWaitJit);
    return S_OK;
}

HRESULT ReJITScript::JITInlining(FunctionID caller, FunctionID callee, BOOL * pfShouldInline)
{
    *pfShouldInline = TRUE;

    HRESULT hr;
    mdToken methodDefCaller;
    ClassID classIDCaller;
    ModuleID moduleIDCaller;
    mdToken methodDefCallee;
    ClassID classIDCallee;
    ModuleID moduleIDCallee;

    hr = m_pProfilerInfo->GetFunctionInfo(
        caller,
        &classIDCaller,
        &moduleIDCaller,
        &methodDefCaller);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"GetFunctionInfo for FunctionID '0x" << std::hex << caller << L"' failed.  HRESULT = 0x" << std::hex << hr << L".");
        return S_OK;
    }

    hr = m_pProfilerInfo->GetFunctionInfo(
        callee,
        &classIDCallee,
        &moduleIDCallee,
        &methodDefCallee);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"GetFunctionInfo for FunctionID '0x" << std::hex << callee << L"' failed.  HRESULT = 0x" << std::hex << hr << L".");
        return S_OK;
    }

    // To simplify scripts, the rejit extension module only supports waiting for inlining
    // that occurs between two methods of the same class
    if ((moduleIDCaller != moduleIDCallee) || (classIDCaller != classIDCallee))
        return S_OK;

    FinishWaitIfAppropriate(moduleIDCaller, methodDefCaller, methodDefCallee, 0, FALSE, 0, ScriptEntry::kWaitInline);
    return S_OK;
}


/* static */
DWORD WINAPI ReJITScript::TestReJitOrRevertStatic(LPVOID pParameter)
{
    ReJITScript * pCallback = ((ReJitTestInfo*)pParameter)->m_pCallback;
    int iCommandFirst = ((ReJitTestInfo*)pParameter)->m_iCommandFirst;
    int iCommandLast = ((ReJitTestInfo*)pParameter)->m_iCommandLast;

    pCallback->TestReJitOrRevert(iCommandFirst, iCommandLast);
    return 1;
}


vector<COR_PRF_METHOD> ZipMethodsAndModules(UINT count, ModuleID * moduleIDs, mdMethodDef * methodIDs)
{
    vector<COR_PRF_METHOD> result;
    result.reserve(count);
    for (UINT i = 0; i < count; i++)
    {
        result.push_back({ moduleIDs[i], methodIDs[i] });
    }
    return result;
}

void ReJITScript::TestReJitOrRevert(int iCommandFirst, int iCommandLast)
{
    assert(iCommandFirst <= iCommandLast);
    ScriptEntry::Command cmd = m_rgScriptEntries[iCommandFirst].command;

    // Estimate the max amount of space we'll need by taking the number of
    // rejit / revert requests, and multiplying by the number of loaded
    // AppDomains (assuming each function will be rejitted / reverted in every
    // AppDomain), and adding a buffer. This is not perfectly accurate, as
    // AppDomains may be loading / unloading while we build up the arrays to
    // send to the profiling API. Thus, this test is primarily meant for
    // deterministic (not highly concurrent) test applications
    UINT cFunctionsMax = ((iCommandLast - iCommandFirst) + 1) * (m_cAppDomains + 10);
    ModuleID * moduleIds = new ModuleID[cFunctionsMax];
    mdToken *  methodIds = new mdToken [cFunctionsMax];

    UINT iFunctions = 0;
    DWORD dwSleepMs = 0;

    // If we're doing a rejit/revert thread loop, just grab the first SleepMs value, and
    // use that to sleep between calling rejit.
    if ((cmd == ScriptEntry::kRejitThreadLoop) ||
        (cmd == ScriptEntry::kRevertThreadLoop))
    {
        dwSleepMs = m_rgScriptEntries[iCommandFirst].dwSleepMs;
    }

    // Iterate through script entries.  For each one find all matching modules, and add
    // them to the batch list we'll send to the profiling API
    for (int iScriptEntry = iCommandFirst; iScriptEntry <= iCommandLast; iScriptEntry++)
    {
        ScriptEntry * pEntry = &(m_rgScriptEntries[iScriptEntry]);
        assert(pEntry->command == cmd);

        BOOL fFoundAtLeastOneMatchingModule = FALSE;

        // Find all modules matching the name in this script entry
        {
            ModuleIDToInfoMap::LockHolder lockHolder(&m_moduleIDToInfoMap);

            ModuleIDToInfoMap::Const_Iterator iterator;
            for (iterator = m_moduleIDToInfoMap.Begin();
                 iterator != m_moduleIDToInfoMap.End();
                 iterator++)
            {
                wstring wszModulePathCur = &(iterator->second.m_wszModulePath[0]);

                // We only care if the full path of the module from our internal
                // module list ends with the module name found in the script
                if (!ContainsAtEnd(wszModulePathCur.c_str(), pEntry->wszModule))
                {
                    continue;
                }

                // The module is a match!
                fFoundAtLeastOneMatchingModule = TRUE;

                int nVersion = ((cmd == ScriptEntry::kRejit || cmd == ScriptEntry::kRejitWithInlines) ? pEntry->nVersion : 0);

                // Are we rejitting / reverting the entire module, or just a
                // specified class/function?
                
                if (*(pEntry->wszClass) == L'\0')
                {
                    TestReJitOrRevertEntireModule(
                        iterator->first,
                        &iterator->second,
                        cmd,
                        nVersion);
                    continue;
                }

                // Rejitting / revering a single class/function, so add it to
                // the batch
                
                moduleIds[iFunctions] = iterator->first;
                methodIds[iFunctions] = GetTokenFromName(
                    iterator->second.m_pImport, 
                    pEntry->wszClass,
                    (pEntry->command == ScriptEntry::kWaitRejitRepeatUntil) ? pEntry->wszFunction2 : pEntry->wszFunction);
                
                // Remember the latest version number for this mdMethodDef. 
                // (Use "0" to indicate that Revert() was th last operation
                // performed on the method.)
                iterator->second.m_pMethodDefToLatestVersionMap->Update(
                    methodIds[iFunctions], 
                    nVersion);
                
                iFunctions++;
                
                // If this assert fires, we probably mis-estimated the max number of
                // AppDomains, or this profiler is running against a highly concurrent
                // test application.
                assert(iFunctions < cFunctionsMax);

                // Intentionally continue through loop to find any other matching
                // modules. This catches the case where one module is loaded (unshared)
                // into multiple AppDomains
            }
        }

        if (!fFoundAtLeastOneMatchingModule)
        {
            TEST_FAILURE(L"Could not find a loaded module matching the path specified in the script: '" << pEntry->wszModule << L"'");
        }
    }

    // iFunctions is pointing at the next 0-based index to fill in, in the
    // module/method arrays, so it equals the count of the array entries already filled
    // in.
    UINT cFunctions = iFunctions;
    assert(cFunctions <= cFunctionsMax);

    // It's possible to get this far without any functions to batch up if all we're doing
    // is rejitting an entire module, which already took care of calling rejit for us.
    if (cFunctions > 0)
    {
        if (cmd == ScriptEntry::kRejitWithInlines)
        {
            vector<COR_PRF_METHOD> methods = ZipMethodsAndModules(cFunctions, moduleIds, methodIds);
            RequestReJITWithNgenInlines(methods);
        }
        else if ((cmd == ScriptEntry::kRejit) ||
            (cmd == ScriptEntry::kWaitRejitRepeatUntil))
        {
            CallRequestReJIT(cFunctions, moduleIds, methodIds);
        }
        else if (cmd == ScriptEntry::kRevert)
        {
            CallRequestRevert(cFunctions, moduleIds, methodIds);
        }
        else
        {
            while (!m_fStopAllThreadLoops)
            {
                if (cmd == ScriptEntry::kRejitThreadLoop)
                {
                    CallRequestReJIT(cFunctions, moduleIds, methodIds);
                    YieldProc(dwSleepMs);
                }
                else 
                {
                    assert(cmd == ScriptEntry::kRevertThreadLoop);
                    CallRequestRevert(cFunctions, moduleIds, methodIds);
                    YieldProc(dwSleepMs);
                }
            }
        }
    }

    delete [] moduleIds;
    delete [] methodIds;

    if ((cmd == ScriptEntry::kRejitThreadLoop) || (cmd == ScriptEntry::kRevertThreadLoop))
    {
        --m_cLoopingThreads;
    }
}

HRESULT ReJITScript::AddCoreLib()
{
    HRESULT hr;
    ICorProfilerModuleEnum *pModuleEnum;

    hr = m_pPrfCom->m_pInfo->EnumModules(&pModuleEnum);
    if (FAILED(hr))
    {
        return hr;
    }

    ModuleID mID;
    BOOL fFoundCoreLib = FALSE;
    while (S_OK == (hr = pModuleEnum->Next(1, &mID, NULL)))
    {
        PLATFORM_WCHAR moduleName[1024];
        PROFILER_WCHAR moduleNameTemp[1024];

        AssemblyID AssemblyID;
        LPCBYTE pBaseLoadAddress;

        hr = m_pPrfCom->m_pInfo->GetModuleInfo(mID,
                                                &pBaseLoadAddress,
                                                1024, //ULONG cchName
                                                NULL, //this is the actual length of moduleName ULONG *pcchName
                                                moduleNameTemp,
                                                &AssemblyID);
        if (FAILED(hr))
        {
            return hr;
        }

        ConvertProfilerWCharToPlatformWChar(moduleName, 1024, moduleNameTemp, 1024);
        
        if (ContainsAtEnd(moduleName, L"System.Private.CoreLib.dll"))
        {
            TEST_DISPLAY(L"Found System.Private.CoreLib, manually calling ModuleLoadFinished");
            fFoundCoreLib = TRUE;
            ModuleLoadFinished(mID, S_OK);
        }
    }

    if (!fFoundCoreLib)
    {
        TEST_FAILURE(L"Could not find System.Private.CoreLib to instrument.");
    }

    return S_OK;
}

void ReJITScript::TestReJitOrRevertEntireModule(
    ModuleID moduleID,
    const ModuleInfo * pModInfo,
    ScriptEntry::Command cmd,
    int nVersion)
{
    const int kMaxMethodsPerModule = 40000;
    const int kMaxTypeDefsPerModule = 5000;

    wstring wszModuleName = pModInfo->m_wszModulePath;
    IMetaDataImport * pImport = pModInfo->m_pImport;
    MethodDefToLatestVersionMap * pMethodDefToLatestVersionMap = pModInfo->m_pMethodDefToLatestVersionMap;

    HRESULT hr;
    mdTypeDef rgTypeDefs[kMaxTypeDefsPerModule];
    ModuleID rgModuleIDs[kMaxMethodsPerModule];
    mdMethodDef rgMethodDefs[kMaxMethodsPerModule];

    TEST_DISPLAY(L"Reading all classes and methods from '" << wszModuleName << L"' (0x" << std::hex << moduleID << L")");

    HCORENUM hEnumTypeDefs = NULL;
    ULONG cTypeDefsReturned = 0;
    hr = pImport->EnumTypeDefs(
        &hEnumTypeDefs,
        rgTypeDefs,
        dimensionof(rgTypeDefs),
        &cTypeDefsReturned);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"TestReJitOrRevertEntireModule: EnumTypeDefs failed with hr=0x" << std::hex << hr);
        return;
    }
    pImport->CloseEnum(hEnumTypeDefs);
    hEnumTypeDefs = NULL;

    TEST_DISPLAY(L"Found '" << cTypeDefsReturned << L"' classes");

    ULONG iMethodDef = 0;
    for (ULONG iTypeDef = 0; iTypeDef < cTypeDefsReturned; iTypeDef++)
    {
        HCORENUM hEnumMethodDefs = NULL;
        ULONG cMethodDefsReturned = 0;

        if (iMethodDef >= dimensionof(rgMethodDefs))
            break;

        if (ShouldAvoidInstrumentingType(pImport, rgTypeDefs[iTypeDef]))
            continue;

        hr = pImport->EnumMethods(
            &hEnumMethodDefs,
            rgTypeDefs[iTypeDef],
            &rgMethodDefs[iMethodDef],
            dimensionof(rgMethodDefs) - iMethodDef,
            &cMethodDefsReturned);
        if (FAILED(hr))
        {
            TEST_FAILURE(L"TestReJitOrRevertEntireModule: EnumMethods(0x" << std::hex << rgTypeDefs[iTypeDef]
                << L") failed with hr=0x" << std::hex << hr);
            return;
        }
        pImport->CloseEnum(hEnumMethodDefs);
        hEnumMethodDefs = NULL;

        iMethodDef += cMethodDefsReturned;
    }

    ULONG cFunctions = iMethodDef;
    TEST_DISPLAY(L"Found '" << cFunctions << L"' methods");

    // Don't forget to fill out the ModuleID array, and to keep track of the
    // latest version number for each method
    for (ULONG i = 0; i < cFunctions; i++)
    {
        rgModuleIDs[i] = moduleID;
        pMethodDefToLatestVersionMap->Update(rgMethodDefs[i], nVersion);
    }

    if (cmd == ScriptEntry::kRejit)
    {
        CallRequestReJIT(cFunctions, rgModuleIDs, rgMethodDefs);
    }
    else
    {
        assert(cmd == ScriptEntry::kRevert);
        CallRequestRevert(cFunctions, rgModuleIDs, rgMethodDefs);
    }
}

BOOL ReJITScript::ShouldAvoidInstrumentingType(
    IMetaDataImport * pImport, 
    mdTypeDef td)
{
    static const wstring k_rgwszTypesToAvoid[] =
    {
        // Don't instrument methods on the type containing our helper methods. 
        // Or else our helper methods will be instrumented into calling into
        // themselves, causing infinite recursion.
        k_wszHelpersContainerType,

        // Similarly, don't instrument methods on any types the managed helpers
        // call into, or that the JIT itself may need to use.
        L"System.IntPtr",
        L"System.Runtime.CompilerServices",
        // AttachTool uses assembly resolve events, instrumenting System.EventArgs
        // causes a stackoverflow
        L"System.EventArgs",
        L"System.AppDomain",
        L"System.ResolveEventArgs",
        L"System.Object"
    };

    HRESULT hr;
    ULONG cchTypeDefActual;
    DWORD dwTypeDefFlags;
    mdTypeDef typeDefBase;
    PROFILER_WCHAR wszTypeDefNameTemp[STRING_LENGTH];
    PLATFORM_WCHAR wszTypeDefName[STRING_LENGTH];

    hr = pImport->GetTypeDefProps(
        td,
        wszTypeDefNameTemp,
        dimensionof(wszTypeDefName),
        &cchTypeDefActual,
        &dwTypeDefFlags,
        &typeDefBase);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"GetTypeDefProps failed for typeDef=0x" << std::hex << td << L".  HRESULT=0x" << std::hex << hr);
        return TRUE;
    }

    ConvertProfilerWCharToPlatformWChar(wszTypeDefName, STRING_LENGTH, wszTypeDefNameTemp, STRING_LENGTH);

    for (UINT i=0; i < dimensionof(k_rgwszTypesToAvoid); i++)
    {
        if (wcsncmp(wszTypeDefName, k_rgwszTypesToAvoid[i].c_str(), wcslen(k_rgwszTypesToAvoid[i].c_str())) == 0)
            return TRUE;
    }

    return FALSE;
}


void ReJITScript::RequestReJITWithNgenInlines(const vector<COR_PRF_METHOD> &inlinees)
{
    vector<COR_PRF_METHOD> methodsToRejit;
    ModuleIDToInfoMap::LockHolder lockHolder(&m_moduleIDToInfoMap);
    for (auto met_iter = inlinees.begin(); met_iter != inlinees.end(); met_iter++)
    {
        ModuleInfo methodOwnerModuleInfo = m_moduleIDToInfoMap.Lookup(met_iter->moduleId);
        int   version = methodOwnerModuleInfo.m_pMethodDefToLatestVersionMap->Lookup(met_iter->methodId);

        for (auto mod_iter = m_moduleIDToInfoMap.Begin();
             mod_iter != m_moduleIDToInfoMap.End();
             mod_iter++)
        {
            const ModuleID moduleId = mod_iter->first;
            const ModuleInfo *pModInfo = &(mod_iter->second);

            ICorProfilerMethodEnum *methodEnum;
            BOOL incompleteData;
            HRESULT hr = m_pProfilerInfo->EnumNgenModuleMethodsInliningThisMethod(moduleId, met_iter->moduleId, met_iter->methodId, &incompleteData, &methodEnum);
            if (SUCCEEDED(hr) && methodEnum != NULL)
            {
                ULONG count, newCount;
                if (SUCCEEDED(hr = methodEnum->GetCount(&count)))
                {
                    if (count > 0)
                    {
                        size_t oldSize = methodsToRejit.size();
                        methodsToRejit.resize(methodsToRejit.size() + count);
                        if (FAILED(hr = methodEnum->Next(count, &methodsToRejit[oldSize], &newCount)) || newCount != count)
                        {
                            TEST_FAILURE(L"ICorProfilerMethodEnum failed to read elements.  HRESULT=0x" << std::hex << hr);
                        }
                        for (unsigned int i = 0; i < count; i++)
                        {
                            COR_PRF_METHOD &inliner = methodsToRejit[oldSize + i];
                            ModuleInfo inlinerModuleInfo = m_moduleIDToInfoMap.Lookup(inliner.moduleId);
                            inlinerModuleInfo.m_pMethodDefToLatestVersionMap->Update(inliner.methodId, version);
                        }
                    }
                }
                else
                {
                    TEST_FAILURE(L"ICorProfilerMethodEnum failed to return count.  HRESULT=0x" << std::hex << hr);
                }

                methodEnum->Release();
            }
        }
    }

    methodsToRejit.insert(methodsToRejit.end(), inlinees.begin(), inlinees.end());
    CallRequestReJIT(methodsToRejit);

    TEST_DISPLAY(L"RequestReJITWithNgenInlines. " << methodsToRejit.size() - inlinees.size() << L" inliners added to original " << inlinees.size() << L" methods.");
}

void ReJITScript::CallRequestReJIT(const vector<COR_PRF_METHOD> &methods)
{
    size_t count = methods.size();
    ModuleID * moduleIDs = new ModuleID [count];
    mdMethodDef * methodIDs = new mdMethodDef[count];
    for (size_t i = 0; i < count; i++)
    {
        moduleIDs[i] = methods[i].moduleId;
        methodIDs[i] = methods[i].methodId;
    }

    CallRequestReJIT((UINT)count, moduleIDs, methodIDs);
    delete[] moduleIDs;
    delete[] methodIDs;
}



void ReJITScript::CallRequestReJIT(
    UINT cFunctionsToRejit, 
    ModuleID * rgModuleIDs, 
    mdMethodDef * rgMethodIDs)
{
    HRESULT hr = m_pProfilerInfo->RequestReJIT(cFunctionsToRejit, rgModuleIDs, rgMethodIDs);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"RequestReJIT failed with 0x" << std::hex << hr);
        return;
    }

    TEST_DISPLAY(L"RequestReJIT called with " << cFunctionsToRejit << L" methods; returned success");
}

void ReJITScript::CallRequestRevert(
    UINT cFunctionsToRevert, 
    ModuleID * rgModuleIDs, 
    mdMethodDef * rgMethodIDs)
{
    HRESULT * rghrStatus = new HRESULT[cFunctionsToRevert];
    HRESULT hr = m_pProfilerInfo->RequestRevert(cFunctionsToRevert, rgModuleIDs, rgMethodIDs, rghrStatus);
    BOOL fAnyFailures = FALSE;

    if (FAILED(hr))
    {
        TEST_FAILURE(L"RequestRevert failed with 0x" << std::hex << hr);
        fAnyFailures = TRUE;
    }

    for (UINT i=0; i < cFunctionsToRevert; i++)
    {
        if (FAILED(rghrStatus[i]))
        {
            TEST_FAILURE(L"RequestRevert provided failure hrStatus '0x" << std::hex << rghrStatus[i] <<
                L"' for [ModuleID=0x" << std::hex << rgModuleIDs[i] << L", methodDef=0x" << std::hex << rgMethodIDs[i] <<
                L"].");
            fAnyFailures = TRUE;
        }
    }

    if (!fAnyFailures)
    {
        TEST_DISPLAY(L"RequestRevert succeeded for all " << cFunctionsToRevert << L" methods");
    }

    delete [] rghrStatus;
}

// METADATA HELPERS

void ReJITScript::AddMemberRefs(
    IMetaDataAssemblyImport * pAssemblyImport,
    IMetaDataAssemblyEmit * pAssemblyEmit,
    IMetaDataEmit * pEmit, 
    ModuleInfo * pModuleInfo)
{
    assert(pModuleInfo != NULL);

    TEST_DISPLAY(L"Adding memberRefs in this module to point to the helper managed methods");

    IMetaDataImport * pImport = pModuleInfo->m_pImport;
    
    HRESULT hr;

    // Signature for method the rewritten IL will call:
    // 
    // public static void MgdEnteredFunction64(UInt64 moduleIDCur, UInt32 mdCur, int nVersionCur)
    // OR
    // public static void MgdEnteredFunction32(UInt32 moduleIDCur, UInt32 mdCur, int nVersionCur)

#ifdef _WIN64
    COR_SIGNATURE sigFunctionProbe[] = 
    {
        IMAGE_CEE_CS_CALLCONV_DEFAULT,      // default calling convention
        0x03,                               // number of arguments == 3
        ELEMENT_TYPE_VOID,                  // return type == void
        ELEMENT_TYPE_U8,                    // arg 1: UInt64 moduleIDCur
        ELEMENT_TYPE_U4,                    // arg 2: UInt32 mdCur
        ELEMENT_TYPE_I4,                    // arg 3: int nVersionCur
    };
#else //  ! _WIN64 (32-bit code follows)
    COR_SIGNATURE sigFunctionProbe[] = 
    {
        IMAGE_CEE_CS_CALLCONV_DEFAULT,      // default calling convention
        0x03,                               // number of arguments == 3
        ELEMENT_TYPE_VOID,                  // return type == void
        ELEMENT_TYPE_U4,                    // arg 1: UInt32 moduleIDCur
        ELEMENT_TYPE_U4,                    // arg 2: UInt32 mdCur
        ELEMENT_TYPE_I4,                    // arg 3: int nVersionCur
    };
#endif //_WIN64
    
    mdAssemblyRef assemblyRef = NULL;
    mdTypeRef typeRef = mdTokenNil;

    if (m_fInstrumentationHooksInSeparateAssembly)
    {
        // Generate assemblyRef to ProfilerHelper.dll
        //BYTE rgbPublicKeyToken[] = {0xcc, 0xac, 0x92, 0xed, 0x87, 0x3b, 0x18, 0x5c};
        //wchar_t *lplocal;
        PROFILER_WCHAR wszLocale[MAX_PATH] = PROFILER_STRING("neutral");
        
        //lplocal = wszlocal;

        ASSEMBLYMETADATA assemblyMetaData;
        memset(&assemblyMetaData, 0, sizeof(assemblyMetaData));
        assemblyMetaData.usMajorVersion = 0;
        assemblyMetaData.usMinorVersion = 0;
        assemblyMetaData.usBuildNumber = 0;
        assemblyMetaData.usRevisionNumber = 0;
        assemblyMetaData.szLocale = wszLocale;
        assemblyMetaData.cbLocale = dimensionof(wszLocale);

        hr = pAssemblyEmit->DefineAssemblyRef(
            NULL,
            0,
            PROFILER_STRING("ProfilerHelper"),
            &assemblyMetaData,
            NULL,                   // hash blob
            NULL,                   // cb of hash blob
            0,                      // flags
            &assemblyRef);
        if (FAILED(hr))
        {
            TEST_FAILURE(L"DefineAssemblyRef failed with HRESULT = 0x" << std::hex << hr);
        }
    }
    else
    {
        // Probes are being added to mscorlib, not ProfilerHelper.dll.

        // Find existing assemblyRef to mscorlib.  Come on, you know you
        // already have one.

        HCORENUM hEnum = NULL;
        mdAssemblyRef rgAssemblyRefs[20];
        ULONG cAssemblyRefsReturned;
        assemblyRef = mdTokenNil;

        while (TRUE)
        {
            hr = pAssemblyImport->EnumAssemblyRefs(
                &hEnum,
                rgAssemblyRefs,
                dimensionof(rgAssemblyRefs),
                &cAssemblyRefsReturned);
            if (FAILED(hr))
            {
                TEST_FAILURE(L"EnumAssemblyRefs failed with HRESULT = 0x" << std::hex << hr);
                return;
            }

            if (cAssemblyRefsReturned == 0)
            {
                TEST_FAILURE(L"Could not find an AssemblyRef to mscorlib");
                return;
            }

            if (FindMscorlibReference(pAssemblyImport, rgAssemblyRefs, cAssemblyRefsReturned, &assemblyRef))
                break;
        }
        pAssemblyImport->CloseEnum(hEnum);
        hEnum = NULL;

        assert(assemblyRef != mdTokenNil);
    }

    // Generate typeRef to RejitTestProfilerHelper.ProfilerHelper or the pre-existing mscorlib type that we're
    // adding the managed helpers to

    const PROFILER_WCHAR * wszTypeToReference = 
        m_fInstrumentationHooksInSeparateAssembly ?
        PROFILER_STRING("RejitTestProfilerHelper.ProfilerHelper") :
        k_wszHelpersContainerTypeProfiler;

    hr = pEmit->DefineTypeRefByName(
        assemblyRef,
        wszTypeToReference,
        &typeRef);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"DefineTypeRefByName to '" << wszTypeToReference << L"' failed with HRESULT = 0x" << std::hex << hr);
    }

    hr = pEmit->DefineMemberRef(
        typeRef,
        k_wszEnteredFunctionProbeNameProfiler,
        sigFunctionProbe,
        sizeof(sigFunctionProbe),
        &(pModuleInfo->m_mdEnterProbeRef));
    if (FAILED(hr))
    {
        TEST_FAILURE(L"DefineMemberRef to '" << k_wszEnteredFunctionProbeName << L"' failed with HRESULT = 0x" << std::hex << hr);
    }

    hr = pEmit->DefineMemberRef(
        typeRef,
        k_wszExitedFunctionProbeNameProfiler,
        sigFunctionProbe,
        sizeof(sigFunctionProbe),
        &(pModuleInfo->m_mdExitProbeRef));
    if (FAILED(hr))
    {
        TEST_FAILURE(L"DefineMemberRef to '" << k_wszExitedFunctionProbeName << L"' failed with HRESULT = 0x" << std::hex << hr);
    }
}

BOOL ReJITScript::FindMscorlibReference(
    IMetaDataAssemblyImport * pAssemblyImport,
    mdAssemblyRef * rgAssemblyRefs,
    ULONG cAssemblyRefs,
    mdAssemblyRef * parMscorlib)
{
    HRESULT hr;

    for (ULONG i=0; i < cAssemblyRefs; i++)
    {
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

        hr = pAssemblyImport->GetAssemblyRefProps(
            rgAssemblyRefs[i],
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
            return FALSE;
        }

        ConvertProfilerWCharToPlatformWChar(wszName, STRING_LENGTH, wszNameTemp, STRING_LENGTH);

        if (ContainsAtEnd(wszName, L"mscorlib") || ContainsAtEnd(wszName, L"System.Private.CoreLib"))
        {
            TEST_DISPLAY(L"Found mscorlib reference");
            *parMscorlib = rgAssemblyRefs[i];
            return TRUE;
        }

        if (ContainsAtEnd(wszName, L"System.Runtime"))
        {
            TEST_DISPLAY(L"Found System.Runtime reference");
            *parMscorlib = rgAssemblyRefs[i];
            return TRUE;
        }
    }

    return FALSE;
}

    
mdMethodDef ReJITScript::GetTokenFromName(
    IMetaDataImport * pImport, 
    const PLATFORM_WCHAR *wszClass,
    const PLATFORM_WCHAR *wszFunction)
{
    HRESULT hr;
    HCORENUM hEnum = NULL;
    mdTypeDef typeDef;
    mdMethodDef rgMethodDefs[2];
    ULONG cMethodDefsReturned;

    PROFILER_WCHAR wszClassTemp[STRING_LENGTH];
    ConvertPlatformWCharToProfilerWChar(wszClassTemp, STRING_LENGTH, wszClass, wcslen(wszClass) + 1);

    hr = pImport->FindTypeDefByName(
        wszClassTemp,
        mdTypeDefNil,       // [IN] TypeDef/TypeRef for Enclosing class.
        &typeDef);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"Failed to find class '" << wszClass << L"'.  HRESULT=0x0x" << std::hex << hr);
    }

    PROFILER_WCHAR wszFunctionTemp[STRING_LENGTH];
    ConvertPlatformWCharToProfilerWChar(wszFunctionTemp, STRING_LENGTH, wszFunction, wcslen(wszFunction) + 1);

    hr = pImport->EnumMethodsWithName(
        &hEnum,
        typeDef,
        wszFunctionTemp,
        &(rgMethodDefs[0]),
        dimensionof(rgMethodDefs),
        &cMethodDefsReturned);
    if (FAILED(hr) || (hr == S_FALSE))
    {
        TEST_FAILURE(L"Found class '" << wszClass << L"', but no member methods with name '" << wszFunction << L"'.  HRESULT=0x0x" << std::hex << hr);
    }
    if (cMethodDefsReturned != 1)
    {
        TEST_FAILURE(L"Expected exactly 1 methodDef to match class '" << wszClass << L"', method '" << wszFunction << L"', but actually found " << cMethodDefsReturned);
    }

    return rgMethodDefs[0];
}



HRESULT __stdcall StackWalkCallback(FunctionID funcId,
                                    UINT_PTR ip,
                                    COR_PRF_FRAME_INFO frameInfo,
                                    ULONG32 contextSize,
                                    BYTE context[],
                                    void *clientData)
{
    return S_OK;
}


//
// ICorProfilerCallback4 Callbacks
//

HRESULT ReJITScript::ReJITCompilationStarted(
        FunctionID functionID,
        ReJITID rejitId,
        BOOL fIsSafeToBlock)
{
    TEST_DISPLAY(L"ReJITScript::ReJITCompilationStarted for FunctionID '0x" << std::hex << functionID << 
        L"' - RejitID '0x" << std::hex << rejitId << L"' called");

    m_cRejitStarted++;

    if (m_testType == kRejitTestType_Nondeterministic)
    {
        // Do not maintain shadow stack and do not maintain FunctionID->Version map for
        // nondeterministic testing.
        return S_OK;
    }

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
        TEST_FAILURE(L"GetFunctionInfo for FunctionID '0x" << std::hex << functionID << L"' failed.  HRESULT = 0x0x" << std::hex << hr);
    }

    ModuleInfo moduleInfo = m_moduleIDToInfoMap.Lookup(moduleID);
    int nVersion = moduleInfo.m_pMethodDefToLatestVersionMap->Lookup(methodDef);
    if (nVersion == 0)
    {
        TEST_FAILURE(L"ReJITCompilationStarted called for FunctionID '0x" << std::hex << functionID << L"', which should have been reverted.");
        return S_OK;
    }

    TEST_DISPLAY(L"Found latest version number of " << nVersion << L" for rejitting function.  Associating it with rejitID. (FunctionID="
        << std::hex << functionID << L", RejitID=0x" << std::hex << rejitId << L", mdMethodDef=0x" << std::hex << methodDef << L").");

    FunctionVersion funcVersion = {{0}};

    // If functionID is already in the map, override our stack's FunctionVersion with
    // the copy in the map.  Else, keep our stack's funcVersion in tact
    m_functionIDToVersionMap.LookupIfExists(functionID, &funcVersion);
    assert(nVersion < dimensionof(funcVersion.m_rgRejitIDs));
    funcVersion.m_rgRejitIDs[nVersion] = rejitId;

    // Put the updated array (versionNumber-to-rejitID) back into the function map
    m_functionIDToVersionMap.Update(functionID, funcVersion);

    return S_OK;
}

HRESULT ReJITScript::GetReJITParameters(
        ModuleID moduleId,
        mdMethodDef methodId,
        ICorProfilerFunctionControl *pFunctionControl)
{
    TEST_DISPLAY(L"ReJITScript::GetReJITParameters called, methodDef = 0x" << std::hex << methodId);

    ModuleInfo moduleInfo = m_moduleIDToInfoMap.Lookup(moduleId);
    HRESULT hr;

    // Deterministic script tests add enter/exit probes, and use them to validate
    // function entry & exit against the script WAIT_CALL lines, as well as against stack
    // snapshots.  Nondeterministic tests may add enter/exit probes as well, though
    // only to a small subset of methods, primarily to ensure the WAIT_CALL lines
    // work properly.

    int nVersion;
    BOOL fFoundVersion = moduleInfo.m_pMethodDefToLatestVersionMap->LookupIfExists(methodId, &nVersion);
    if (m_testType == kRejitTestType_Deterministic)
    {
        if (!fFoundVersion)
        {
            TEST_FAILURE(L"Failed looking up version number for methodID '" << methodId << 
                L"', moduleID '" << moduleId << L"'");
            return S_OK;
        }

        if (nVersion == 0)
        {
            TEST_FAILURE(L"GetReJITParameters called for methodDef '0x" << std::hex << methodId << L"', which should have been reverted.");
            return S_OK;
        }
    }

    if (fFoundVersion && (nVersion != 0))
    {
        // Either this is a deterministic test, in which case all rejitting should
        // provide enter/exit probes with specific version numbers OR this is a
        // nondeterministic test, but this method is one of the few where we can safely
        // provide enter/exit probes.
        DisableInlining(pFunctionControl);
        hr = RewriteIL(
            m_pProfilerInfo,
            pFunctionControl,
            moduleId,
            methodId,
            nVersion,
            moduleInfo.m_mdEnterProbeRef,
            moduleInfo.m_mdExitProbeRef);
    }
    else
    {
        assert(m_testType == kRejitTestType_Nondeterministic);

        // Nondeterministic tests often cannot do fancy validation, since:
        //     * arbitrary functions are getting instrumented (which our simple IL
        //         rewriter may not be able to handle well), and
        //     * the test inputs may be heavily multithreaded, which prevents us from
        //         knowing which version of a rejitted function may be getting called.
        //
        // So rejit by using the exact same IL that was there originally.
        hr = RewriteIL(
            m_pProfilerInfo,
            pFunctionControl,
            moduleId,
            methodId,

            // Dummy values for version/probe tokens means to add no new instrumentation
            0, mdTokenNil, mdTokenNil);
    }
    if (FAILED(hr))
    {
        TEST_FAILURE(L"RewriteIL failed.  ModuleID=0x" << std::hex << moduleId << L", methodDef=0x" << std::hex << methodId << L", hr = 0x" << std::hex << hr);
    }

    // useful to test the synchronization for GetReJITParameters callback is correct
    // the callback should only get called once per rejit, even if multiple threads call the same method
    if (m_getReJITParametersDelayMs != 0)
    {
        YieldProc(m_getReJITParametersDelayMs);
    }

    return S_OK;
}

void ReJITScript::DisableInlining(ICorProfilerFunctionControl * pFunctionControl)
{
    HRESULT hr = pFunctionControl->SetCodegenFlags(COR_PRF_CODEGEN_DISABLE_INLINING);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"SetCodegenFlags(COR_PRF_CODEGEN_DISABLE_INLINING) failed with hr="
            << std::hex << hr);
    }
}

void ReJITScript::TestGetReJITIDs(FunctionID functionId)
{
    ReJITID reJitIds[kcFunctionsToBatchForRejit];
    ULONG   cReJitIds = 0;

    printf("Getting ReJITIDs for '%p'\n", reinterpret_cast<void *>(functionId));

    HRESULT hr = m_pProfilerInfo->GetReJITIDs(functionId, 0, &cReJitIds, reJitIds);
    if (hr != E_INVALIDARG)
    {
        printf("Should have failed -- 0x%08x\n", hr);
        return;
    }

    hr = m_pProfilerInfo->GetReJITIDs(functionId, 1, &cReJitIds, reJitIds);
    if (FAILED(hr))
    {
        printf("Wrong return value -- 0x%08x\n", hr);
        return;
    }
    else
    {
        printf("GetReJITIDs returned %u ids (hr = %08x).\n", cReJitIds, hr);
        for (ULONG i = 0; i < cReJitIds && i < 1; i++)
        {
            printf("\tid #%u = %p\n", i, reinterpret_cast<void *>(reJitIds[i]));
        }
    }

    if (hr == S_FALSE)
    {
        hr = m_pProfilerInfo->GetReJITIDs(functionId, dimensionof(reJitIds), &cReJitIds, reJitIds);
        if (hr != S_OK)
        {
            printf("Wrong return value -- 0x%08x\n", hr);
            return;
        }
        else
        {
            printf("GetReJITIDs returned %u ids (hr = %08x).\n", cReJitIds, hr);
            for (ULONG i = 0; i < cReJitIds && i < dimensionof(reJitIds); i++)
            {
                printf("\tid #%u = %p\n", i, reinterpret_cast<void *>(reJitIds[i]));
            }
        }
    }
}

HRESULT ReJITScript::ReJITCompilationFinished(
        FunctionID functionId,
        ReJITID rejitId,
        HRESULT hrStatus,
        BOOL fIsSafeToBlock)
{
    TEST_DISPLAY("ReJITScript::ReJITCompilationFinished '0x" << std::hex << functionId << L"' - '0x" << std::hex << rejitId << L"', '0x" << std::hex << hrStatus << L"' called");
    m_fAtLeastOneSuccessfulRejitFinished = TRUE;
    return S_OK;
}


HRESULT ReJITScript::ReJITError(
    ModuleID moduleId,
    mdMethodDef methodId,
    FunctionID functionId,
    HRESULT hrStatus)
{
    if (m_testType == kRejitTestType_Nondeterministic)
    {
        // Nondeterministic testing will rightfully receive errors when arbitrary methods
        // are requested to be rejitted (typically CORPROF_E_DATAINCOMPLETE indicating
        // they are not yet be loaded). So don't fail the test.
        return S_OK;
    }

    // Even in deterministic tests, a rejit error may be expected if the script
    // explicitly tries to wait for it
    if (FinishWaitIfAppropriate(moduleId, methodId, mdTokenNil, 0, FALSE, hrStatus, ScriptEntry::kWaitRejitError))
    {
        return S_OK;
    }

    TEST_FAILURE(L"ReJITError called.  ModuleID=0x" << std::hex << moduleId << L", methodDef=0x" << std::hex << methodId << L", FunctionID=0x" << std::hex << functionId << L", HR=0x" << std::hex << hrStatus);
    return hrStatus;
}


// REJIT SCRIPT / VERIFICATION

BOOL ReJITScript::SeekToNextNewline(FILE * fp)
{
    WCHAR wch;
    while ((wch = fgetwc(fp)) != L'\n');
    if (wch == WEOF)
        return FALSE;

    assert(wch == L'\n');
    return TRUE;
}

BOOL ReJITScript::ScriptFileToString(wchar_t *wszScript, UINT cchScript)
{
    wstring scriptFileInfo = ReadEnvironmentVariable(L"bpd_rejitscript");
    if (scriptFileInfo.size() == 0)
    {
        TEST_FAILURE(L"Failed to read value of environment variable bpd_rejitscript.");
        return FALSE;
    }
    if (scriptFileInfo.size() >= 1000)
    {
        TEST_FAILURE(L"Value of environment variable bpd_rejitscript is too big.  Char count=" <<
            scriptFileInfo.size() << L", max expected char count=" << 999);
        return FALSE;
    }

    TEST_DISPLAY(L"Reading rejit script block using bpd_rejitscript value: '" << scriptFileInfo << L"'");

    const wchar_t * wszDelimiters = L" \t\r\n";
    wchar_t wszScriptFileInfo[1000];
    memset(wszScriptFileInfo, 0, sizeof(wchar_t) * 1000);
    for(unsigned int i = 0; i < scriptFileInfo.size() && i < 999; ++i)
    {
        wszScriptFileInfo[i] = scriptFileInfo[i];
    }

    wchar_t *buffer = NULL;
    wchar_t *wszStartLineNo = wcstok(wszScriptFileInfo, wszDelimiters, &buffer);
    if (wszStartLineNo == NULL)
    {
        TEST_FAILURE(L"Failed to read 'start line number' from value of environment variable bpd_rejitscript");
        return FALSE;
    }

    int nStartLineNo = std::stoi(wszStartLineNo);
    if (nStartLineNo == 0)
    {
        TEST_FAILURE(L"Failed converting Orange script START line number '" << wszStartLineNo << L"' to an integer");
        return FALSE;
    }

    wchar_t *wszEndLineNo = wcstok(NULL, wszDelimiters, &buffer);
    if (wszEndLineNo == NULL)
    {
        TEST_FAILURE(L"Failed to read 'end line number' from value of environment variable bpd_rejitscript");
        return FALSE;
    }

    int nEndLineNo = std::stoi(wszEndLineNo);
    if (nEndLineNo == 0)
    {
        TEST_FAILURE(L"Failed converting Orange script END line number '" << wszEndLineNo << L"' to an integer");
        return FALSE;
    }

    wchar_t *wszFile = wcstok(NULL, wszDelimiters, &buffer);
    if (wszFile == NULL)
    {
        TEST_FAILURE(L"Failed to read script filename from value of environment variable bpd_rejitscript");
        return FALSE;
    }

    // Open Orange script file
    string narrowFileName = ToNarrow(wszFile);
    FILE *fpScript = fopen(narrowFileName.c_str(), "rt");
    if (fpScript == NULL)
    {
        TEST_FAILURE(L"Unable to open file '" << wszFile << L"'.  Err=" << errno);
        return FALSE;
    }

    // Skip to nStartLineNo
    for (int i=0; i < nStartLineNo-1; i++)
    {
        if (!SeekToNextNewline(fpScript))
        {
            TEST_FAILURE(L"Encountered EOF in the Orange script, trying to seek to line '" << nStartLineNo << L"'");
            return FALSE;
        }
    }

    int nLineNoCur = nStartLineNo;
    UINT cchCur = 0;

    while (nLineNoCur <= nEndLineNo)
    {
        if (cchCur >= cchScript-1)
        {
            TEST_FAILURE(L"rejit script block from Orange script is too big (exceeded '" << cchScript << L"' characters)");
            break;
        }

        WCHAR wch = fgetwc(fpScript);
        if (wch == WEOF)
        {
            break;
        }

        if (wch == L'\n')
            nLineNoCur++;

        *wszScript = wch;
        wszScript++;
        cchCur++;
    }

    fclose(fpScript);

    if (nLineNoCur < nEndLineNo)
    {
        TEST_FAILURE(L"Unable to read up to line '" << nEndLineNo << L"' in the Orange script.");
        return FALSE;
    }

    if (cchCur >= cchScript)
        return FALSE;

    *wszScript = L'\0';

    // Remember the first line number of the rejit script as the current line
    // number we're executing.  (We'll stay here while we read the entire
    // script, and only move forward once we've executed this line, which
    // only happens when the process gets going.)
    m_iScriptLineNumberCur = nStartLineNo;

    return TRUE;
}

HRESULT ReJITScript::ReadScript()
{
    PLATFORM_WCHAR wszScript[80000];
    TEST_DISPLAY(L"Reading rejit script...");
    if (!ScriptFileToString(wszScript, dimensionof(wszScript)))
    {
        return E_FAIL;
    }

    wchar_t *wszBeginningOfLine = wszScript;
    int nLineNumberFromOrangeScript = m_iScriptLineNumberCur;

    HRESULT hr = S_OK;
    while (TRUE)
    {
        wchar_t *wszEndOfLine = wcschr(wszBeginningOfLine, L'\n');
        if (wszEndOfLine != NULL)
        {
            // Stomp over newline to make wszBeginningOfLine look like a
            // string containing a single line from the script
            assert(*wszEndOfLine == '\n');
            *wszEndOfLine = L'\0';
        }

        hr = InterpretLineFromScript(wszBeginningOfLine, nLineNumberFromOrangeScript);

        if (FAILED(hr) || (wszEndOfLine == NULL))
        {
            break;
        }

        wszBeginningOfLine = wszEndOfLine + 1;
        nLineNumberFromOrangeScript++;
    }

    return hr;
}

HRESULT ReJITScript::InterpretLineFromScript(wchar_t *wszLine, int nLineNumber)
{
    TEST_DISPLAY(L"Reading line # '" << nLineNumber << L"': '" << wszLine << L"'");
    
    if (*wszLine == L'#')
    {
        // Ignore comment lines
        return S_OK;
    }

    // Make a clean copy of the line, since wcstok below pokes NULLs into the
    // original Line string
    PLATFORM_WCHAR wszLineCopy[1024];
    wcscpy(wszLineCopy, wszLine);

    const wchar_t *wszDelimiters = L" \t\r\n";
    wchar_t *buffer;
    wchar_t *wszCommand = wcstok(wszLine, wszDelimiters, &buffer);
    if (wszCommand == NULL)
    {
        // Empty line.  Done.
        return S_OK;
    }

    m_cScriptEntries++;
    if (m_cScriptEntries >= dimensionof(m_rgScriptEntries))
    {
        TEST_FAILURE(L"Script is longer than '" << dimensionof(m_rgScriptEntries) << L"' commands.");
        return E_FAIL;
    }
    ScriptEntry * pEntry = &(m_rgScriptEntries[m_cScriptEntries - 1]);
    wcscpy(pEntry->wszOrigEntryText, wszLineCopy);

    BOOL fParseClassAndFunction = FALSE;

    pEntry->nLineNumber = nLineNumber;
    if (StrCmpIns(wszCommand, L"wait_jit"))
    {
        // Format:
        // WAIT_JIT Module ClassName FunctionName
        pEntry->command =  ScriptEntry::kWaitJit;
        fParseClassAndFunction = TRUE;
    }
    else if (StrCmpIns(wszCommand, L"wait_call"))
    {
        // Format:
        // WAIT_CALL Module ClassName FunctionName
        pEntry->command =  ScriptEntry::kWaitCall;
        fParseClassAndFunction = TRUE;
    }
    else if (StrCmpIns(wszCommand, L"wait_inline"))
    {
        // Format:
        // WAIT_INLINE Module ClassName FunctionName1 FunctionName2
        pEntry->command =  ScriptEntry::kWaitInline;
        fParseClassAndFunction = TRUE;
    }
    else if (StrCmpIns(wszCommand, L"wait_module"))
    {
        // Format:
        // WAIT_CALL Module IsShared
        pEntry->command =  ScriptEntry::kWaitModule;
    }
    else if (StrCmpIns(wszCommand, L"wait_rejiterror"))
    {
        // Format:
        // WAIT_REJITERROR Module ClassName FunctionName HRESULT
        pEntry->command =  ScriptEntry::kWaitRejitError;
        fParseClassAndFunction = TRUE;
    }
    else if (StrCmpIns(wszCommand, L"rejit"))
    {
        // Format:
        // REJIT Module ClassName FunctionName VersionNumber
        pEntry->command =  ScriptEntry::kRejit;
        fParseClassAndFunction = TRUE;
    }
    else if (StrCmpIns(wszCommand, L"rejit_with_inlines"))
    {
        // Format:
        // REJIT Module ClassName FunctionName VersionNumber
        pEntry->command = ScriptEntry::kRejitWithInlines;
        fParseClassAndFunction = TRUE;
    }
    else if (StrCmpIns(wszCommand, L"rejit_module"))
    {
        // Format:
        // REJIT_MODULE Module VersionNumber
        pEntry->command =  ScriptEntry::kRejit;
    }
    else if (StrCmpIns(wszCommand, L"rejit_thread_loop"))
    {
        // Format:
        // REJIT_THREAD_LOOP Module ClassName FunctionName SleepInMsBetweenRejits
        pEntry->command =  ScriptEntry::kRejitThreadLoop;
        fParseClassAndFunction = TRUE;
    }
    else if (StrCmpIns(wszCommand, L"revert_thread_loop"))
    {
        // Format:
        // REVERT_THREAD_LOOP Module ClassName FunctionName SleepInMsBetweenRejits
        pEntry->command = ScriptEntry::kRevertThreadLoop;
        fParseClassAndFunction = TRUE;
    }
    else if (StrCmpIns(wszCommand, L"revert"))
    {
        // Format:
        // REVERT Module ClassName FunctionName
        pEntry->command = ScriptEntry::kRevert;
        fParseClassAndFunction = TRUE;
    }
    else if (StrCmpIns(wszCommand, L"stackwalk"))
    {
        // Format:
        // STACKWALK
        pEntry->command = ScriptEntry::kStackwalk;

        // No other parameters, so we're done
        return S_OK;
    }
    else if (StrCmpIns(wszCommand, L"stop_all_thread_loops"))
    {
        // Format:
        // STOP_ALL_THREAD_LOOPS
        pEntry->command = ScriptEntry::kStopAllThreadLoops;

        // No other parameters, so we're done
        return S_OK;
    }
    else if (StrCmpIns(wszCommand, L"wait_rejit_repeat_until"))
    {
        // Format:
        // WAIT_REJIT_REPEAT_UNTIL Module ClassName FunctionNameToWait FunctionNameToRejit FunctionNameUntil
        pEntry->command =  ScriptEntry::kWaitRejitRepeatUntil;
        fParseClassAndFunction = TRUE;
    }
    else if (StrCmpIns(wszCommand, L"nop"))
    {
        // Format:
        // NOP
        pEntry->command = ScriptEntry::kNop;

        // No other parameters, so we're done
        return S_OK;
    }
    else
    {
        TEST_FAILURE(L"Unrecognized command in script: '" << wszCommand << L"'");
        return E_FAIL;
    }

    if (   ((pEntry->command == ScriptEntry::kRejitThreadLoop) || 
            (pEntry->command == ScriptEntry::kRevertThreadLoop)) &&
        (m_testType == kRejitTestType_Deterministic))
    {
        TEST_FAILURE(L"INCONSISTENT SETTINGS.  Script contains REJIT_THREAD_LOOP / REVERT_THREAD_LOOP, but the test name is 'ReJITScript'.  The thread loop commands are intended for nondeterministic testing only.");
        return E_FAIL;
    }

    wchar_t *wszModule = wcstok(NULL, wszDelimiters, &buffer);
    if (wszModule == NULL)
    {
        TEST_FAILURE(L"Module missing from script line");
        return E_FAIL;
    }

    wcscpy(pEntry->wszModule, wszModule);

    if ((pEntry->command == ScriptEntry::kRejit) &&
        ContainsAtEnd(wszModule, L"mscorlib.dll") &&
        m_fInstrumentationHooksInSeparateAssembly)
    {
        TEST_FAILURE(L"INCONSISTENT SETTINGS.  Script contains a REJIT command for mscorlib, but BPD_INSTRUMENTATIONHOOKSINSEPARATEASSEMBLY is set.  It is forbidden to instrument mscorlib to call into a separate assembly.  Ensure the Orange script either (a) removes rejit commands for mscorlib OR (b) includes 'mode BPD_INSTRUMENTATIONHOOKSINSEPARATEASSEMBLY off'");
        return E_FAIL;
    }

    if (pEntry->command == ScriptEntry::kWaitModule)
    {
        // Only parameter left for kWaitModule is whether it should be loaded
        // shared
        wchar_t *wszShared = wcstok(NULL, wszDelimiters, &buffer);
        if (StrCmpIns(wszShared, L"shared"))
        {
            pEntry->fShared = TRUE;
        }
        else if (StrCmpIns(wszShared, L"unshared"))
        {
            pEntry->fShared = FALSE;
        }
        else
        {
            TEST_FAILURE(L"Expected parameter to be 'shared' or 'unshared'.  Actual value = '" << wszShared << L"'");
            return E_FAIL;
        }

        return S_OK;
    }

    if (fParseClassAndFunction)
    {
        // Parse out class and function names
        
        wchar_t *wszClass = wcstok(NULL, wszDelimiters, &buffer);
        if (wszClass == NULL)
        {
            TEST_FAILURE(L"Class missing from script line");
            return E_FAIL;
        }
        
        wcscpy(pEntry->wszClass, wszClass);

        wchar_t *wszFunction = wcstok(NULL, wszDelimiters, &buffer);
        if (wszFunction == NULL)
        {
            TEST_FAILURE(L"Function missing from script line");
            return E_FAIL;
        }

        wcscpy(pEntry->wszFunction, wszFunction);
    }

    if (pEntry->command == ScriptEntry::kWaitRejitError)
    {
        // WAIT_REJITERROR command also takes the expected error HRESULT
        wchar_t *wszHRESULT = wcstok(NULL, wszDelimiters, &buffer);
        if (wszHRESULT == NULL)
        {
            TEST_FAILURE(L"HRESULT missing from WAIT_REJITERROR script line");
            return E_FAIL;
        }

        pEntry->hrRejitError = std::stoi(wszHRESULT, nullptr, 16);
    }

    if ((pEntry->command == ScriptEntry::kWaitInline) ||
        (pEntry->command == ScriptEntry::kWaitRejitRepeatUntil))
    {
        // WAIT_INLINE and WAIT_REJIT_REPEAT_UNTIL commands also takes second function
        // (inlinee, function to rejit, respectively)
        wchar_t *wszFunction2 = wcstok(NULL, wszDelimiters, &buffer);
        if (wszFunction2 == NULL)
        {
            TEST_FAILURE(L"Second function missing from script line");
            return E_FAIL;
        }
        
        wcscpy(pEntry->wszFunction2, wszFunction2);

        // WAIT_REJIT_REPEAT_UNTIL takes a third function
        if (pEntry->command == ScriptEntry::kWaitRejitRepeatUntil)
        {
            wchar_t *wszFunction3 = wcstok(NULL, wszDelimiters, &buffer);
            if (wszFunction3 == NULL)
            {
                TEST_FAILURE(L"Third function missing from script line");
                return E_FAIL;
            }
            
            wcscpy(pEntry->wszFunction3, wszFunction3);
        }
    }

    if (pEntry->command == ScriptEntry::kRejit || pEntry->command == ScriptEntry::kRejitWithInlines)
    {
        // REJIT & REJIT_MODULE commands also take a version number

        wchar_t *wszVersionNumber = wcstok(NULL, wszDelimiters, &buffer);
        if (wszVersionNumber == NULL)
        {
            TEST_FAILURE(L"Version number missing from script line");
            return E_FAIL;
        }
        pEntry->nVersion = std::stoi(wszVersionNumber);
        if (pEntry->nVersion == 0)
        {
            TEST_FAILURE(L"Failed converting version number '" << wszVersionNumber << L"' to an integer");
            return E_FAIL;
        }
    }

    if ((pEntry->command == ScriptEntry::kRejitThreadLoop) ||
        (pEntry->command == ScriptEntry::kRevertThreadLoop))
    {
        // Thread loop commands take a sleep parameter

        wchar_t *wszSleepMs = wcstok(NULL, wszDelimiters, &buffer);
        if (wszSleepMs == NULL)
        {
            TEST_FAILURE(L"SleepMs missing from script line");
            return E_FAIL;
        }
        pEntry->dwSleepMs = std::stoi(wszSleepMs);
        if (pEntry->dwSleepMs == 0)
        {
            TEST_FAILURE(L"Failed converting sleep ms '" << wszSleepMs << L"' to an integer");
            return E_FAIL;
        }
    }

    return S_OK;
}


void ReJITScript::NtvEnteredFunction(
    ModuleID moduleIDCur,
    mdMethodDef mdCur,
    int nVersionCur)
{
    ModuleInfo moduleInfo = m_moduleIDToInfoMap.Lookup(moduleIDCur);
    PROFILER_WCHAR wszTypeDefNameTemp[LONG_LENGTH];
    PLATFORM_WCHAR wszTypeDefName[LONG_LENGTH];
    PROFILER_WCHAR wszMethodDefNameTemp[LONG_LENGTH];
    PLATFORM_WCHAR wszMethodDefName[LONG_LENGTH];

    GetClassAndFunctionNamesFromMethodDef(
        moduleInfo.m_pImport,
        moduleIDCur, 
        mdCur,
        wszTypeDefNameTemp,
        LONG_LENGTH,
        wszMethodDefNameTemp,
        LONG_LENGTH);

    ConvertProfilerWCharToPlatformWChar(wszTypeDefName, LONG_LENGTH, wszTypeDefNameTemp, LONG_LENGTH);
    ConvertProfilerWCharToPlatformWChar(wszMethodDefName, LONG_LENGTH, wszMethodDefNameTemp, LONG_LENGTH);

    TEST_DISPLAY(L"Rejitted function entered: " << wszTypeDefName << L"." << wszMethodDefName << L", Version=" << nVersionCur);

    // Only deterministic testing should be verifying specific version numbers and
    // maintaining shadow stacks
    if (m_testType == kRejitTestType_Deterministic)
    {
        VerifyFunctionVersion(moduleIDCur, mdCur, nVersionCur);

        // Update shadow stack
        if (m_cShadowStackFrameInfos + 1 >= dimensionof(m_rgShadowStackFrameInfo))
        {
            TEST_FAILURE(L"Shadow stack about to grow beyond expected max size of " << 
                dimensionof(m_rgShadowStackFrameInfo) << L" frames.  Is there a test bug causing infinite recursion?");
        }
        else
        {
            ShadowStackFrameInfo * pFrame = &m_rgShadowStackFrameInfo[m_cShadowStackFrameInfos];
            pFrame->m_moduleID = moduleIDCur;
            pFrame->m_methodDef = mdCur;
            pFrame->m_nVersion = nVersionCur;
            m_cShadowStackFrameInfos++;
        }
    }

    // Were we waiting for this function to be called in order to do something?
    
    // Check if there's a WAIT_CALL waiting for us
    if (FinishWaitIfAppropriate(moduleIDCur, mdCur, mdTokenNil, nVersionCur, FALSE, 0, ScriptEntry::kWaitCall))
        return;

    // Check if there's a WAIT_REJIT_REPEAT_UNTIL waiting for us to be called in order
    // to do a rejit
    if (DoRejitFromRepeatCommandIfNecessary(moduleIDCur, mdCur))
        return;

    // Check if there's a WAIT_REJIT_REPEAT_UNTIL waiting for us to be called to STOP
    // doing the repeated rejits (i.e., is this function the "UNTIL"?)
    if (FinishWaitIfAppropriate(moduleIDCur, mdCur, mdTokenNil, nVersionCur, FALSE, 0, ScriptEntry::kWaitRejitRepeatUntil))
        return;
}

void ReJITScript::NtvExitedFunction(
    ModuleID moduleIDCur,
    mdMethodDef mdCur,
    int nVersionCur)
{
    ModuleInfo moduleInfo = m_moduleIDToInfoMap.Lookup(moduleIDCur);
    WCHAR wszTypeDefName[512];
    WCHAR wszMethodDefName[512];
    GetClassAndFunctionNamesFromMethodDef(
        moduleInfo.m_pImport,
        moduleIDCur, 
        mdCur,
        wszTypeDefName,
        dimensionof(wszTypeDefName),
        wszMethodDefName,
        dimensionof(wszMethodDefName));

    TEST_DISPLAY(L"Rejitted function exited: " << wszTypeDefName << L"." << wszMethodDefName << L", Version=" << nVersionCur);

    // Only deterministic testing should be maintaining shadow stacks
    if (m_testType != kRejitTestType_Deterministic)
        return;

    // Update shadow stack, but verify its leaf is the function we're exiting
    if (m_cShadowStackFrameInfos <= 0)
    {
        TEST_FAILURE(L"Exiting a function with an empty shadow stack.");
        return;
    }

    ShadowStackFrameInfo * pFrame = &m_rgShadowStackFrameInfo[m_cShadowStackFrameInfos-1];
    if ((pFrame->m_moduleID != moduleIDCur) ||
        (pFrame->m_methodDef != mdCur) ||
        (pFrame->m_nVersion != nVersionCur))
    {
        TEST_FAILURE(L"Exited function does not map leaf of shadow stack (" << m_cShadowStackFrameInfos <<
            L" frames high).  Leaf of shadow stack: ModuleID='" <<
            std::hex << pFrame->m_moduleID << L"', methodDef ='0x" << std::hex << pFrame->m_methodDef << L"', Version='" << pFrame->m_nVersion
            << L"'.  Actual function exited: ModuleID='" <<
            std::hex << moduleIDCur << L"', methodDef ='0x" << std::hex << mdCur << L"', Version='" << nVersionCur << 
            L"'.  Was there a tail call or something funky about the stack at this point?");
        return;
    }

    // Do the pop
    memset(pFrame, 0, sizeof(*pFrame));
    m_cShadowStackFrameInfos--;
}

void ReJITScript::VerifyFunctionVersion(
    ModuleID moduleID,
    mdMethodDef methodDef,
    int nVersion)
{
    assert(m_testType == kRejitTestType_Deterministic);

    ModuleInfo moduleInfo = m_moduleIDToInfoMap.Lookup(moduleID);
    int nLatestVersionStored = moduleInfo.m_pMethodDefToLatestVersionMap->Lookup(methodDef);
    if (nVersion == 0)
    {
        TEST_FAILURE(L"The rejitted version of a function was called, but that function should have been reverted!  [ModuleID=" 
            << std::hex << moduleID <<
            L", mdMethodDef=0x" << std::hex << methodDef << L"].  Called version: '" << nVersion << 
            L"'");
    }
    else if (nLatestVersionStored != nVersion)
    {
        TEST_FAILURE(L"Unexpected function version called for [ModuleID=0x" << std::hex << moduleID <<
            L", mdMethodDef=0x" << std::hex << methodDef << L"].  Called version: '" << nVersion << 
            L"'; Expected / latest version stored: '" << nLatestVersionStored << L"'");
    }

    // FUTURE: Could do a stackwalk to obtain FunctionID & RejitID of caller, and verify
    // them as well.
}

BOOL ReJITScript::DoRejitFromRepeatCommandIfNecessary(
    ModuleID moduleID,
    mdMethodDef methodDef)
{
    if (m_iScriptEntryCur >= m_cScriptEntries)
        return FALSE;

    ScriptEntry * pEntry = &(m_rgScriptEntries[m_iScriptEntryCur]);
    if (pEntry->command != ScriptEntry::kWaitRejitRepeatUntil)
        return FALSE;

    // Check first function in WAIT_REJIT_REPEAT_UNTIL to see if it matches the current
    // function we've entered
    if (!DoesFunctionMatchScriptEntry(moduleID, methodDef, pEntry, 1))
        return FALSE;

    // Everything's a match.  Do the rejit
    ExecuteRejitOrRevertCommands(ScriptEntry::kWaitRejitRepeatUntil);
    return TRUE;
}


BOOL ReJITScript::FinishWaitIfAppropriate(
    ModuleID moduleID,
    mdMethodDef methodDef,
    mdMethodDef methodDef2,
    int nVersionCur,
    BOOL fShared,
    HRESULT hrRejitError,
    ScriptEntry::Command command)
{
    if (m_iScriptEntryCur >= m_cScriptEntries)
        return FALSE;

    ScriptEntry * pEntry = &(m_rgScriptEntries[m_iScriptEntryCur]);

    // Are we waiting for the thing that happened?
    if (pEntry->command != command)
        return FALSE;
    
    // If the specified module / function doesn't match what we're waiting for, then this is a
    // no-op.
    BOOL fMatch = FALSE;
    switch(command)
    {
    default:
        assert(!"FinishWaitIfAppropriate encountered unexpected command");
        return FALSE;

    case ScriptEntry::kWaitModule:
        fMatch = DoesModuleMatchScriptEntry(moduleID, fShared, pEntry);
        break;

    case ScriptEntry::kWaitJit:
    case ScriptEntry::kWaitCall:
        fMatch = DoesFunctionMatchScriptEntry(moduleID, methodDef, pEntry, 1);
        break;

    case ScriptEntry::kWaitRejitRepeatUntil:
        fMatch = DoesFunctionMatchScriptEntry(moduleID, methodDef, pEntry, 3);
        break;

    case ScriptEntry::kWaitInline:
        fMatch = 
            (DoesFunctionMatchScriptEntry(moduleID, methodDef, pEntry, 1) &&
            DoesFunctionMatchScriptEntry(moduleID, methodDef2, pEntry, 2));
        break;

    case ScriptEntry::kWaitRejitError:
        fMatch = ( DoesFunctionMatchScriptEntry(moduleID, methodDef, pEntry, 1) &&
                   (hrRejitError == pEntry->hrRejitError) );
        break;
    }

    if (!fMatch)
        return FALSE;

    // We have a match.
    switch(command)
    {
    default:
        assert(!"FinishWaitIfAppropriate encountered unexpected command");
        return FALSE;

    case ScriptEntry::kWaitModule:
        TEST_DISPLAY(L"WAIT_MODULE Verified!  " << pEntry->wszModule << (pEntry->fShared ? L" SHARED" : L" UNSHARED"));
        break;

    case ScriptEntry::kWaitJit:
    case ScriptEntry::kWaitCall:
        TEST_DISPLAY(L"WAIT Verified!  " << pEntry->wszModule << L" " << pEntry->wszClass << L" " << pEntry->wszFunction <<
            L" Version='" << nVersionCur << L"'");
        break;

    case ScriptEntry::kWaitRejitRepeatUntil:
        TEST_DISPLAY(L"UNTIL Verified!  " << pEntry->wszModule << L" " << pEntry->wszClass << L" " << pEntry->wszFunction3 <<
            L"; Breaking out of WAIT_REJIT_REPEAT_UNTIL loop");
        break;

    case ScriptEntry::kWaitInline:
        TEST_DISPLAY(L"WAIT_INLINE Verified!  " << pEntry->wszModule << L" " << pEntry->wszClass << L" '" << pEntry->wszFunction <<
            L"' Inlining '" << pEntry->wszFunction << L"'");
        break;

    case ScriptEntry::kWaitRejitError:
        TEST_DISPLAY(L"WAIT_REJITERROR Verified!  " << pEntry->wszModule << L" " << pEntry->wszClass << L" " << pEntry->wszFunction <<
            L" Encountered expected error = 0x" << std::hex << pEntry->hrRejitError);
        break;
    }

    AdvanceToNextScriptCommand();

    // Now that we're past the wait, do the next batch of stuff
    ExecuteCommandsUntilNextWait();

    return TRUE;
}

BOOL ReJITScript::DoesModuleMatchScriptEntry(ModuleID moduleID, BOOL fShared, ScriptEntry * pEntry)
{
    // Compare module name
    ModuleInfo moduleInfo;
    if (!m_moduleIDToInfoMap.LookupIfExists(moduleID, &moduleInfo))
    {
        // The module isn't in our list, which is probably because it's not an
        // interesting one (i.e., we're not rejitting anything from it).
        return FALSE;
    }
    if (!ContainsAtEnd(moduleInfo.m_wszModulePath, pEntry->wszModule))
        return FALSE;

    // If module name matches, then shared-ness MUST match, or else it's a
    // test failure.  Typical cause is a bogus script, or pre-commands failed.
    if (!!fShared == !!(pEntry->fShared))
    {
        return TRUE;
    }

    TEST_FAILURE(L"Script expected that ModuleID 0x" << std::hex << moduleID << L" would be loaded " <<
        (pEntry->fShared ? L"SHARED" : L"UNSHARED") <<
        L", but it was actually loaded " << (fShared ? L"SHARED" : L"UNSHARED") <<
        L".  Please ensure that the pre-commands to gac (or ungac) modules belonging to this test have successfully run.");
    return FALSE;
}

void ReJITScript::GetClassAndFunctionNamesFromMethodDef(
    IMetaDataImport * pImport,
    ModuleID moduleID, 
    mdMethodDef methodDef,
    PROFILER_WCHAR *wszTypeDefName,
    ULONG cchTypeDefName,
    PROFILER_WCHAR *wszMethodDefName,
    ULONG cchMethodDefName)
{
    HRESULT hr;
    mdTypeDef typeDef;
    ULONG cchMethodDefActual;
    DWORD dwMethodAttr;
    ULONG cchTypeDefActual;
    DWORD dwTypeDefFlags;
    mdTypeDef typeDefBase;

    hr = pImport->GetMethodProps(
        methodDef,
        &typeDef,
        wszMethodDefName,
        cchMethodDefName,
        &cchMethodDefActual,
        &dwMethodAttr,
        NULL,       // [OUT] point to the blob value of meta data
        NULL,       // [OUT] actual size of signature blob
        NULL,       // [OUT] codeRVA
        NULL);      // [OUT] Impl. Flags
    if (FAILED(hr))
    {
        TEST_FAILURE(L"GetMethodProps failed in ModuleID=0x" << std::hex << moduleID << L" for methodDef=0x" << std::hex << methodDef << L".  HRESULT=0x" << std::hex << hr);
    }

    hr = pImport->GetTypeDefProps(
        typeDef,
        wszTypeDefName,
        cchTypeDefName,
        &cchTypeDefActual,
        &dwTypeDefFlags,
        &typeDefBase);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"GetTypeDefProps failed in ModuleID=0x" << std::hex << moduleID << L" for typeDef=0x" << std::hex << typeDef << L".  HRESULT=0x" << std::hex << hr);
    }
}

BOOL ReJITScript::DoesFunctionMatchScriptEntry(
    ModuleID moduleID,
    mdMethodDef methodDef,
    ScriptEntry * pEntry,
    int nScriptFunctionNumber)
{
    ModuleInfo moduleInfo;
    if (!m_moduleIDToInfoMap.LookupIfExists(moduleID, &moduleInfo))
    {
        // The module isn't in our list, which is probably because it's not an
        // interesting one (i.e., we're not rejitting anything from it).
        return FALSE;
    }

    // Compare module name
    if (!ContainsAtEnd(moduleInfo.m_wszModulePath, pEntry->wszModule))
        return FALSE;

    PROFILER_WCHAR wszTypeDefNameTemp[512];
    PLATFORM_WCHAR wszTypeDefName[512];
    PROFILER_WCHAR wszMethodDefNameTemp[512];
    PLATFORM_WCHAR wszMethodDefName[512];
    GetClassAndFunctionNamesFromMethodDef(
        moduleInfo.m_pImport,
        moduleID, 
        methodDef,
        wszTypeDefNameTemp,
        512,
        wszMethodDefNameTemp,
        512);

    ConvertProfilerWCharToPlatformWChar(wszTypeDefName, 512, wszTypeDefNameTemp, 512);
    ConvertProfilerWCharToPlatformWChar(wszMethodDefName, 512, wszMethodDefNameTemp, 512);

    // Compare function name
    const PLATFORM_WCHAR *wszFunctionToCompare = NULL;
    switch(nScriptFunctionNumber)
    {
    default:
        assert(!"nScriptFunctionNumber specified is unexpected");
        return FALSE;

    case 1:
        wszFunctionToCompare = pEntry->wszFunction;
        break;

    case 2:
        wszFunctionToCompare = pEntry->wszFunction2;
        break;

    case 3:
        wszFunctionToCompare = pEntry->wszFunction3;
        break;
    }

    if (wcscmp(wszFunctionToCompare, wszMethodDefName) != 0)
    {
        return FALSE;
    }

    // Compare class name
    if (wcscmp(pEntry->wszClass, wszTypeDefName) != 0)
        return FALSE;

    return TRUE;
}


void ReJITScript::ExecuteCommandsUntilNextWait()
{
    while (m_iScriptEntryCur < m_cScriptEntries)
    {
        ScriptEntry * pEntry = &(m_rgScriptEntries[m_iScriptEntryCur]);

        switch(pEntry->command)
        {
        default:
            assert(!"Found unexpected script command");
            return;

        case ScriptEntry::kWaitJit:
        case ScriptEntry::kWaitCall:
        case ScriptEntry::kWaitModule:
        case ScriptEntry::kWaitInline:
        case ScriptEntry::kWaitRejitError:
        case ScriptEntry::kWaitRejitRepeatUntil:
            // We hit the wait, so stop executing commands.  Next time a function is
            // entered or JITted, we'll check to see if we should advance past the Wait
            return;

        case ScriptEntry::kStackwalk:
            SpawnThreadToDoStackwalk();
            AdvanceToNextScriptCommand();
            break;

        case ScriptEntry::kStopAllThreadLoops:
            m_fStopAllThreadLoops = TRUE;
            TEST_DISPLAY(L"Waiting for " << m_cLoopingThreads << L" rejit / revert looping threads to exit...");
            while (m_cLoopingThreads != 0)
            {
                // Do nothing, m_cLoopingThreads is signalled from another thread
            }
            TEST_DISPLAY(L"...rejit / revert looping threads exited successfully");
            TEST_DISPLAY(L"Total number of attempted rejits: " << m_cRejitStarted);
            AdvanceToNextScriptCommand();
            break;

        case ScriptEntry::kRejit:
        case ScriptEntry::kRevert:
            // Execute the next contiguous block of rejit or revert commands.
            ExecuteRejitOrRevertCommands(pEntry->command);
            // m_iScriptEntryCur is now pointing just after the contiguous block of
            // commands we just executed.
            break;

        case ScriptEntry::kRejitWithInlines:
            // Execute the next contiguous block of rejit or revert commands.
            ExecuteRejitOrRevertCommandsSync(pEntry->command);
            break;


        case ScriptEntry::kRejitThreadLoop:
        case ScriptEntry::kRevertThreadLoop:
            // Execute the next contiguous block of rejit or revert commands.
            ExecuteRejitOrRevertThreadLoopCommands(pEntry->command);
            // m_iScriptEntryCur is now pointing just after the contiguous block of
            // commands we just executed.
            break;

            /*
        case ScriptEntry::kRejitForever:
            ActivateRejitForever(pEntry->wszModule, pEntry->wszClass, pEntry->wszFunction);
            AdvanceToNextScriptCommand();
            break; */

        case ScriptEntry::kNop:
            AdvanceToNextScriptCommand();
            break;
        }
    }
}

HRESULT __stdcall MyStackSnapshotCallback(
                FunctionID funcId,
                UINT_PTR ip,
                COR_PRF_FRAME_INFO frameInfo,
                ULONG32 contextSize,
                BYTE context[],
                void * clientData)
{
    assert(clientData != NULL);
    DSSData * pDssData = (DSSData *) clientData;
    return pDssData->m_pCallback->StackSnapshotCallback(funcId, ip, frameInfo, contextSize, context, pDssData);
}

HRESULT ReJITScript::StackSnapshotCallback(
                FunctionID funcId,
                UINT_PTR ip,
                COR_PRF_FRAME_INFO frameInfo,
                ULONG32 contextSize,
                BYTE context[],
                DSSData * pDssData)
{
    assert(m_testType == kRejitTestType_Deterministic);

    if (funcId == 0)
        return S_OK;

    if (pDssData->m_cDSSFrameInfos >= dimensionof(pDssData->m_rgDSSFrameInfos))
    {
        TEST_FAILURE(L"DSS is reporting more frames than the expected max of " << 
            dimensionof(pDssData->m_rgDSSFrameInfos) << L" frames.  Is there a test bug causing infinite recursion?");
        return S_OK;
    }

    ClassID     clsid;
    ULONG32 cTypeArgsActual;
    ClassID rgTypeArgs[10];

    DSSFrameInfo * pDssFrame = &pDssData->m_rgDSSFrameInfos[pDssData->m_cDSSFrameInfos];

    pDssFrame->m_functionID = funcId;
    pDssFrame->m_ip = ip;

    HRESULT hr = m_pProfilerInfo->GetFunctionInfo2(
        funcId,
        frameInfo,
        &clsid,
        &pDssFrame->m_moduleID,
        &pDssFrame->m_methodDef,
        dimensionof(rgTypeArgs),
        &cTypeArgsActual,
        rgTypeArgs);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"During stackwalk, GetFunctionInfo2 failed with HRESULT=0x" << std::hex << hr);
        return S_OK;
    }

    pDssData->m_cDSSFrameInfos++;

    return S_OK;
}

void ReJITScript::CompareDSSAndShadowStacks(DSSData * pDssData)
{
    assert(m_testType == kRejitTestType_Deterministic);

    int nNextShadowFrame = m_cShadowStackFrameInfos-1;

    // Intentionally start the for loop at 1 instead of 0, so that we skip
    // over the ProfilerHelper!MgdEnteredFunction frame.
    for (int i=1; ((nNextShadowFrame != -1) && (i < pDssData->m_cDSSFrameInfos)); i++)
    {
        BOOL fMatch = CompareDSSAndShadowFrames(
            &pDssData->m_rgDSSFrameInfos[i],
            &m_rgShadowStackFrameInfo[nNextShadowFrame],
            nNextShadowFrame);

        if (!fMatch)
        {
            // See if next DSS frame matches the current shadow stack frame.
            continue;
        }

        nNextShadowFrame--;
    }

    if (nNextShadowFrame != -1)
    {
        // We ran out of DSS frames before verifying all shadow stack frames.
        TEST_FAILURE(L"Stackwalk comparison failed.  There were still " << nNextShadowFrame+1 <<
            L" frames from the shadow stack to verify");
        // TODO: Would be nice to print these frames.  Even better to dump
        // all frames on shadow & DSS walk before comparison even begins.
    }
}

BOOL ReJITScript::CompareDSSAndShadowFrames(
    DSSFrameInfo * pDssFrame, 
    ShadowStackFrameInfo * pShadowFrame,
    int nShadowFrame)
{
    assert(m_testType == kRejitTestType_Deterministic);

    BOOL fMatch = FALSE;
    FunctionID funcId = pDssFrame->m_functionID;
    UINT_PTR ip = pDssFrame->m_ip;
    ModuleID modid = pDssFrame->m_moduleID;
    mdToken tknFromFunctionInfo = pDssFrame->m_methodDef;
    HRESULT hr;
    mdToken tkn;
    mdTypeDef tknClass;
    WCHAR wszMethod[1000];
    ULONG cchMethod;
    DWORD dwAttr;
    PCCOR_SIGNATURE pvSigBlob;
    ULONG       cbSigBlob;
    ULONG       ulCodeRVA;
    DWORD       dwImplFlags;

    COMPtrHolder<IMetaDataImport> pimdi;
    {
        COMPtrHolder<IUnknown> punk;
        hr = m_pProfilerInfo->GetTokenAndMetaDataFromFunction(
            funcId,
            IID_IMetaDataImport,
            &punk,
            &tkn);
        if (FAILED(hr))
        {
            TEST_FAILURE(L"During stackwalk, GetTokenAndMetaDataFromFunction failed with HRESULT=0x" << std::hex << hr);
            return fMatch;
        }

        hr = punk->QueryInterface(IID_IMetaDataImport, (LPVOID*) &pimdi);
        if (FAILED(hr))
        {
            TEST_FAILURE(L"During stackwalk, QueryInterface failed with HRESULT=0x" << std::hex << hr);
            return fMatch;
        }
    }

    hr = pimdi->GetMethodProps( 
        tkn,                     // The method for which to get props.   
        &tknClass,                // Put method's class here. 
        wszMethod,              // Put method's name here.  
        dimensionof(wszMethod), // Size of szMethod buffer in wide chars.   
        &cchMethod,             // Put actual size here 
        &dwAttr,               // Put flags here.  
        &pvSigBlob,        // [OUT] point to the blob value of meta data   
        &cbSigBlob,            // [OUT] actual size of signature blob  
        &ulCodeRVA,            // [OUT] codeRVA    
        &dwImplFlags);    // [OUT] Impl. Flags    
    if (FAILED(hr))
    {
        TEST_FAILURE(L"During stackwalk, GetMethodProps failed with HRESULT=0x" << std::hex << hr);
        return fMatch;
    }

    FunctionID funcIDFromGetFunctionFromIP2;
    ReJITID reJITID;
    hr = m_pProfilerInfo->GetFunctionFromIP2((LPCBYTE) ip, &funcIDFromGetFunctionFromIP2, &reJITID);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"During stackwalk, GetFunctionFromIP2 failed with HRESULT=0x" << std::hex << hr);
        return fMatch;
    }
    if (funcId != funcIDFromGetFunctionFromIP2)
    {
        TEST_FAILURE(L"Stackwalk encountered FunctionID '0x" << std::hex << funcId << 
            L"', which didn't match FunctionID returned from GetFunctionFromIP2: '0x" << std::hex << funcIDFromGetFunctionFromIP2 << L"'");
    }

    TEST_DISPLAY(L"STACKWALK FRAME '" << wszMethod << L"': FuncID=0x" << std::hex << funcId << L", RejitID=0x" << std::hex << reJITID << 
        L", ModID=0x" << std::hex << modid << L", methodDef=0x" << std::hex << tkn);

    // Convert the version number from the shadow stack to a rejitid
    ReJITID rejitIDShadow = (ReJITID) -1;
    FunctionVersion funcVersion;
    fMatch = m_functionIDToVersionMap.LookupIfExists(funcId, &funcVersion);
    if (fMatch)
    {
        assert(pShadowFrame->m_nVersion < dimensionof(funcVersion.m_rgRejitIDs));
        rejitIDShadow = funcVersion.m_rgRejitIDs[pShadowFrame->m_nVersion];

        // Compare!
        fMatch = 
            ((pShadowFrame->m_moduleID == modid) &&
            (pShadowFrame->m_methodDef == tknFromFunctionInfo) &&
            (rejitIDShadow == reJITID));
    }

    if (fMatch)
    {
        TEST_DISPLAY(L"STACKWALK frame matches shadow stack frame!");
    }
    else
    {
        TEST_DISPLAY(L"Skipping mismatched frame from DoStackSnapshot.  Shadow stack frame "
            << nShadowFrame << L": ModuleID='"
            << std::hex << pShadowFrame->m_moduleID << L"', methodDef='0x" << std::hex << pShadowFrame->m_methodDef << L"', rejitid='0x" << std::hex << rejitIDShadow <<
            L"', Version='" << pShadowFrame->m_nVersion << L"'.  DSS: ModuleID='"
            << std::hex << modid << L"', methodDef='0x" << std::hex << tknFromFunctionInfo << L"', rejitid='0x" << std::hex << reJITID << L"'.");
    }

    return fMatch;
}


void ReJITScript::SpawnThreadToDoStackwalk()
{
    assert(m_testType == kRejitTestType_Deterministic);

    DSSData dssData = {0};
    dssData.m_pCallback = this;
    HRESULT hr = m_pProfilerInfo->GetCurrentThreadID(&(dssData.m_threadID));
    if (FAILED(hr))
    {
        TEST_FAILURE(L"GetCurrentThreadID failed with HRESULT=0x0x" << std::hex << hr);
    }

#ifdef _WIN32
    // DoStackSnapshot currently does not work on non-windows platforms.
    std::thread stackWalkThread(DoStackwalkThreadProc, &dssData);

    TEST_DISPLAY(L"Waiting for DoStackwalkThreadProc thread to complete...");

     stackWalkThread.join();
     TEST_DISPLAY(L"DoStackwalkThreadProc thread complete!");
#endif // _WIN32
}

/* static */
DWORD WINAPI ReJITScript::DoStackwalkThreadProc(LPVOID pParameter)
{
    DSSData * dssData = (DSSData *) pParameter;
    ReJITScript * pCallback = dssData->m_pCallback;
    pCallback->DoStackwalk(dssData);
    return 1;
}


void ReJITScript::DoStackwalk(DSSData * pDssData)
{
    assert(m_testType == kRejitTestType_Deterministic);

    HRESULT hr = m_pProfilerInfo->DoStackSnapshot(
        pDssData->m_threadID,
        MyStackSnapshotCallback,
        COR_PRF_SNAPSHOT_DEFAULT,
        (LPVOID) pDssData,
        NULL,                           // [in, size_is(contextSize)] BYTE context[],
        0);                             // [in] ULONG32 contextSize)
    if (FAILED(hr))
    {
        TEST_FAILURE(L"DSS Failed with HRESULT=0x" << std::hex << hr);
    }

    CompareDSSAndShadowStacks(pDssData);
}

void  ReJITScript::ExecuteRejitOrRevertCommandsSync(ScriptEntry::Command cmd)
{
    ReJitTestInfo parameter;
    InitThreadParamForRejitOrRevert(cmd, &parameter);
    TestReJitOrRevertStatic(&parameter);
}

void ReJITScript::ExecuteRejitOrRevertCommands(ScriptEntry::Command cmd)
{
    ReJitTestInfo parameter;
    InitThreadParamForRejitOrRevert(cmd, &parameter);

    DWORD dwThreadId = 0;

    // NOTE!  Passing stack memory from the current thread to this newly created thread.
    //  This function must not return until the newly created thread has exited!
    std::thread rejitThread(TestReJitOrRevertStatic, &parameter);

    TEST_DISPLAY(L"Waiting for rejit thread to complete...");

    rejitThread.join();
    TEST_DISPLAY(L"Rejit thread complete!");
}

void ReJITScript::ExecuteRejitOrRevertThreadLoopCommands(ScriptEntry::Command cmd)
{
    m_cRejitStarted = 0;
    ReJitTestInfo * pParameter = new ReJitTestInfo;
    InitThreadParamForRejitOrRevert(cmd, pParameter);

    ++m_cLoopingThreads;

    std::thread rejitThread(TestReJitOrRevertStatic, pParameter);
    
    TEST_DISPLAY(L"Rejit thread loop created and running");
    rejitThread.detach();
}

void ReJITScript::InitThreadParamForRejitOrRevert(ScriptEntry::Command cmd, ReJitTestInfo * pParameter)
{
    assert(m_iScriptEntryCur < m_cScriptEntries);
    assert(m_rgScriptEntries[m_iScriptEntryCur].command == cmd);

    // Batch the next contiguous rejit commands into a single request. Do this by
    // creating a new thread to send the batch list to the profiling API. First, provide
    // this new thread with the script entry range of rejit commands to send.
    pParameter->m_pCallback = this;
    pParameter->m_iCommandFirst = m_iScriptEntryCur;

    if (cmd == ScriptEntry::kWaitRejitRepeatUntil)
    {
        // If we're at a WAIT_REJIT_REPEAT_UNTIL, don't batch, and don't advance to next
        // command
        pParameter->m_iCommandLast = m_iScriptEntryCur;
        return;
    }

    for (; m_iScriptEntryCur < m_cScriptEntries; AdvanceToNextScriptCommand())
    {
        ScriptEntry * pEntry = &(m_rgScriptEntries[m_iScriptEntryCur]);

        if (pEntry->command != cmd)
        {
            // Stop batching once we hit the first non-matching command
            break;
        }
    }

    pParameter->m_iCommandLast = m_iScriptEntryCur-1;
}

#pragma warning(pop)
