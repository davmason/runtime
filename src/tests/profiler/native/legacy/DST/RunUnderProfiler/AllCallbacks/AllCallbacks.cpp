// ======================================================================================
//
// AllCallbacks.cpp
//
// Implementation of ICorProfilerCallback2.
//
// ======================================================================================

#include "AllCallbacks.h" 

_COM_SMARTPTR_TYPEDEF(ICorProfilerInfo2, __uuidof(ICorProfilerInfo2));

HRESULT CAllCallbacks::Initialize(IUnknown * pProfilerInfoUnk)
{
    ICorProfilerInfo2Ptr pICP2 = pProfilerInfoUnk;
    pICP2->SetEventMask(0x00);
    pICP2->SetFunctionIDMapper(0x00);
    pICP2->SetEnterLeaveFunctionHooks(0x00, 0x00, 0x00);
    pICP2->SetEnterLeaveFunctionHooks2(0x00, 0x00, 0x00);
    return S_OK;
}

/***************************************************************************************
 ********************              DllMain/ClassFactory             ********************
 ***************************************************************************************/

class AllCallbacksModule : public CAtlDllModuleT< AllCallbacksModule >
{
public :
};

AllCallbacksModule _AtlModule;

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

