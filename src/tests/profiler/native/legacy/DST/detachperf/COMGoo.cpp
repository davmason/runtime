// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include <objbase.h>
#include "Common.hxx"
#include "COMGoo.h"
#include "DetachPerfTester.h"


///////////////////////////////////////////////////////////////////////////////

// we'll set the CLSID of the profiler component to {C84E218544C640D9B88552FF521FEC03}    
const GUID CCOMGoo::CLSID_DETACHPERFTESTER = 
    { 0xc84e2185, 0x44c6, 0x40d9, { 0xb8, 0x85, 0x52, 0xff, 0x52, 0x1f, 0xec, 0x3 } };

// static members must be initialized at file-scope
long    CCOMGoo::m_cComponents  = 0;
long    CCOMGoo::m_cServerLocks = 0;
HMODULE CCOMGoo::m_hModule      = NULL; 
const LPCWSTR CCOMGoo::m_pcwszFriendlyName = L"DetachPerfTester - profiler detach testing";
const LPCWSTR CCOMGoo::m_pcwszVerIndProgID = L"DetachPerfTester.detachperftest";
const LPCWSTR CCOMGoo::m_pcwszProgID       = L"DetachPerfTester.detachperftest.1";

///////////////////////////////////////////////////////////////////////////////


//*********************************************************************
//  Descripton: This function will register the COM component in the 
//              Registry. The component calls this function from its 
//              DllRegisterServer function.
//  
//  Params:     hModule - Handle to Module implementing the COM
//                  component.                      
//              clsid - The Class ID of the component.
//              pcwszFriendlyName - Self explanatory.
//              pcwszVerIndProgID - Version independent Prof ID.
//              pcwszProgID - Self explanatory.
//
//  Return:     S_OK if method successful, else appropriate error code.
//  
//  Notes:      n/a
//*********************************************************************

HRESULT CCOMGoo::RegisterServer(HMODULE hModule, const CLSID & clsid,         
    LPCWSTR pcwszFriendlyName, LPCWSTR pcwszVerIndProgID, LPCWSTR pcwszProgID)       
{
    HRESULT hr = S_OK;
    DWORD dwRet = 0;
    HKEY hKey = NULL;
    wchar_t wszModule[512] = {0};
    wchar_t wszKey[128] = {0};
    wchar_t * wszClsid = NULL;

    // Get server location.
    dwRet = GetModuleFileName(
                hModule, 
                (LPTSTR) wszModule,
                sizeof(wszModule)/sizeof(wszModule[0]));

    if (ERROR_INSUFFICIENT_BUFFER == GetLastError()) {
        hr = ERROR_INSUFFICIENT_BUFFER;
        goto ErrReturn;
    }
    if (0 == dwRet) {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto ErrReturn;
    }
    
    // Convert the CLSID into string.
    hr = StringFromCLSID(clsid, (LPOLESTR*)& wszClsid);
    HR_CHECK(hr, S_OK, L"ole32!StringFromCLSID");

    // Step 1: Populate the CLSID fields in the registry

    // Build the key CLSID\\{...}
    wcscpy_s(wszKey, sizeof(wszKey)/sizeof(wszKey[0]), L"CLSID\\");
    wcscat_s(wszKey, sizeof(wszKey)/sizeof(wszKey[0]), wszClsid);

    // create the key if does not already exist
    hr = SetKeyAndValue(wszKey, NULL, pcwszFriendlyName);
    HR_CHECK(hr, S_OK, L"CCOMGoo::SetKeyAndValue");

    // Add the server filename subkey under the CLSID key.
    hr = SetKeyAndValue(wszKey, L"InprocServer32", wszModule);
    HR_CHECK(hr, S_OK, L"CCOMGoo::SetKeyAndValue");

    // Add the ProgID subkey under the CLSID key.
    hr = SetKeyAndValue(wszKey, L"ProgID", pcwszProgID);
    HR_CHECK(hr, S_OK, L"CCOMGoo::SetKeyAndValue");

    // Add the version-independent ProgID subkey under CLSID key.
    hr = SetKeyAndValue(wszKey, L"VersionIndependentProgID", pcwszVerIndProgID);
    HR_CHECK(hr, S_OK, L"CCOMGoo::SetKeyAndValue");

    // Step 2: Add the version-independent ProgID subkey under HKEY_CLASSES_ROOT.

    hr = SetKeyAndValue(pcwszVerIndProgID, NULL, pcwszFriendlyName); 
    HR_CHECK(hr, S_OK, L"CCOMGoo::SetKeyAndValue");

    hr = SetKeyAndValue(pcwszVerIndProgID, L"CLSID", wszClsid);
    HR_CHECK(hr, S_OK, L"CCOMGoo::SetKeyAndValue");

    hr = SetKeyAndValue(pcwszVerIndProgID, L"CurVer", pcwszProgID);
    HR_CHECK(hr, S_OK, L"CCOMGoo::SetKeyAndValue");

    // Step 3 : Add the versioned ProgID subkey under HKEY_CLASSES_ROOT.

    hr = SetKeyAndValue(pcwszProgID, NULL, pcwszFriendlyName);
    HR_CHECK(hr, S_OK, L"CCOMGoo::SetKeyAndValue");
 
    hr = SetKeyAndValue(pcwszProgID, L"CLSID", wszClsid);
    HR_CHECK(hr, S_OK, L"CCOMGoo::SetKeyAndValue");

ErrReturn:

    // cleanup
    if (NULL != wszClsid)
        CoTaskMemFree(wszClsid);
    
    if (S_OK != hr)
        UnregisterServer(clsid, pcwszVerIndProgID, pcwszProgID);

    return hr;
}


//*********************************************************************
//  Descripton: This function will unregister a COM component. The 
//              component calls this function from its 
//              DllUnregisterServer function.
//  
//  Params:     clsid - The Class ID of the component.
//              pcwszVerIndProgID - Version independent Prof ID.
//              pcwszProgID - Self explanatory.
//
//  Return:     S_OK if method successful, else appropriate error code.
//  
//  Notes:      n/a
//*********************************************************************

HRESULT CCOMGoo::UnregisterServer(const CLSID & clsid, 
    LPCWSTR pcwszVerIndProgID, LPCWSTR pcwszProgID)
{
    DWORD dwRet = 0;
    HRESULT hr = S_OK;
    wchar_t wszKey[128] = {0};
    wchar_t * wszClsid = NULL;

    // Convert the CLSID into string.
    hr = StringFromCLSID(clsid, (LPOLESTR*)& wszClsid);
    HR_CHECK(hr, S_OK, L"ole32!StringFromCLSID");

    // Step 1: Remove the CLSID\{GUID} key in the registry

    // Build the key CLSID\\{...}
    wcscpy_s(wszKey, sizeof(wszKey)/sizeof(wszKey[0]), L"CLSID\\");
    wcscat_s(wszKey, sizeof(wszKey)/sizeof(wszKey[0]), wszClsid);
    
    // remove the component's CLSID key and its subkeys
    hr = RecursiveDeleteKey(HKEY_CLASSES_ROOT, wszKey);
    HR_CHECK(hr, S_OK, L"CCOMGoo::RecursiveDeleteKey");

    // Step 2: Delete the version-independent ProgID Key.
    hr = RecursiveDeleteKey(HKEY_CLASSES_ROOT, pcwszVerIndProgID);
    HR_CHECK(hr, S_OK, L"CCOMGoo::RecursiveDeleteKey");

    // Step 3: Delete the ProgID key.
    hr = RecursiveDeleteKey(HKEY_CLASSES_ROOT, pcwszProgID);
    HR_CHECK(hr, S_OK, L"CCOMGoo::RecursiveDeleteKey");


ErrReturn:

    // cleanup
    if (NULL != wszClsid)
        CoTaskMemFree(wszClsid);

    return hr;
}


HRESULT CCOMGoo::SetKeyAndValue(LPCWSTR szKey, LPCWSTR pcwszSubkey, LPCWSTR pcwszValue)
{
    DWORD dwRet = 0; 
    HKEY hKey = NULL;
    HRESULT hr = S_OK;
    wchar_t wszKeyBuf[1024] = {0};

    // Copy keyname into buffer.
    wcscpy_s(wszKeyBuf, sizeof(wszKeyBuf)/sizeof(wszKeyBuf[0]), szKey) ;

    // Add subkey name to buffer.
    if (NULL != wszKeyBuf && NULL != pcwszSubkey) {
        wcscat_s(wszKeyBuf, sizeof(wszKeyBuf)/sizeof(wszKeyBuf[0]), L"\\");
        wcscat_s(wszKeyBuf, sizeof(wszKeyBuf)/sizeof(wszKeyBuf[0]), pcwszSubkey);
    }

    // Create and open key and subkey.
    dwRet = RegCreateKeyEx(
                HKEY_CLASSES_ROOT,
                (LPCWSTR) wszKeyBuf, 
                0,
                NULL,
                0,
                KEY_WRITE,
                NULL, 
                & hKey,
                NULL);

    if (ERROR_SUCCESS != dwRet) {
        hr = HRESULT_FROM_WIN32(dwRet);
        goto ErrReturn;
    }

    // Set the Value.
    if (NULL != pcwszValue) {

        dwRet = RegSetValueEx(
                    hKey, 
                    NULL, 
                    0, 
                    REG_SZ, 
                    (BYTE*) pcwszValue, 
                    (DWORD) (sizeof(wchar_t) * (wcslen(pcwszValue)+1))) ;

        if (ERROR_SUCCESS != dwRet) {
            hr = HRESULT_FROM_WIN32(dwRet);
        }
    }


ErrReturn:
    
    if (NULL != hKey) {
        RegCloseKey(hKey);
    }

    return hr;
}


// <TODO> Please ensure hKeyParent is set to HKEY_CLASSES_ROOT </TODO>
HRESULT CCOMGoo::RecursiveDeleteKey(HKEY hKeyParent, LPCWSTR pcwszKeyChild)
{
    DWORD dwRet = 0;
    HRESULT hr = S_OK;
    HKEY hKeyChild = NULL;
    wchar_t wszBuffer[256] = {0};
    
    // Open the child.
    dwRet = RegOpenKeyEx(
                hKeyParent, 
                (LPWSTR) pcwszKeyChild,
                0,
                KEY_ALL_ACCESS, 
                & hKeyChild);

    if (ERROR_SUCCESS != dwRet) {
        hr = HRESULT_FROM_WIN32(dwRet);
        goto ErrReturn;
    }

    // Enumerate all of the decendents of this child.
    DWORD dwSize = sizeof(wszBuffer)/sizeof(wszBuffer[0]);
    while (ERROR_SUCCESS == RegEnumKeyEx(
                                hKeyChild, 
                                0,
                                (LPWSTR) wszBuffer, 
                                & dwSize, 
                                NULL,
                                NULL, 
                                NULL, 
                                NULL))
    {
        // Delete the decendents of this child.
        hr = RecursiveDeleteKey(hKeyChild, (LPWSTR)wszBuffer);

        if (S_OK != hr) {
            RegCloseKey(hKeyChild); // Cleanup before exiting
            goto ErrReturn;
        }

        dwSize = sizeof(wszBuffer)/sizeof(wszBuffer[0]);
    }

    // Close the child.
    if (NULL != hKeyChild)
        RegCloseKey(hKeyChild);

    // Delete this child.
    RegDeleteKeyEx(hKeyParent, (LPCWSTR)pcwszKeyChild, 0, 0);

ErrReturn:
    return hr;    
}


///////////////////////////////////////////////////////////////////////////////


HRESULT __stdcall CFactory::QueryInterface(const IID& iid, void** ppv)
{    
    if ((iid == IID_IUnknown) || (iid == IID_IClassFactory)) {
        *ppv = static_cast<IClassFactory*>(this); 
    }
    else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}


ULONG __stdcall CFactory::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}


ULONG __stdcall CFactory::Release() 
{
    if (0 == InterlockedDecrement(&m_cRef)) {
        delete this;
        return 0;
    }
    return m_cRef;
}


HRESULT __stdcall CFactory::CreateInstance(IUnknown* pUnknownOuter,
    const IID& iid, void** ppv) 
{
    // Cannot aggregate.
    if (NULL != pUnknownOuter) {
        return CLASS_E_NOAGGREGATION;
    }

    // Create component.
    DetachPerfTester * pDetachPerfTester = new DetachPerfTester;
    if (NULL == pDetachPerfTester) {
        return E_OUTOFMEMORY;
    }

    // Get the requested interface.
    HRESULT hr = pDetachPerfTester->QueryInterface(iid, ppv);

    // Release the IUnknown pointer (If the QI failed, component will delete itself.)
    pDetachPerfTester->Release();
    return hr;
}


HRESULT __stdcall CFactory::LockServer(BOOL bLock) 
{
    if (bLock) {
        InterlockedIncrement(& CCOMGoo::m_cServerLocks); 
    }
    else {
        InterlockedDecrement(& CCOMGoo::m_cServerLocks);
    }
    return S_OK;
}


///////////////////////////////////////////////////////////////////////////////


STDAPI DllCanUnloadNow()
{
    if ((0 == CCOMGoo::m_cComponents) && (0 == CCOMGoo::m_cServerLocks)) {
        return S_OK;
    }
    else {
        return S_FALSE;
    }
}


STDAPI DllGetClassObject(const CLSID& clsid, const IID& iid, void ** ppv)
{
    // Can we create this component?
    if (clsid != CCOMGoo::CLSID_DETACHPERFTESTER) {
        return CLASS_E_CLASSNOTAVAILABLE;
    }

    // Create class factory.
    CFactory * pFactory = new CFactory();  // No AddRef in constructor
    if (NULL == pFactory) {
        return E_OUTOFMEMORY;
    }

    // Get requested interface.
    HRESULT hr = pFactory->QueryInterface(iid, ppv);
    pFactory->Release();

    return hr ;
}


STDAPI DllRegisterServer()
{
    return CCOMGoo::RegisterServer(
                CCOMGoo::m_hModule, 
                CCOMGoo::CLSID_DETACHPERFTESTER, 
                CCOMGoo::m_pcwszFriendlyName, 
                CCOMGoo::m_pcwszVerIndProgID, 
                CCOMGoo::m_pcwszProgID);
}


STDAPI DllUnregisterServer()
{
    return CCOMGoo::UnregisterServer(
                CCOMGoo::CLSID_DETACHPERFTESTER, 
                CCOMGoo::m_pcwszVerIndProgID, 
                CCOMGoo::m_pcwszProgID);
}


BOOL __stdcall DllMain(HINSTANCE hinstDLL, DWORD dwReason, void * lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        CCOMGoo::m_hModule = (HMODULE)hinstDLL ;
    }
    return TRUE ;
}

