#include "../../ProfilerCommon.h"
#ifdef WINDOWS
#include <process.h>
#endif

// global variable to track test failures.
static BOOL g_FailedTest = FALSE;


HRESULT EnumerateAllModules(IPrfCom * pPrfCom, ModuleID moduleId, bool bExpectedInEnumeration)
{
    HRESULT hr = S_OK;
    ULONG cObjFetched = 0;
    ObjectID * pObjects = NULL;
    ULONG cModules = 0;
    COMPtrHolder<ICorProfilerModuleEnum> pModuleEnum;
    PLATFORM_WCHAR wszModuleName[MAX_PATH] = {0};
    PROFILER_WCHAR wszModuleNameTemp[MAX_PATH] = {0};
    ULONG cchModuleName = 0;
    PLATFORM_WCHAR wszAssemblyName[MAX_PATH] = {0};
    PROFILER_WCHAR wszAssemblyNameTemp[MAX_PATH] = {0};
    ULONG cchAssemblyName = 0;
    PLATFORM_WCHAR wszAppdomainName[MAX_PATH] = {0};
    PROFILER_WCHAR wszAppdomainNameTemp[MAX_PATH] = {0};
    ULONG cchAppdomainName = 0;
	AssemblyID assemblyID;
	AppDomainID appdomainID;
	bool bFoundInEnumeration = false;
    
    
    // pre-condition checks
    if (NULL == pPrfCom)
    {
        FAILURE(L"Pre-condition check failed. pPrfCom is NULL");
        hr = E_INVALIDARG;
        goto ErrReturn;
    }
    if (moduleId == 0)
    {
        FAILURE(L"Pre-condition check failed. moduleId is NULL");
        hr = E_INVALIDARG;
        goto ErrReturn;
    }
    
        
    // now let us start enumerating the loaded modules ......
    hr = pPrfCom->m_pInfo->EnumModules( & pModuleEnum);
    if (S_OK != hr)
    {
        FAILURE(L"ICorProfilerInfo::EnumModules returned 0x" << std::hex << hr);
        goto ErrReturn;
    }

    // check the module count reported by the enumeration. 
    hr = pModuleEnum->GetCount(& cModules);
    if (S_OK != hr)
    {
        FAILURE(L"ICorProfilerModuleEnum::GetCount returned 0x" << std::hex << hr);
        goto ErrReturn;
    } 

    // Retrieve all the module info ..........
    pObjects = new ObjectID[cModules];
    if (NULL == pObjects)
    {
        FAILURE(L"Failed to allocate memory");
        hr = E_OUTOFMEMORY;
        goto ErrReturn;
    }
    
    hr = pModuleEnum->Next(cModules, pObjects, & cObjFetched);
    if (S_OK != hr)
    {
        FAILURE(L"ICorProfilerModuleEnum::Next returned 0x" << std::hex << hr);
        goto ErrReturn;
    }
    if (cObjFetched != cModules)
    {
        FAILURE(L"cObjFetched != * pcModules");
        hr = E_FAIL;
        goto ErrReturn;
    }

    // For each module in the enumeration .......
    for (unsigned int idx = 0; idx < cObjFetched; idx++)
    {
    
        // Step 1: Get its module info
        hr = pPrfCom->m_pInfo->GetModuleInfo(
                                (ModuleID) pObjects[idx],
                                NULL,
                                MAX_PATH,
                                & cchModuleName,
                                wszModuleNameTemp,
                                & assemblyID);
		ConvertProfilerWCharToPlatformWChar(wszModuleName, MAX_PATH, wszModuleNameTemp, MAX_PATH);
        if (S_OK != hr)
        {
            FAILURE(L"ICorProfilerInfo::GetModuleInfo returned 0x" << std::hex << hr);
            goto ErrReturn;
        }
        
        if (moduleId == (ModuleID) pObjects[idx])
        {
            bFoundInEnumeration = true;
        }

        // Step 2: Get its assembly info
		hr = pPrfCom->m_pInfo->GetAssemblyInfo(
								assemblyID,
								MAX_PATH,
								& cchAssemblyName,
								wszAssemblyNameTemp,
								& appdomainID,
								NULL);

		ConvertProfilerWCharToPlatformWChar(wszAssemblyName, MAX_PATH, wszAssemblyNameTemp, MAX_PATH);

        if (S_OK != hr)
        {
            FAILURE(L"ICorProfilerInfo::GetAssemblyInfo returned 0x" << std::hex << hr);
            goto ErrReturn;
        }

        // Step 3: Get its app-domain info
		hr = pPrfCom->m_pInfo->GetAppDomainInfo(
								appdomainID,
								MAX_PATH,
								& cchAppdomainName,
								wszAppdomainNameTemp,
								NULL);
		ConvertProfilerWCharToPlatformWChar(wszAppdomainName, MAX_PATH, wszAppdomainNameTemp, MAX_PATH);
        if (S_OK != hr)
        {
            FAILURE(L"ICorProfilerInfo::GetAppDomainInfo returned 0x" << std::hex << hr);
            goto ErrReturn;
        }
								

        DISPLAY(L"Module ID = " << (ModuleID)pObjects[idx]);
        DISPLAY(L"Module NAME = " << wszModuleName);
		DISPLAY(L"Assembly ID = " << assemblyID);
		DISPLAY(L"Assembly NAME = " << wszAssemblyName);
		DISPLAY(L"Appdomain ID = " << appdomainID);
		DISPLAY(L"Appdomain NAME = " << wszAppdomainName);
		DISPLAY(L"");
    }
    
    if (bFoundInEnumeration != bExpectedInEnumeration)
    {
      DISPLAY(L"The following module was ");
      
      if (!bExpectedInEnumeration)
        DISPLAY(L"not ");

      DISPLAY(L"expected in the enumeration : " << moduleId);
      
      hr = E_FAIL;
      goto ErrReturn;
    }
    
    DISPLAY(L"");

    
 ErrReturn:

    DISPLAY(L"");
 
    if (NULL != pObjects) {
        delete[] pObjects; 
        pObjects = NULL;
    }
        
    if (S_OK != hr)
        g_FailedTest = TRUE;

    return hr;
}


HRESULT Unit_EnumModules_ModuleLoadFinished(IPrfCom * pPrfCom, ModuleID moduleId, HRESULT hrStatus) 
{
    HRESULT hr = S_OK; 
        
    DISPLAY(L"Enumerating inside ModuleLoadFinished callback\n");

    if (S_OK == hrStatus)
    {
        hr = EnumerateAllModules(pPrfCom, moduleId, true);
        if (S_OK != hr)
        {
            FAILURE(L"EnumerateAllModules() returned 0x" << std::hex << hr);
            goto ErrReturn;
        }
    }
    
 ErrReturn:

    DISPLAY(L"");
    return hr; 
}


HRESULT Unit_EnumModules_ModuleUnloadFinished(IPrfCom * pPrfCom, ModuleID moduleId, HRESULT hrStatus) 
{
    HRESULT hr = S_OK; 
    
    DISPLAY(L"Enumerating inside ModuleUnloadFinished callback\n");

    if (S_OK == hrStatus)
    {
        hr = EnumerateAllModules(pPrfCom, moduleId, false);
        if (S_OK != hr)
        {
            FAILURE(L"EnumerateAllModules() returned 0x" << std::hex << hr);
            goto ErrReturn;
        }
    }
    
 ErrReturn:

    return hr; 
}


HRESULT Unit_EnumModules_ModuleLoadStarted(IPrfCom * pPrfCom, ModuleID moduleId) 
{
    DISPLAY(L"Enumerating inside ModuleLoadStarted callback\n");

    HRESULT hr = EnumerateAllModules(pPrfCom, moduleId, false);
    if (S_OK != hr)
    {
        FAILURE(L"EnumerateAllModules() returned 0x" << std::hex << hr);
        goto ErrReturn;
    }
    
 ErrReturn:

    DISPLAY(L"");
    return hr;   
}


HRESULT Unit_EnumModules_ModuleUnloadStarted(IPrfCom * pPrfCom, ModuleID moduleId) 
{
    DISPLAY(L"Enumerating inside ModuleUnloadStarted callback\n");

    HRESULT hr = EnumerateAllModules(pPrfCom, moduleId, false);
    if (S_OK != hr)
    {
        FAILURE(L"EnumerateAllModules() returned 0x" << std::hex << hr);
        goto ErrReturn;
    }
    
 ErrReturn:

    DISPLAY(L"");
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

HRESULT Unit_EnumModules_Verify(IPrfCom * pPrfCom)
{
    HRESULT hr = S_OK;

    if (TRUE == g_FailedTest)
    {
        FAILURE(L"A Test Failure has occured.");
        hr = E_FAIL;
        goto ErrReturn;
    }    

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

void Unit_EnumModules_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize UnitTests extension")

    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_MODULE_LOADS;
    pModuleMethodTable->VERIFY = (FC_VERIFY) &Unit_EnumModules_Verify;

    pModuleMethodTable->MODULELOADSTARTED = (FC_MODULELOADSTARTED) &Unit_EnumModules_ModuleLoadStarted;
    pModuleMethodTable->MODULELOADFINISHED = (FC_MODULELOADFINISHED) &Unit_EnumModules_ModuleLoadFinished;
    pModuleMethodTable->MODULEUNLOADSTARTED = (FC_MODULEUNLOADSTARTED) &Unit_EnumModules_ModuleUnloadStarted;
    pModuleMethodTable->MODULEUNLOADFINISHED = (FC_MODULEUNLOADFINISHED) &Unit_EnumModules_ModuleUnloadFinished;

    return;
}
