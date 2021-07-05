#include "../../ProfilerCommon.h"
#include "../../LegacyCompat.h" // Whidbey Framework Support

#include "../../../holder.h"


HRESULT testHR;

HRESULT VSW104809_ModuleLoadFinished(IPrfCom * pPrfCom, ModuleID moduleId, HRESULT hrStatus)
{
    HRESULT hr = S_OK;

    ULONG cbName = NULL;
    PLATFORM_WCHAR moduleName[LONG_LENGTH];
    PROFILER_WCHAR moduleNameTemp[LONG_LENGTH];

    // Added regression test for VSWhidbey 104809 -GetModuleInfo incorrectly adjusts the buffer size
    hr = PINFO->GetModuleInfo(moduleId, 0, 1, &cbName, moduleNameTemp, 0);
	ConvertProfilerWCharToPlatformWChar(moduleName, LONG_LENGTH, moduleNameTemp, LONG_LENGTH);
    if (hr !=  HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) //0x8007007a)
    {
        FAILURE( L"GetModuleInfo did not return a failure with a small buffer.  VSWhidbey 496412\n" );
    }

    hr = pPrfCom->m_pInfo->GetModuleInfo(moduleId, 0, LONG_LENGTH, &cbName, moduleNameTemp, 0);

    ConvertProfilerWCharToPlatformWChar(moduleName, LONG_LENGTH, moduleNameTemp, LONG_LENGTH);

    // Convert to lowercase so string comparison will succed on 9x.
    ConvertWStringToLower(moduleName, LONG_LENGTH);

    if (wcsstr(moduleName, L"factorial.exe") != 0 || wcsstr(moduleName, L"factorial.ni.exe") != 0)
    {
		DISPLAY(L"Injection code into  module: " << moduleName);

        COMPtrHolder<IMethodMalloc> pMethodMalloc;
        hr = pPrfCom->m_pInfo->GetILFunctionBodyAllocator(moduleId, &pMethodMalloc);

        unsigned __int8* pMethod = (unsigned __int8*)pMethodMalloc->Alloc( 3 );
        *(pMethod + 0) = 0x0A;      // 000010 (2 bytes) | 10 (tiny format)
        *(pMethod + 1) = 0x00;      // nop
        *(pMethod + 2) = 0x2A;      // ret

        hr = pPrfCom->m_pInfo->SetILFunctionBody(moduleId, 0x06000001, pMethod);

        if (SUCCEEDED(hr))
        {
            DISPLAY(L"SetILFunctionBody Succeeded!\n");
            testHR = S_OK;
        }
        else
        {
            DISPLAY(L"SetILFunctionBody Failed!\n");
        }
    }
    else
    {
        DISPLAY(L"skipping module\n");
    }

    return hr;
}

HRESULT VSW104809_Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"Verify VSW104809 callbacks\n" );
    return testHR;

}

void VSW_104809_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize VSW104809 extension\n");

    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_MODULE_LOADS | COR_PRF_MONITOR_APPDOMAIN_LOADS;
    pModuleMethodTable->VERIFY = (FC_VERIFY) &VSW104809_Verify;
    pModuleMethodTable->MODULELOADFINISHED = (FC_MODULELOADFINISHED) &VSW104809_ModuleLoadFinished;

    testHR = E_FAIL;

    return;
}
