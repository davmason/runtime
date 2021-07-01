// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "releaseondetach.h"

ReleaseOnDetach::ReleaseOnDetach() :
    _dispenser(NULL),
    _failures(0),
    _detachSucceeded(false)
{

}

ReleaseOnDetach::~ReleaseOnDetach()
{
    if (_dispenser != NULL)
    {
        _dispenser->Release();
        _dispenser = NULL;
    }

    if (_failures == 0 && _detachSucceeded)
    {
        // If we're here, that means we were Released enough to trigger the destructor
        LogMessage(U("PROFILER TEST PASSES"));
    }
    else
    {
        LogMessage(U("Test failed _failures={INT} _detachSucceeded={INT}"), _failures.load(), _detachSucceeded);
    }

    fflush(stdout);

    NotifyManagedCodeViaCallback();
}

GUID ReleaseOnDetach::GetClsid()
{
    // {B8C47A29-9C1D-4EEA-ABA0-8E8B3E3B792E}
    GUID clsid = { 0xB8C47A29, 0x9C1D, 0x4EEA, { 0xAB, 0xA0, 0x8E, 0x8B, 0x3E, 0x3B, 0x79, 0x2E } };
    return clsid;
}

HRESULT ReleaseOnDetach::InitializeForAttach(IUnknown* pICorProfilerInfoUnk, void* pvClientData, UINT cbClientData)
{
    HRESULT hr = Profiler::Initialize(pICorProfilerInfoUnk);
    if (FAILED(hr))
    {
        _failures++;
        LogFailure(U("Profiler::Initialize failed with hr={HR}"), hr);
        return hr;
    }

    LogMessage(U("Initialize for attach started"));

    DWORD eventMaskLow = COR_PRF_MONITOR_MODULE_LOADS;
    DWORD eventMaskHigh = 0x0;
    if (FAILED(hr = pCorProfilerInfo->SetEventMask2(eventMaskLow, eventMaskHigh)))
    {
        _failures++;
        LogFailure(U("ICorProfilerInfo::SetEventMask2() failed hr={HR}"), hr);
        return hr;
    }

    return S_OK;
}

HRESULT ReleaseOnDetach::Shutdown()
{
    Profiler::Shutdown();

    return S_OK;
}

HRESULT ReleaseOnDetach::ProfilerAttachComplete()
{
    SHUTDOWNGUARD();

    HRESULT hr = pCorProfilerInfo->RequestProfilerDetach(0);
    if (FAILED(hr))
    {
        _failures++;
        LogFailure(U("RequestProfilerDetach failed with hr={HR}"), hr);
    }
    else
    {
        LogMessage(U("RequestProfilerDetach successful"));
    }

    return S_OK;
}

HRESULT ReleaseOnDetach::ProfilerDetachSucceeded()
{
    SHUTDOWNGUARD();

    LogMessage(U("Profiler detach succeeded"));
    _detachSucceeded = true;
    return S_OK;
}
