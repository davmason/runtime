#pragma once

#include "TestProfiler.h"

#define HR_CHECK(_hr_, _exp_, _func_)   if (_exp_ != _hr_) {            \
                                            goto ErrReturn;             \
                                        }                               \


/////////////////////////////////
// class declaration : CCOMGoo //
/////////////////////////////////

class CCOMGoo
{
 public:

    // Count of active components
    static atomic<long> m_cComponents;

    // Count of locks
    static std::atomic<long> m_cServerLocks;

    // Friendly name of component
    static const wchar_t * m_pcwszFriendlyName;

    // Version-independent ProgID
    static const wchar_t * m_pcwszVerIndProgID;

    // ProgID
    static const wchar_t * m_pcwszProgID;
        
 #if defined(WIN32) || defined(WIN64)
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
#endif

private: 

 #if defined(WIN32) || defined(WIN64)
    static HRESULT SetKeyAndValue(
        LPCWSTR szKey, 
        LPCWSTR pcwszSubkey, 
        LPCWSTR pcwszValue
        );

    static HRESULT RecursiveDeleteKey(
        HKEY hKeyParent,
        LPCWSTR pcwszKeyChild
        );
#endif
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

    CFactory() :
        m_cRef(1) 
        {

        }
  
    virtual ~CFactory() = default;


 protected:

    std::atomic<long> m_cRef;
};

