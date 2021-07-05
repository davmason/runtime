// ======================================================================================
//
// NoCallbacks.cpp
//
// Implementation of ICorProfilerCallback2.
//
// ======================================================================================

#include "NoCallbacks.h" 

_COM_SMARTPTR_TYPEDEF(ICorProfilerInfo2, __uuidof(ICorProfilerInfo2));

HRESULT CNoCallbacks::Initialize(IUnknown * pProfilerInfoUnk)
{
    ICorProfilerInfo2Ptr pICP2 = pProfilerInfoUnk;
    pICP2->SetEventMask(COR_PRF_ALL);
    return S_OK;
}

/***************************************************************************************
 ********************              DllMain/ClassFactory             ********************
 ***************************************************************************************/

class NoCallbacksModule : public CAtlDllModuleT< NoCallbacksModule >
{
public :
};

NoCallbacksModule _AtlModule;

extern "C" BOOL WINAPI DllMain( HINSTANCE /*hInstance*/, DWORD dwReason, LPVOID lpReserved )
{
    return  _AtlModule.DllMain(dwReason,lpReserved);
} // DllMain


// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow()
{
    return _AtlModule.DllCanUnloadNow();
}

// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer()
{
    return _AtlModule.DllRegisterServer(FALSE);
}


STDAPI DllUnregisterServer()
{
    return _AtlModule.DllUnregisterServer(FALSE);
}