// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// Prf2_CodeProfiler.cpp
//
// ======================================================================================

#include "../../ProfilerCommon.h"

class CodeProfiler
{
public:

    CodeProfiler(IPrfCom * pPrfCom);
    ~CodeProfiler();
    HRESULT Verify(IPrfCom * pPrfCom);

#pragma region static_wrapper_methods
    static HRESULT AppDomainCreationFinishedWrapper(IPrfCom * pPrfCom,
        AppDomainID appDomainId,
        HRESULT hrStatus)
    {
        return STATIC_CLASS_CALL(CodeProfiler)->AppDomainCreationFinished(pPrfCom, appDomainId, hrStatus);
    }

    static HRESULT AssemblyLoadFinishedWrapper(IPrfCom * pPrfCom,
        AssemblyID assemblyId,
        HRESULT hrStatus)
    {
        return STATIC_CLASS_CALL(CodeProfiler)->AssemblyLoadFinished(pPrfCom, assemblyId, hrStatus);
    }

    static HRESULT ClassLoadFinishedWrapper(IPrfCom * pPrfCom,
        ClassID classId,
        HRESULT hrStatus)
    {
        return STATIC_CLASS_CALL(CodeProfiler)->ClassLoadFinished(pPrfCom, classId, hrStatus);
    }

    static HRESULT ExceptionCatcherEnterWrapper(IPrfCom * pPrfCom,
        FunctionID functionId,
        ObjectID objectId)
    {
        return STATIC_CLASS_CALL(CodeProfiler)->ExceptionCatcherEnter(pPrfCom, functionId, objectId);
    }

    static HRESULT ExceptionSearchFilterEnterWrapper(IPrfCom * pPrfCom,
        FunctionID functionId)
    {
        return STATIC_CLASS_CALL(CodeProfiler)->ExceptionSearchFilterEnter(pPrfCom, functionId);
    }

    static HRESULT ExceptionUnwindFinallyEnterWrapper(IPrfCom * pPrfCom,
        FunctionID functionId)
    {
        return STATIC_CLASS_CALL(CodeProfiler)->ExceptionUnwindFinallyEnter(pPrfCom, functionId);
    }

    static HRESULT JITCachedFunctionSearchFinishedWrapper(IPrfCom * pPrfCom,
        FunctionID functionId,
        COR_PRF_JIT_CACHE result)
    {
        return STATIC_CLASS_CALL(CodeProfiler)->JITCachedFunctionSearchFinished(pPrfCom, functionId, result);
    }

    static HRESULT JITCompilationStartedWrapper(IPrfCom * pPrfCom,
        FunctionID functionId,
        BOOL fIsSafeToBlock)
    {
        return STATIC_CLASS_CALL(CodeProfiler)->JITCompilationStarted(pPrfCom, functionId, fIsSafeToBlock);
    }

    static HRESULT JITCompilationFinishedWrapper(IPrfCom * pPrfCom,
        FunctionID functionId,
        HRESULT hrStatus,
        BOOL fIsSafeToBlock)
    {
        return STATIC_CLASS_CALL(CodeProfiler)->JITCompilationFinished(pPrfCom, functionId, hrStatus, fIsSafeToBlock);
    }

    static HRESULT ModuleLoadFinishedWrapper(IPrfCom * pPrfCom,
        ModuleID moduleId,
        HRESULT hrStatus)
    {
        return STATIC_CLASS_CALL(CodeProfiler)->ModuleLoadFinished(pPrfCom, moduleId, hrStatus);
    }

#pragma endregion

private:
#pragma region callback_handler_prototypes
    HRESULT CodeProfiler::AppDomainCreationFinished(
        IPrfCom * /* pPrfCom */ ,
        AppDomainID /* appDomainId */ ,
        HRESULT /* hrStatus */ );

    HRESULT CodeProfiler::AssemblyLoadFinished(
        IPrfCom * /* pPrfCom */ ,
        AssemblyID /* assemblyId */ ,
        HRESULT /* hrStatus */ );

    HRESULT CodeProfiler::ClassLoadFinished(
        IPrfCom * /* pPrfCom */ ,
        ClassID /* classId */ ,
        HRESULT /* hrStatus */ );

    HRESULT CodeProfiler::ExceptionCatcherEnter(
        IPrfCom * /* pPrfCom */ ,
        FunctionID /* functionId */ ,
        ObjectID /* objectId */ );

    HRESULT CodeProfiler::ExceptionSearchFilterEnter(
        IPrfCom * /* pPrfCom */ ,
        FunctionID /* functionId */ );

    HRESULT CodeProfiler::ExceptionUnwindFinallyEnter(
        IPrfCom * /* pPrfCom */ ,
        FunctionID /* functionId */ );

    HRESULT CodeProfiler::JITCachedFunctionSearchFinished(
        IPrfCom * /* pPrfCom */ ,
        FunctionID /* functionId */ ,
        COR_PRF_JIT_CACHE /* result */ );

    HRESULT CodeProfiler::JITCompilationStarted(
        IPrfCom * /* pPrfCom */ ,
        FunctionID /* functionId */ ,
        BOOL /* fIsSafeToBlock */ );

    HRESULT CodeProfiler::JITCompilationFinished(
        IPrfCom * /* pPrfCom */ ,
        FunctionID /* functionId */ ,
        HRESULT /* hrStatus */ ,
        BOOL /* fIsSafeToBlock */ );

    HRESULT CodeProfiler::ModuleLoadFinished(
        IPrfCom * /* pPrfCom */ ,
        ModuleID /* moduleId */ ,
        HRESULT /* hrStatus */ );

#pragma endregion

};

/*
*  Initialize the CodeProfiler class.  
*/
CodeProfiler::CodeProfiler(IPrfCom * /* pPrfCom */)
{
}

/*
*  Clean up
*/
CodeProfiler::~CodeProfiler()
{
}

HRESULT CodeProfiler::AppDomainCreationFinished(
    IPrfCom *pPrfCom,
    AppDomainID appDomainId,
    HRESULT hrStatus)
{
    if (FAILED(hrStatus))
    {
        return S_OK;
    }
    if (NULL == appDomainId)
    {
        FAILURE(L"AppDomainCreationFinished: AppDomainID cannot be NULL");
        return E_FAIL;
    }
    WCHAR appDomainName[STRING_LENGTH];
    ULONG nameLength = 0;
    ProcessID processId = 0;
    MUST_PASS(PINFO->GetAppDomainInfo(appDomainId, STRING_LENGTH,
        &nameLength, appDomainName, &processId));
    return S_OK;
}

HRESULT CodeProfiler::AssemblyLoadFinished(
    IPrfCom *pPrfCom,
    AssemblyID assemblyId,
    HRESULT hrStatus)
{
    if (FAILED(hrStatus))
    {
        return S_OK;
    }
    if (NULL == assemblyId)
    {
        FAILURE(L"AssemblyLoadFinished: AssemblyID cannot be NULL");
        return E_FAIL;
    }
    ULONG dummy;
    AppDomainID appDomainId;
    ModuleID moduleId;
    WCHAR assemblyName[LONG_LENGTH];
    MUST_PASS(PINFO->GetAssemblyInfo( assemblyId,
	 								       LONG_LENGTH,
	                                       &dummy,
	                                       assemblyName,
	                                       &appDomainId,
									       &moduleId ));
    return S_OK;
}

HRESULT CodeProfiler::ClassLoadFinished(
                                        IPrfCom *pPrfCom,
                                        ClassID classId,
                                        HRESULT hrStatus)
{
    if (FAILED(hrStatus))
    {
        return S_OK;
    }
    if (NULL == classId)
    {
        FAILURE(L"ClassLoadFinished: ClassID cannot be NULL");
        return E_FAIL;
    }
    ModuleID modId;
    mdTypeDef classToken;
    ClassID parentClassID;
    ULONG32 nTypeArgs;
    ClassID typeArgs[SHORT_LENGTH];
    HRESULT hr = PINFO->GetClassIDInfo2(classId,
                                &modId,
                                &classToken,
                                &parentClassID,
                                SHORT_LENGTH,
                                &nTypeArgs,
                                typeArgs);
    if ((S_OK != hr) && (CORPROF_E_CLASSID_IS_ARRAY != hr) && (CORPROF_E_CLASSID_IS_COMPOSITE != hr))
    {
        FAILURE(L"ClassLoadFinished: GetClassIDInfo2 failed, returned " << HEX(hr));
        return E_FAIL;
    }
    else
    {
        return S_OK;
    }
}

HRESULT CodeProfiler::ExceptionCatcherEnter(
    IPrfCom *pPrfCom,
    FunctionID /* functionId */ ,
    ObjectID /* objectId */ )
{
    COR_PRF_EX_CLAUSE_INFO pinfo;
    HRESULT hr = PINFO->GetNotifiedExceptionClauseInfo(&pinfo);
    if (hr != S_OK)
    {
        FAILURE(L"ExceptionCatcherEnter: GetNotifiedExceptionClauseInfo failed, returned " << HEX(hr));
    }
    return hr;
}
HRESULT CodeProfiler::ExceptionSearchFilterEnter(
    IPrfCom *pPrfCom,
    FunctionID /* functionId */ )
{
    COR_PRF_EX_CLAUSE_INFO pinfo;
    HRESULT hr = PINFO->GetNotifiedExceptionClauseInfo(&pinfo);
    if (hr != S_OK)
    {
        FAILURE(L"ExceptionSearchFilterEnter: GetNotifiedExceptionClauseInfo failed, returned " << HEX(hr));
    }
    return hr;
}

HRESULT CodeProfiler::ExceptionUnwindFinallyEnter(
    IPrfCom *pPrfCom,
    FunctionID /* functionId */ )
{
    COR_PRF_EX_CLAUSE_INFO pinfo;
    HRESULT hr = PINFO->GetNotifiedExceptionClauseInfo(&pinfo);
    if (hr != S_OK)
    {
        FAILURE(L"ExceptionUnwindFinallyEnter: GetNotifiedExceptionClauseInfo failed, returned " << HEX(hr));
    }
    return hr;
}

HRESULT CodeProfiler::JITCachedFunctionSearchFinished(
    IPrfCom *pPrfCom,
    FunctionID functionId ,
    COR_PRF_JIT_CACHE result )
{
    if ((COR_PRF_CACHED_FUNCTION_FOUND != result) || (NULL == functionId))
    {
        return S_OK;
    }
    COR_PRF_FRAME_INFO frameInfo = NULL;
    ClassID classId = NULL;
    ModuleID moduleId = NULL;
    mdToken token = NULL;
    ULONG32 nTypeArgs = NULL;
    ClassID typeArgs[SHORT_LENGTH];

    MUST_PASS(PINFO->GetFunctionInfo2(functionId, 
                                  frameInfo,
                                  &classId,
                                  &moduleId,
                                  &token,
                                  SHORT_LENGTH,
                                  &nTypeArgs,
                                  typeArgs));
    return S_OK;
}

HRESULT CodeProfiler::JITCompilationStarted(
    IPrfCom * pPrfCom ,
    FunctionID functionId ,
    BOOL /* fIsSafeToBlock */)
{
    HRESULT hr;
    COR_PRF_FRAME_INFO frameInfo = NULL;
    ClassID classId = NULL;
    ModuleID moduleId = NULL;
    mdToken token = NULL;
    ULONG32 nTypeArgs = NULL;
    ClassID typeArgs[SHORT_LENGTH];
    LPCBYTE pMethodHeader = NULL;
    ULONG iMethodSize = 0;

    hr = PINFO->GetFunctionInfo2(functionId, 
                                  frameInfo,
                                  &classId,
                                  &moduleId,
                                  &token,
                                  SHORT_LENGTH,
                                  &nTypeArgs,
                                  typeArgs);
    if (FAILED(hr))
    {
        DISPLAY(L"GetFunctionInfo2 in JITCompilationStarted failed.");
        return S_OK;
    }

    hr = PINFO->GetILFunctionBody(moduleId, token, &pMethodHeader, &iMethodSize);
    if (FAILED(hr))
    {
        DISPLAY(L"GetILFunctionBody in JITCompilationStarted failed.");
        return S_OK;
    }

    return S_OK;
}

HRESULT CodeProfiler::JITCompilationFinished(
    IPrfCom *pPrfCom,
    FunctionID functionId ,
    HRESULT hrStatus ,
    BOOL /* fIsSafeToBlock */ )
{
    if ((FAILED(hrStatus)) || (NULL == functionId))
    {
        return S_OK;
    }

    COR_PRF_FRAME_INFO frameInfo = NULL;
    ClassID classId = NULL;
    ModuleID moduleId = NULL;
    mdToken token = NULL;
    ULONG32 nTypeArgs = NULL;
    ClassID typeArgs[SHORT_LENGTH];

    MUST_PASS(PINFO->GetFunctionInfo2(functionId, 
        frameInfo,
        &classId,
        &moduleId,
        &token,
        SHORT_LENGTH,
        &nTypeArgs,
        typeArgs));

    IMetaDataImport * pIMDImport = NULL;

    MUST_PASS(PINFO->GetModuleMetaData(moduleId,
        ofRead,
        IID_IMetaDataImport,
        (IUnknown **)&pIMDImport));

    WCHAR funcName[STRING_LENGTH];
    MUST_PASS(pIMDImport->GetMethodProps(token,
        NULL,
        funcName,
        STRING_LENGTH,
        0,
        0,
        NULL,
        NULL,
        NULL,
        NULL));
    pIMDImport->Release();

    return S_OK;
}

HRESULT CodeProfiler::ModuleLoadFinished(
    IPrfCom *pPrfCom,
    ModuleID moduleId ,
    HRESULT hrStatus )
{
    if (FAILED(hrStatus))
    {
        return S_OK;
    }
    if (NULL == moduleId)
    {
        FAILURE(L"ModuleLoadFinished: ModuleID cannot be NULL");
        return E_FAIL;
    }
    WCHAR moduleName[STRING_LENGTH];
    ULONG nameLength = 0;
    AssemblyID assemblyId;
    HRESULT hr = PINFO->GetModuleInfo(moduleId, NULL, STRING_LENGTH, &nameLength,
                                   moduleName, &assemblyId);
    if (hr != S_OK)
    {
        FAILURE(L"AssemblyLoadFinished: GetAssemblyInfo failed, returned " << HEX(hr));
    }
    return hr;
}

/*
*  Verification
*/
HRESULT CodeProfiler::Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"This stress profiler does not perform verification so it implicitly passed!")
        return S_OK;
}

/*
*  Verification routine called by TestProfiler
*/
HRESULT CodeProfiler_Verify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(CodeProfiler);
    HRESULT hr = pCodeProfiler->Verify(pPrfCom);

    // Clean up instance of test class
    delete pCodeProfiler;
    pPrfCom->m_pTestClassInstance = NULL;

    return hr;
}

/*
*  Initialization routine called by TestProfiler
*/

void CodeProfiler_Initialize(IPrfCom * pPrfCom,
                             PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize CodeProfiler.\n");

    // Create and save an instance of test class
    // pModuleMethodTable->TEST_POINTER is passed into every callback as pPrfCom->m_privatePtr
    SET_CLASS_POINTER(new CodeProfiler(pPrfCom));

    // Turn off callback counting to reduce synchronization/serialization in TestProfiler
    // It isn't needed since we aren't using the counts anyway.
    pPrfCom->m_fEnableCallbackCounting = FALSE;

    // Initialize MethodTable - we want to all the callbacks of a typical code profiler
    pModuleMethodTable->FLAGS =
        COR_PRF_MONITOR_CLASS_LOADS |
        COR_PRF_MONITOR_MODULE_LOADS | 
        COR_PRF_MONITOR_ASSEMBLY_LOADS | 
        COR_PRF_MONITOR_APPDOMAIN_LOADS | 
        COR_PRF_MONITOR_JIT_COMPILATION | 
        COR_PRF_MONITOR_CACHE_SEARCHES | 
        COR_PRF_MONITOR_THREADS | 
        COR_PRF_MONITOR_ENTERLEAVE | 
        COR_PRF_MONITOR_EXCEPTIONS | 
        COR_PRF_MONITOR_CODE_TRANSITIONS | 
        COR_PRF_MONITOR_SUSPENDS;

    REGISTER_CALLBACK(APPDOMAINCREATIONFINISHED, CodeProfiler::AppDomainCreationFinishedWrapper);
    REGISTER_CALLBACK(ASSEMBLYLOADFINISHED, CodeProfiler::AssemblyLoadFinishedWrapper);
    REGISTER_CALLBACK(CLASSLOADFINISHED, CodeProfiler::ClassLoadFinishedWrapper);
    REGISTER_CALLBACK(EXCEPTIONCATCHERENTER, CodeProfiler::ExceptionCatcherEnterWrapper);
    REGISTER_CALLBACK(EXCEPTIONSEARCHFILTERENTER, CodeProfiler::ExceptionSearchFilterEnterWrapper);
    REGISTER_CALLBACK(EXCEPTIONUNWINDFINALLYENTER, CodeProfiler::ExceptionUnwindFinallyEnterWrapper);
    REGISTER_CALLBACK(JITCACHEDFUNCTIONSEARCHFINISHED, CodeProfiler::JITCachedFunctionSearchFinishedWrapper);
    REGISTER_CALLBACK(JITCOMPILATIONSTARTED, CodeProfiler::JITCompilationStartedWrapper);
    REGISTER_CALLBACK(JITCOMPILATIONFINISHED, CodeProfiler::JITCompilationFinishedWrapper);
    REGISTER_CALLBACK(MODULELOADFINISHED, CodeProfiler::ModuleLoadFinishedWrapper);
    REGISTER_CALLBACK(VERIFY, CodeProfiler_Verify);
    return;
}

