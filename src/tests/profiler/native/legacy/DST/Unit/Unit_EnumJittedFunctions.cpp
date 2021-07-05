#include "ProfilerCommon.h"
#ifdef WINDOWS
#include <process.h>
#endif

// global variable to track test failures.
static BOOL g_FailedTest = FALSE;
static int g_ulFunctionIdEnumerated = 0;

HRESULT EnumerateAllJittedMethods(IPrfCom * pPrfCom, FunctionID functionId, bool bExpectedInEnumeration)
{
    HRESULT hr = S_OK;
    ICorProfilerFunctionEnum* pEnum = NULL;
    ULONG cJittedMethods = 0;
    ULONG cJittedMethodsFetched = 0;
    COR_PRF_FUNCTION * pIds  = NULL;
    ClassID classId;
    ModuleID moduleId;
    mdToken token; 
    COMPtrHolder<IMetaDataImport> pIMDImport;
    WCHAR wszMethodName[MAX_PATH] = {0};
    ULONG cchMethodName = 0;
    bool bFoundInEnumeration = false;


    // pre-condition checks
    if (NULL == pPrfCom)
    {
        FAILURE(L"Pre-condition check failed. pPrfCom is NULL");
        hr = E_INVALIDARG;
        goto ErrReturn;
    }
    if (0 == functionId)
    {
        FAILURE(L"Pre-condition check failed. functionId is NULL");
        hr = E_INVALIDARG;
        goto ErrReturn;
    }
    
        
    // now let us start enumerating the jitted methods ......
    hr = pPrfCom->m_pInfo->EnumJITedFunctions( & pEnum);
    if (S_OK != hr)
    {
        FAILURE(L"ICorProfilerInfo::EnumJittedFunctions returned 0x" << std::hex << hr);
        goto ErrReturn;
    }

    // check the count reported by the enumeration. 
    hr = pEnum->GetCount(& cJittedMethods);
    if (S_OK != hr)
    {
        FAILURE(L"ICorProfilerFunctionEnum::GetCount returned 0x" << std::hex << hr);
        goto ErrReturn;
    }

    // Retrieve all the jitted method info ..........
    pIds = new COR_PRF_FUNCTION[cJittedMethods];
    if (NULL == pIds)
    {
        FAILURE(L"Failed to allocate memory");
        hr = E_OUTOFMEMORY;
        goto ErrReturn;
    }
    
    hr = pEnum->Next(cJittedMethods, pIds, & cJittedMethodsFetched);
    if (S_OK != hr)
    {
        FAILURE(L"ICorProfilerFunctionEnum::Next returned 0x" << std::hex << hr);
        goto ErrReturn;
    }
    if (cJittedMethodsFetched != cJittedMethods)
    {
        FAILURE(L"cJittedMethodsFetched != cJittedMethods");
        hr = E_FAIL;
        goto ErrReturn;
    }

    // For each jitted method in the enumeration .......
    for (unsigned int idx = 0; idx < cJittedMethodsFetched; idx++)
    {
        // Step 1: Get the function info
        hr = pPrfCom->m_pInfo->GetFunctionInfo(
                                (FunctionID) pIds[idx].functionId,
                                & classId,
                                & moduleId,
                                & token);
        if (S_OK != hr)
        {
            FAILURE(L"ICorProfilerInfo::GetFunctionInfo returned 0x" << std::hex << hr);
            goto ErrReturn;
        }
        
        if (functionId == (FunctionID) pIds[idx].functionId)
        {
            bFoundInEnumeration = true;
        }

        // Step 2: Get the associated module's metadata interface
        hr = pPrfCom->m_pInfo->GetModuleMetaData(
								moduleId,
								ofRead,
								IID_IMetaDataImport,
								(IUnknown **)&pIMDImport);

        if (S_OK != hr)
        {
            FAILURE(L"ICorProfilerInfo::GetModuleMetaData returned 0x" << std::hex << hr);
            goto ErrReturn;
        }

        // Step 3: Get method properties
        hr = pIMDImport->GetMethodProps(
								token,
								NULL,
								wszMethodName,
								MAX_PATH,
								& cchMethodName,
								NULL,
								NULL,
								NULL,
								NULL,
								NULL);
        if (S_OK != hr)
        {
            FAILURE(L"ICorProfilerInfo::GetAppDomainInfo returned 0x" << std::hex << hr);
            goto ErrReturn;
        }
								
#if 0
        DISPLAY(L"Function ID = " << (FunctionID) pIds[idx].functionId);
        DISPLAY(L"Function NAME = " << wszMethodName);
        DISPLAY(L"");
#endif
        ++g_ulFunctionIdEnumerated;
    }

    
    if (bFoundInEnumeration != bExpectedInEnumeration)
    {
      DISPLAY(L"The following jitted function was ");
      
      if (!bExpectedInEnumeration)
        DISPLAY(L"not ");

      DISPLAY(L"expected in the enumeration : " << functionId);
      
      hr = E_FAIL;
      goto ErrReturn;

    }
    
 ErrReturn:

    if (NULL != pIds) {
        delete[] pIds; 
        pIds = NULL;
    }
        
    if (NULL != pEnum) {
        pEnum->Release();
        pEnum = NULL;
    }
        
    
    if (S_OK != hr)
        g_FailedTest = TRUE;

    return hr;
}


HRESULT Unit_EnumJittedFunctions_JitCompilationFinished(IPrfCom * pPrfCom, FunctionID functionId, HRESULT hrStatus, BOOL fIsSafeToBlock) 
{
    HRESULT hr = S_OK; 

#if 0
    DISPLAY(L"Enumerating inside JitCompilationFinished callback\n");
#endif

    if (TRUE == fIsSafeToBlock)
    {
        hr = EnumerateAllJittedMethods(pPrfCom, functionId, true);
        if (S_OK != hr)
        {
            FAILURE(L"EnumerateAllJittedMethods() returned 0x" << std::hex << hr);
            goto ErrReturn;
        }
    }
    
 ErrReturn:

    return hr; 
}


HRESULT Unit_EnumJittedFunctions_JitCompilationStarted(IPrfCom * pPrfCom, FunctionID functionId, BOOL fIsSafeToBlock) 
{
    HRESULT hr = S_OK; 
#if 0    
    DISPLAY(L"Enumerating inside JitCompilationStarted callback\n");
#endif
    
    if (TRUE == fIsSafeToBlock)
    {
        hr = EnumerateAllJittedMethods(pPrfCom, functionId, false);
        if (S_OK != hr)
        {
            FAILURE(L"EnumerateAllJittedMethods() returned 0x" << std::hex << hr);
            goto ErrReturn;
        }
    }
    
 ErrReturn:

    return hr; 
}


//*********************************************************************
//  Descripton: Overall verification to ensure ???
//  
//  Params:     IPrfCom * pPrfCom   - 
//                  pointer to test module containing helper functions
//
//  Return:     S_OK if method successful, else E_FAIL
//  
//  Notes:      n/a
//*********************************************************************

HRESULT Unit_EnumJittedFunctions_Verify(IPrfCom * pPrfCom)
{
    HRESULT hr = S_OK;

    if (TRUE == g_FailedTest)
    {
        FAILURE(L"A Test Failure has occured.");
        hr = E_FAIL;
        goto ErrReturn;
    }    

    DISPLAY(g_ulFunctionIdEnumerated << L" Functions enumerated.");
ErrReturn:
    return hr;
}


//*********************************************************************
//  Descripton: In this initialization step, we subscribe to profiler 
//              callbacks and also specify the necessary event mask. 
//  
//  Params:     IPrfCom * pPrfCom   - 
//                  pointer to test module containing helper functions
//              PMODULEMETHODTABLE pModuleMethodTable - 
//                  pointer to the method table which enables you
//                  to subscribe to profiler callbacks.
//
//  Return:     n/a
//  
//  Notes:      n/a
//*********************************************************************

void Unit_EnumJittedFunctions_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize UnitTests extension")

    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_JIT_COMPILATION;
    pModuleMethodTable->VERIFY = (FC_VERIFY) &Unit_EnumJittedFunctions_Verify;

    pModuleMethodTable->JITCOMPILATIONSTARTED = (FC_JITCOMPILATIONSTARTED) &Unit_EnumJittedFunctions_JitCompilationStarted;
    pModuleMethodTable->JITCOMPILATIONFINISHED = (FC_JITCOMPILATIONFINISHED) &Unit_EnumJittedFunctions_JitCompilationFinished;

    return;
}
