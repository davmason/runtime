// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// ======================================================================================
//
// EventLog.cpp
//
// Implementation of ICorProfilerCallback2.
//
// ======================================================================================

#include "EventLog.h" 

// For StratoTanker -> SilverlightW2 integration
#ifndef CORPROF_E_PROFILER_NOT_ATTACHABLE
#define CORPROF_E_PROFILER_NOT_ATTACHABLE 0x80131368
#endif

CEventLog::CEventLog()
{
    printf("Event Log Test Profiler Loaded.\n");
}

CEventLog::~CEventLog()
{
    printf("Event Log Test Profiler Unloading.\n");
}

HRESULT CEventLog::Initialize(IUnknown * pProfilerInfoUnk)
{
    WCHAR target[256];
    if (GetEnvironmentVariable(L"PROF_INIT_CALLBACK_FAILED", target, 256))  
    {
        printf("EventLog::Initialize - PROF_INIT_CALLBACK_FAILED\n");
        return E_FAIL;
    }
    return S_OK;
}

HRESULT CEventLog::InitializeForAttach(
    IUnknown * pCorProfilerInfoUnk,
    void * pvClientData,
    UINT cbClientData)
{
    WCHAR target[256];

    if (GetEnvironmentVariable(L"PROF_INIT_CALLBACK_FAILED", target, 256))  
    {
        printf("EventLog::Initialize - PROF_INIT_CALLBACK_FAILED\n");
        return E_FAIL;
    }

    if (GetEnvironmentVariable(L"INIT_FOR_ATTACH_ENOTIMPL", target, 256))  
    {
        printf("EventLog::Initialize - INIT_FOR_ATTACH_ENOTIMPL\n");
        return E_NOTIMPL;
    }

    if (GetEnvironmentVariable(L"INIT_FOR_ATTACH_EXPLICIT_FORBID", target, 256))  
    {
        printf("EventLog::Initialize - INIT_FOR_ATTACH_EXPLICIT_FORBID\n");
        return CORPROF_E_PROFILER_NOT_ATTACHABLE;
    }

    return S_OK;
}

HRESULT WINAPI OnQIForCallback3(void* pv, REFIID riid, LPVOID* ppv, DWORD dw)
{
    *ppv = NULL;

    // This function should only be called for IID_ICorProfilerCallback3!
    if (!InlineIsEqualGUID(riid, IID_ICorProfilerCallback3))
    {
        return E_NOINTERFACE;
    }

    WCHAR target[256];
    // Is the test asking us to intentionally make like we don't implement
    // ICorProfilerCallback3?
    if (GetEnvironmentVariable(L"NO_CALLBACK3", target, 256))  
    {
        printf("EventLog::QueryInterface - NO_CALLBACK3\n");
        return E_NOINTERFACE;
    }

    // No one says otherwise, so just return ICorProfilerCallback3
    ICorProfilerCallback3 * pCallback3 = (ICorProfilerCallback3 *) pv;
    pCallback3->AddRef();
    *ppv = (LPVOID) pCallback3;
    return S_OK;
}


/***************************************************************************************
 ********************              DllMain/ClassFactory             ********************
 ***************************************************************************************/

class EventLogModule : public CAtlDllModuleT< EventLogModule >
{
public :
};

EventLogModule _AtlModule;

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
    WCHAR target[256];
    if (GetEnvironmentVariable(L"PROF_NO_CALLBACK_IFACE", target, 256))  
    {
        printf("DllGetClassObject - PROF_NO_CALLBACK_IFACE\n");
        return E_NOINTERFACE;
    }else if (GetEnvironmentVariable(L"PROF_CCI_FAILED", target, 256))  
    {
        printf("DllGetClassObject - PROF_CCI_FAILED\n");
        return E_FAIL;
    } else {
        return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
    }
}

STDAPI DllRegisterServer()
{
    return _AtlModule.DllRegisterServer(FALSE);
}


STDAPI DllUnregisterServer()
{
    return _AtlModule.DllUnregisterServer(FALSE);
}