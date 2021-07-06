// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "stdafx.h"


InjectMetaData * g_pInjectMD = NULL;

HRESULT InjectMetaData_Verify(IPrfCom * pPrfCom)
{
    return reinterpret_cast<InjectMetaData *>(pPrfCom->m_pTestClassInstance)->Verify();
}

HRESULT InjectMetaData_ReJITCompilationStarted(IPrfCom * pPrfCom, FunctionID functionID, ReJITID rejitId, BOOL fIsSafeToBlock)
{
    return reinterpret_cast<InjectMetaData *>(pPrfCom->m_pTestClassInstance)->ReJITCompilationStarted(functionID, rejitId, fIsSafeToBlock);
}

HRESULT InjectMetaData_GetReJITParameters(IPrfCom * pPrfCom, ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl)
{
    return reinterpret_cast<InjectMetaData *>(pPrfCom->m_pTestClassInstance)->GetReJITParameters(moduleId, methodId, pFunctionControl);
}

HRESULT InjectMetaData_ReJITError(IPrfCom * pPrfCom, ModuleID moduleId, mdMethodDef methodId, FunctionID functionId, HRESULT hrStatus)
{
    return reinterpret_cast<InjectMetaData *>(pPrfCom->m_pTestClassInstance)->ReJITError(moduleId, methodId, functionId, hrStatus);
}

HRESULT InjectMetaData_AppDomainCreationStarted(IPrfCom * pPrfCom, AppDomainID appDomainID)
{
    return reinterpret_cast<InjectMetaData *>(pPrfCom->m_pTestClassInstance)->AppDomainCreationStarted(appDomainID);
}

HRESULT InjectMetaData_AppDomainShutdownFinished(IPrfCom * pPrfCom, AppDomainID appDomainID, HRESULT hrStatus)
{
    return reinterpret_cast<InjectMetaData *>(pPrfCom->m_pTestClassInstance)->AppDomainShutdownFinished(appDomainID, hrStatus);
}

HRESULT InjectMetaData_ModuleLoadFinished(IPrfCom * pPrfCom, ModuleID moduleID, HRESULT hrStatus)
{
    return reinterpret_cast<InjectMetaData *>(pPrfCom->m_pTestClassInstance)->ModuleLoadFinished(moduleID, hrStatus);
}

HRESULT InjectMetaData_ModuleUnloadStarted(IPrfCom * pPrfCom, ModuleID moduleID)
{
    return reinterpret_cast<InjectMetaData *>(pPrfCom->m_pTestClassInstance)->ModuleUnloadStarted(moduleID);
}

HRESULT InjectMetaData_JITCompilationStarted(IPrfCom * pPrfCom, FunctionID functionID, BOOL fIsSafeToBlock)
{
    return reinterpret_cast<InjectMetaData *>(pPrfCom->m_pTestClassInstance)->JITCompilationStarted(functionID, fIsSafeToBlock);
}

// static
void InjectMetaData::Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& testName)
{
    wprintf(L"In InjectMetaData::Initialize");
    InjectMetaData * pInjectMD = NULL; 
    TestType testType;
    if (testName == L"NewUserString")
    {
        testType = kNewUserString;
        pInjectMD = new InjectNewUserDefinedString(pPrfCom, testType);
    }
    else if (testName == L"NewTypes")
    {
        testType = kInjectTypes;
        pInjectMD = new InjectTypes(pPrfCom, testType);
    }
    else if (testName == L"NewGenericTypes")
    {
        testType = kInjectGenericTypes;
        pInjectMD = new InjectGenericTypes(pPrfCom, testType);
    }
    else if (testName == L"NewModuleRef")
    {
        testType = kInjectModuleRef;
        pInjectMD = new InjectModuleRef(pPrfCom, testType);
    }
    else if (testName == L"NewAssemblyRef")
    {
        testType = kInjectAssemblyRef;
        pInjectMD = new InjectAssemblyRef(pPrfCom, testType);
    }
    else if (testName == L"NewMethodSpec")
    {
        testType = kInjectMethodSpec;
        pInjectMD = new InjectMethodSpec(pPrfCom, testType);
    }
    else if (testName == L"NewTypeSpec")
    {
        testType = kInjectTypeSpec;
        pInjectMD = new InjectTypeSpec(pPrfCom, testType);
    }
    else if (testName == L"NewExternalMethodCall")
    {
        testType = kInjectExternalMethodCall;
        pInjectMD = new InjectExternalCall(pPrfCom, testType);
    }
    else if (testName == L"NewExternalMethodCall_CoreLib")
    {
        testType = kInjectExternalMethodCallCoreLib;
        pInjectMD = new InjectExternalCall(pPrfCom, testType);
    }
    else
    {
        return;
    }

    // Create new test object
    SET_CLASS_POINTER(pInjectMD);

    pModuleMethodTable->FLAGS =
        COR_PRF_MONITOR_MODULE_LOADS
        | COR_PRF_MONITOR_ASSEMBLY_LOADS
        | COR_PRF_MONITOR_APPDOMAIN_LOADS
        | COR_PRF_MONITOR_JIT_COMPILATION
        | COR_PRF_MONITOR_THREADS
        | COR_PRF_ENABLE_REJIT
        // | COR_PRF_DISABLE_ALL_NGEN_IMAGES // Note:: We never want to add this flag.
        ;

    REGISTER_CALLBACK(VERIFY, InjectMetaData_Verify);
    REGISTER_CALLBACK(REJITCOMPILATIONSTARTED, InjectMetaData_ReJITCompilationStarted);
    REGISTER_CALLBACK(GETREJITPARAMETERS, InjectMetaData_GetReJITParameters);
    REGISTER_CALLBACK(REJITERROR, InjectMetaData_ReJITError);

    REGISTER_CALLBACK(APPDOMAINCREATIONSTARTED, InjectMetaData_AppDomainCreationStarted);
    REGISTER_CALLBACK(APPDOMAINSHUTDOWNFINISHED, InjectMetaData_AppDomainShutdownFinished);
    REGISTER_CALLBACK(MODULELOADFINISHED, InjectMetaData_ModuleLoadFinished);
    REGISTER_CALLBACK(MODULEUNLOADSTARTED, InjectMetaData_ModuleUnloadStarted);
    REGISTER_CALLBACK(JITCOMPILATIONSTARTED, InjectMetaData_JITCompilationStarted);
}


//---------------------------------------------------------------------------------------
//
// InjectMetaData implementation
//

InjectMetaData::InjectMetaData(IPrfCom * pPrfCom, TestType testType) :
    m_testType(testType),
    m_pProfilerInfo(NULL),
    m_pPrfCom(pPrfCom),
    m_ulRejitCallCount(0),
    m_modidTarget(NULL),
    m_tokmdTargetMethod(mdMethodDefNil)
{
    HRESULT hr = pPrfCom->m_pInfo->QueryInterface(IID_ICorProfilerInfo7, (void**)&m_pProfilerInfo);
    if (FAILED(hr))
    {
        FAILURE(L"QueryInterface() for ICorProfilerInfo7 failed.");
    }

    g_pInjectMD = this;
}


InjectMetaData::~InjectMetaData()
{
}

HRESULT InjectMetaData::Verify()
{
    TEST_FAILURE(L"Verify should be implemented in the derived classes");
    return E_FAIL;
}

HRESULT InjectMetaData::AppDomainCreationStarted(AppDomainID appDomainID)
{
    return S_OK;
}

HRESULT InjectMetaData::AppDomainShutdownFinished(AppDomainID appDomainID,
    HRESULT hrStatus)
{
    return S_OK;
}


HRESULT InjectMetaData::ModuleLoadFinished(ModuleID moduleID, HRESULT hrStatus)
{
    LPCBYTE pbBaseLoadAddr;
    PROFILER_WCHAR wszName[DEFAULT_STRING_LENGTH];
    PLATFORM_WCHAR wszNameTemp[DEFAULT_STRING_LENGTH];
    ULONG cchNameIn = dimensionof(wszName);
    ULONG cchNameOut;
    AssemblyID assemblyID;
    DWORD dwModuleFlags;
    HRESULT hr = m_pProfilerInfo->GetModuleInfo2(
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

    TEST_DISPLAY(L"Module Loaded:" << wszNameTemp);

    if (ContainsAtEnd(wszNameTemp, MDINJECTSOURCE_WITHEXT) || ContainsAtEnd(wszNameTemp, MDINJECTSOURCE_WITHOUTEXT))
    {
        m_modidSource = moduleID;
        TEST_DISPLAY(L"MDInjectSource found on ModuleLoadFinished");

        {
            COMPtrHolder<IUnknown> pUnk;
            hr = m_pProfilerInfo->GetModuleMetaData(moduleID, ofWrite, IID_IMetaDataImport2, &pUnk);
            if (FAILED(hr))
            {
                TEST_FAILURE(L"GetModuleMetaData failed for IID_IMetaDataImport2 in ModuleID '" << HEX(moduleID) << L"' (" << wszNameTemp << L")");
            }
            hr = pUnk->QueryInterface(IID_IMetaDataImport2, (LPVOID *)&m_pSourceImport);
            if (FAILED(hr))
            {
                TEST_FAILURE(L"Failed QI'ing for IID_IMetaDataImport2 for ModuleID '" << HEX(moduleID) << L"' (" << wszNameTemp << L")");
            }
        }
    }

    if (ContainsAtEnd(wszNameTemp, MDINJECTTARGET_WITHEXT) || ContainsAtEnd(wszNameTemp, MDINJECTTARGET_NATIVEIMAGE_WITHEXT))
    {
        m_modidTarget = moduleID;
        TEST_DISPLAY(L"MDInjectTarget found on ModuleLoadFinished");

        {
            COMPtrHolder<IUnknown> pUnk;
            hr = m_pProfilerInfo->GetModuleMetaData(moduleID, ofWrite, IID_IMetaDataEmit2, &pUnk);
            if (FAILED(hr))
            {
                TEST_FAILURE(L"GetModuleMetaData failed for IID_IMetaDataEmit2 in ModuleID '" << HEX(moduleID) << L"' (" << wszNameTemp << L")");
            }
            hr = pUnk->QueryInterface(IID_IMetaDataEmit2, (LPVOID *)&m_pTargetEmit);
            if (FAILED(hr))
            {
                TEST_FAILURE(L"Failed QI'ing for IID_IMetaDataEmit2 for ModuleID '" << HEX(moduleID) << L"' (" << wszNameTemp << L")");
            }
        }

        {
            COMPtrHolder<IUnknown> pUnk;
            hr = m_pProfilerInfo->GetModuleMetaData(moduleID, ofRead, IID_IMetaDataImport2, &pUnk);
            if (FAILED(hr))
            {
                TEST_FAILURE(L"GetModuleMetaData failed for ModuleID '" << HEX(moduleID) << L"' (" << wszNameTemp << L")");
            }
            hr = pUnk->QueryInterface(IID_IMetaDataImport2, (LPVOID *)&m_pTargetImport);
            if (FAILED(hr))
            {
                TEST_FAILURE(L"Failed QI'ing for IID_IMetaDataImport2 for ModuleID '" << HEX(moduleID) << L"' (" << wszNameTemp << L")");
            }
        }

        {
            COMPtrHolder<IUnknown> pUnk;
            hr = m_pProfilerInfo->GetModuleMetaData(moduleID, ofWrite, IID_IMetaDataAssemblyEmit, &pUnk);
            if (FAILED(hr))
            {
                TEST_FAILURE(L"GetModuleMetaData failed for IID_IMetaDataAssemblyEmit in ModuleID '" << HEX(moduleID) << L"' (" << wszNameTemp << L")");
            }
            hr = pUnk->QueryInterface(IID_IMetaDataAssemblyEmit, (LPVOID *)&m_pAssemblyEmit);
            if (FAILED(hr))
            {
                TEST_FAILURE(L"Failed QI'ing for IID_IMetaDataAssemblyEmit for ModuleID '" << HEX(moduleID) << L"' (" << wszNameTemp << L")");
            }
        }

        {
            COMPtrHolder<IUnknown> pUnk;
            hr = m_pProfilerInfo->GetModuleMetaData(moduleID, ofRead, IID_IMetaDataAssemblyImport, &pUnk);
            if (FAILED(hr))
            {
                TEST_FAILURE(L"GetModuleMetaData failed for IID_IMetaDataAssemblyImport in ModuleID '" << HEX(moduleID) << L"' (" << wszNameTemp << L")");
            }
            hr = pUnk->QueryInterface(IID_IMetaDataAssemblyImport, (LPVOID *)&m_pAssemblyImport);
            if (FAILED(hr))
            {
                TEST_FAILURE(L"Failed QI'ing for IID_IMetaDataAssemblyImport for ModuleID '" << HEX(moduleID) << L"' (" << wszNameTemp << L")");
            }
        }
    }

    return S_OK;
}


HRESULT InjectMetaData::ModuleUnloadStarted(ModuleID moduleID)
{
    return S_OK;
}


void InjectMetaData::CallRequestReJIT(const vector<COR_PRF_METHOD> &methods)
{
    size_t count = methods.size();
    ModuleID * moduleIDs = new ModuleID[count];
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

void InjectMetaData::CallRequestReJIT(
    UINT cFunctionsToRejit,
    ModuleID * rgModuleIDs,
    mdMethodDef * rgMethodIDs)
{
    HRESULT hr = m_pProfilerInfo->RequestReJIT(cFunctionsToRejit, rgModuleIDs, rgMethodIDs);
    if (FAILED(hr))
    {
        TEST_FAILURE(L"RequestReJIT failed with " << HEX(hr));
        return;
    }

    TEST_DISPLAY(L"RequestReJIT called with " << cFunctionsToRejit << L" methods; returned success");
}

// ICorProfilerCallback4 Callbacks
//

HRESULT InjectMetaData::ReJITCompilationStarted(FunctionID functionID, ReJITID rejitId, BOOL fIsSafeToBlock)
{
    m_ulRejitCallCount++;
    return S_OK;
}

HRESULT InjectMetaData::GetReJITParameters(
    ModuleID moduleId,
    mdMethodDef methodId,
    ICorProfilerFunctionControl *pFunctionControl)
{
    TEST_DISPLAY(L"InjectMetaData::GetReJITParameters called, methodDef = " << HEX(methodId));
    return S_OK;
}

HRESULT InjectMetaData::ReJITError(
    ModuleID moduleId,
    mdMethodDef methodId,
    FunctionID functionId,
    HRESULT hrStatus)
{
    TEST_FAILURE(L"ReJITError called.  ModuleID=" << HEX(moduleId) << L", methodDef=" << HEX(methodId) << L", FunctionID=" << HEX(functionId) << L", HR=" << HEX(hrStatus));
    return hrStatus;
}

HRESULT InjectMetaData::JITCompilationStarted(FunctionID functionID, BOOL fIsSafeToBlock)
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
        else if (wcscmp(wszMethodDefNamePlatform, CALLMETHOD) == 0)
        {
            m_tokmdTargetMethod = methodDef;
        }
    }

    return S_OK;
}

// VERIFICATION

void InjectMetaData::GetClassAndFunctionNamesFromMethodDef(
    IMetaDataImport * pImport,
    ModuleID moduleID,
    mdMethodDef methodDef,
    LPWSTR wszTypeDefName,
    ULONG cchTypeDefName,
    LPWSTR wszMethodDefName,
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
        TEST_FAILURE(L"GetMethodProps failed in ModuleID=" << HEX(moduleID) << L" for methodDef=" << HEX(methodDef) << L".  HRESULT=" << HEX(hr));
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
        TEST_FAILURE(L"GetTypeDefProps failed in ModuleID=" << HEX(moduleID) << L" for typeDef=" << HEX(typeDef) << L".  HRESULT=" << HEX(hr));
    }
}

int InjectMetaData::NativeFunction()
{
    TEST_FAILURE(L"Unexpected. Test class should implement this method.");
    return -1;
}
