// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "../ProfilerCommon.h"

// Legacy support
#include "../LegacyCompat.h"
// Legacy support

bool IsMscorlib(const wchar_t *wszDLL);

INT gModules_Enums;
INT gModules_NumFrozenObjects;

HRESULT ShowModuleFlag(DWORD dwModuleFlags, IPrfCom * pPrfCom)
{
    if (dwModuleFlags&COR_PRF_MODULE_DISK) DISPLAY( L"  COR_PRF_MODULE_DISK ");
    if (dwModuleFlags&COR_PRF_MODULE_NGEN) DISPLAY( L"  COR_PRF_MODULE_NGEN ");
    if (dwModuleFlags&COR_PRF_MODULE_DYNAMIC) DISPLAY( L"   COR_PRF_MODULE_DYNAMIC ");
    if (dwModuleFlags&COR_PRF_MODULE_COLLECTIBLE) DISPLAY( L"   COR_PRF_MODULE_COLLECTIBLE ");
    if (dwModuleFlags&COR_PRF_MODULE_RESOURCE) DISPLAY( L"  COR_PRF_MODULE_RESOURCE ");
    if (dwModuleFlags&COR_PRF_MODULE_FLAT_LAYOUT) DISPLAY( L"   COR_PRF_MODULE_FLAT_LAYOUT ");
    return S_OK;
}


HRESULT Modules_ModuleLoadFinished(IPrfCom * pPrfCom,
                    ModuleID moduleId,
                    HRESULT /*hrStatus*/)
{
    HRESULT hr = S_OK;
    DWORD length;
    PLATFORM_WCHAR modName[STRING_LENGTH];
    PROFILER_WCHAR modNameTemp[STRING_LENGTH];
    LPCBYTE pBaseLoadAddress;

    MUST_PASS(PINFO->GetModuleInfo(moduleId,
                                    &pBaseLoadAddress,
                                    STRING_LENGTH,
                                    &length,
                                    modNameTemp,
                                    NULL));

	ConvertProfilerWCharToPlatformWChar(modName, STRING_LENGTH, modNameTemp, STRING_LENGTH);

	DISPLAY(L"GetModuleInfo id name baseAddress: 0x" << HEX(moduleId) << modName << " 0x" << pBaseLoadAddress);

	// TODO: I don't think these new variables are necessary
    DWORD length2;
    PLATFORM_WCHAR modName2[STRING_LENGTH];
    PROFILER_WCHAR modName2Temp[STRING_LENGTH];
    LPCBYTE pBaseLoadAddress2;
    DWORD dwModuleFlags = 0;
    MUST_PASS(PINFO->GetModuleInfo2(moduleId,
                                    &pBaseLoadAddress2,
                                    STRING_LENGTH,
                                    &length2,
                                    modName2Temp,
                                    NULL,
                                    &dwModuleFlags));
    
	ConvertProfilerWCharToPlatformWChar(modName2, STRING_LENGTH, modName2Temp, STRING_LENGTH);
	DISPLAY(L"GetModuleInfo2 id name baseAddress: 0x" << HEX(moduleId) << modName2 << L"0x" << HEX(pBaseLoadAddress2));
    ShowModuleFlag(dwModuleFlags, pPrfCom);

    assert(pBaseLoadAddress==pBaseLoadAddress2);
    assert(length==length2);
    assert(wcscmp(modName, modName2) == 0);
    assert(wcslen(modName)!=0);
    if (dwModuleFlags&COR_PRF_MODULE_DYNAMIC) assert(pBaseLoadAddress2==0);


    
    wstring moduleName(modName);


    COMPtrHolder<ICorProfilerObjectEnum>pICorProfilerObjectEnum;

	// Oct 15, 2007: Even though string freezing is disabled now
	// EnumModuleFrozenObjects should still return S_OK and methods
	// on ICorProfilerObjectEnum should return without a failing 
	// hresult.
    hr = pPrfCom->m_pInfo->EnumModuleFrozenObjects(moduleId,
                                             &pICorProfilerObjectEnum);

    if (hr == CORPROF_E_DATAINCOMPLETE)
    {
        DISPLAY( L"EnumModuleFrozenObjects returned CORPROF_E_DATAINCOMPLETE\n" );
        return S_OK;
    }
    if (hr != S_OK)	
    {
		FAILURE(L"EnumModuleFrozenObjects returned 0x" << HEX(hr) << "\n");
        return hr;
    }

    // Exercise the ICorProfilerObjectEnum functions
    if (gModules_Enums == 1)
    {
		if (IsMscorlib(moduleName.c_str()))
        {
            // We're only interested in mscorlib for this test.  Return if we are not at that module load.
            return S_OK;
        }

        ULONG frozenCount = 0;

		// Oct 15, 2007: Even though strings are not being frozen, this 
		// API should not return failing hresult values.
        hr = pICorProfilerObjectEnum->GetCount(&frozenCount);
        if (FAILED(hr))
        {
			FAILURE(L"pICorProfilerObjectEnum->GetCount returned 0x" << HEX(hr) << "\n");
            return hr;
        }

		DISPLAY(L"" << frozenCount << " Frozen Objects\n");
        gModules_NumFrozenObjects += frozenCount;

        ObjectID objInfo[1];
        ULONG fetched = 0;

		// With no string freezing this call should return S_FALSE hresult value and fetched == 0
        hr = pICorProfilerObjectEnum->Next(1,
                                          objInfo,
                                          &fetched);
		if (hr != S_FALSE)
		{
			FAILURE(L"pICorProfilerObjectEnum->Next returned 0x" << HEX(hr) << "\n");
            return hr;
		}
		if (fetched != 0)
		{
			FAILURE(L"pICorProfilerObjectEnum->Next fetched " << fetched << " elements\n");
            return hr;
		}
		if (FAILED(hr))
        {
			FAILURE(L"pICorProfilerObjectEnum->Next returned 0x" << HEX(hr) << "\n");
            return hr;
        }
		

        hr = pICorProfilerObjectEnum->Reset();
		if (hr != S_OK || FAILED(hr))
		{
			FAILURE(L"pICorProfilerObjectEnum->Reset returned 0x" << HEX(hr) << "\n");
            return hr;
        }

		// With no string freezing, and therefore no frozen objects to skip over, this call 
		// should return S_FALSE. 
        hr = pICorProfilerObjectEnum->Skip(5);
		if (hr != S_FALSE)
		{
			FAILURE(L"pICorProfilerObjectEnum->Skip returned 0x" << HEX(hr) << "\n");
            return hr;
		}
		if (FAILED(hr))
        {
			FAILURE(L"pICorProfilerObjectEnum->Skip returned 0x" << HEX(hr) << "\n");
            return hr;
        }

        ICorProfilerObjectEnum *pICorProfilerObjectEnumClone;
        hr = pICorProfilerObjectEnum->Clone(&pICorProfilerObjectEnumClone);		
        if (hr != S_OK || FAILED(hr))
        {
			FAILURE(L"pICorProfilerObjectEnum->Clone returned 0x" << HEX(hr) << "\n");
            return hr;
        }		
    }

    return hr;

}


HRESULT Modules_Verify(IPrfCom * pPrfCom)
{
    DISPLAY( L"Verify ModuleLoadFinished callbacks\n" );
	// Oct 15, 2007: There should be no more frozen objects in modules (after the removal of 
	// hardbinding between NGen images, String Freezing has been disabled
	// and frozen objects will not be created)
    if ((gModules_Enums == 1) && (gModules_NumFrozenObjects > 0))
    {
        FAILURE( L"Frozen objects found!" );
        return E_FAIL;
    }
    return S_OK;
}


void Modules_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, BOOL testModuleEnums)
{
    DISPLAY( L"Initialize Modules module\n" ) ;

    gModules_Enums = 0;
    gModules_NumFrozenObjects = 0;

    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_MODULE_LOADS;

    pModuleMethodTable->VERIFY = (FC_VERIFY) &Modules_Verify;
    pModuleMethodTable->MODULELOADFINISHED = (FC_MODULELOADFINISHED) &Modules_ModuleLoadFinished;

    if (testModuleEnums == TRUE)
            gModules_Enums = 1;

    return;
}





