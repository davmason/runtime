// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "../ProfilerCommon.h"

// Legacy support
#include "../LegacyCompat.h"
// Legacy support

#include <vector>
using namespace std;

ULONG gModules_NumEmptyName;
ULONG gDynamicModules_NumWrongName;

vector<ModuleID> modulePool;
#define DEFAULT_DYNAMIC_MODULES L"Default Dynamic Module"
#define MY_DYNAMIC_MODULES L"MyRefEmitModule"


HRESULT DynamicModules_ModuleLoadFinished(IPrfCom * pPrfCom,
                    ModuleID moduleId,
                    HRESULT /*hrStatus*/)
{
    HRESULT hr = S_OK;
    DWORD length;
    PROFILER_WCHAR modNameProfiler[STRING_LENGTH];
    PLATFORM_WCHAR modNamePlatform[STRING_LENGTH];

    
    for(ULONG  i=0;i<modulePool.size();++i)
    {
        ModuleID mId = modulePool.at(i);

        MUST_PASS(PINFO->GetModuleInfo(mId,
                                       NULL,
                                       STRING_LENGTH,
                                       &length,
                                       modNameProfiler,
                                       NULL));
		ConvertProfilerWCharToPlatformWChar(modNamePlatform, STRING_LENGTH, modNameProfiler, STRING_LENGTH);
		DISPLAY(L"" << i << L" : moduleId " << HEX(mId) << L", module name " << modNamePlatform << L"\n");
        if (wcslen(modNamePlatform)==0) gModules_NumEmptyName++;
        if (wcsncmp(modNamePlatform, MY_DYNAMIC_MODULES, wcslen(MY_DYNAMIC_MODULES))!=0)gDynamicModules_NumWrongName++;
    }


    MUST_PASS(PINFO->GetModuleInfo(moduleId,
                                    NULL,
                                    STRING_LENGTH,
                                    &length,
                                    modNameProfiler,
                                    NULL));
    
	ConvertProfilerWCharToPlatformWChar(modNamePlatform, STRING_LENGTH, modNameProfiler, STRING_LENGTH);
	DISPLAY(L"moduleId " << HEX(moduleId) << L" is loaded, module name " << modNamePlatform << L"\n");

    if (wcslen(modNamePlatform)==0) gModules_NumEmptyName++;
    //return S_OK if it is not a dynamic module
    if (wcsncmp(modNamePlatform, DEFAULT_DYNAMIC_MODULES, wcslen(DEFAULT_DYNAMIC_MODULES))!=0) 
        return S_OK;

    modulePool.push_back(moduleId);
    return S_OK;
}


HRESULT DynamicModules_Verify(IPrfCom * pPrfCom)
{
	DISPLAY(L"Verify ModuleLoadFinished callbacks\n");
	DWORD length;
    PROFILER_WCHAR modNameProfiler[STRING_LENGTH];
    PLATFORM_WCHAR modNamePlatform[STRING_LENGTH];
    for(ULONG  i=0;i<modulePool.size();++i)
    {
        ModuleID mId = modulePool.at(i);

        MUST_PASS(PINFO->GetModuleInfo(mId,
                                       NULL,
                                       STRING_LENGTH,
                                       &length,
                                       modNameProfiler,
                                       NULL));
		ConvertProfilerWCharToPlatformWChar(modNamePlatform, STRING_LENGTH, modNameProfiler, STRING_LENGTH);
		DISPLAY(L"" << i << L" : moduleId " << HEX(mId) << L", module name " << modNamePlatform << L"\n");
        if (wcslen(modNamePlatform)==0) 
			gModules_NumEmptyName++;
        if (wcsncmp(modNamePlatform, MY_DYNAMIC_MODULES, wcslen(MY_DYNAMIC_MODULES)) != 0)
			gDynamicModules_NumWrongName++;
    }


    if (gModules_NumEmptyName > 0)
    {
        FAILURE( L"Empty module name returned from GetModuleInfo!" );
        return E_FAIL;
    }

    if (gDynamicModules_NumWrongName > 0)
    {
        FAILURE( L"Wrong Dynamic module name returned from GetModuleInfo!" );
        return E_FAIL;
    }


    return S_OK;
}


void DynamicModules_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY( L"Initialize DynamicModules module\n" );

    gModules_NumEmptyName = 0;
    gDynamicModules_NumWrongName = 0;
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_MODULE_LOADS;
    pModuleMethodTable->VERIFY = (FC_VERIFY) &DynamicModules_Verify;
    pModuleMethodTable->MODULELOADFINISHED = (FC_MODULELOADFINISHED) &DynamicModules_ModuleLoadFinished;

    
    return;
}





