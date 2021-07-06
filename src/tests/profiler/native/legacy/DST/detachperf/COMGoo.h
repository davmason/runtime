// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#pragma once

class CCOMGoo
{
 public:

    // Count of active components
    static long m_cComponents;

    // Count of locks
    static long m_cServerLocks;

    // Friendly name of component
    static const LPCWSTR m_pcwszFriendlyName;

    // Version-independent ProgID
    static const LPCWSTR m_pcwszVerIndProgID;

    // ProgID
    static const LPCWSTR m_pcwszProgID;

    // CLSID
    static const GUID CLSID_DETACHPERFTESTER;

    // DLL module handle
    static HMODULE m_hModule;

    // Helper function to register the component
    static HRESULT RegisterServer(
        HMODULE hModule,        // DLL module handle
        const CLSID & clsid,    // Class ID
        LPCWSTR szFriendlyName, // Friendly Name
        LPCWSTR szVerIndProgID, // Version indep ProgID
        LPCWSTR szProgID        // ProgID
        );

    // Helper function to unregister the component
    static HRESULT UnregisterServer(
        const CLSID & clsid,    // Class ID
        LPCWSTR szVerIndProgID, // Version indep ProgID
        LPCWSTR szProgID        // ProgID
        );


private: 

    static HRESULT SetKeyAndValue(
        LPCWSTR szKey, 
        LPCWSTR pcwszSubkey, 
        LPCWSTR pcwszValue
        );

    static HRESULT RecursiveDeleteKey(
        HKEY hKeyParent,
        LPCWSTR pcwszKeyChild
        );
};


//////////////////////////////////
// class declaration : CFactory //
//////////////////////////////////

class CFactory : public IClassFactory
{
 public:

    // Derived from IUnknown. So.......

    virtual HRESULT __stdcall QueryInterface(
        const IID& iid, 
        void** ppv
        );         

    virtual ULONG __stdcall AddRef(
        );

    virtual ULONG __stdcall Release(
        );


    // IClassFactory implementation

    virtual HRESULT __stdcall CreateInstance(
        IUnknown* pUnknownOuter,
        const IID& iid,
        void** ppv
        );

    virtual HRESULT __stdcall LockServer(
        BOOL bLock
        ); 

    CFactory(
        ) 
        : m_cRef(1) {
            }
  
    ~CFactory(
    )   {
        }


 protected:

    long m_cRef;
};

