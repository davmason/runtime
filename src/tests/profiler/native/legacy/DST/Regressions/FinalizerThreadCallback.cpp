// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
//
// FinalizerThreadCallback.cpp
//
// Regression test for coreclr issue #13499 (https://github.com/dotnet/coreclr/issues/13499)
// Description of the issue:
// Profiler was not being notified of ThreadCreated/ThreadAssignedToOSThread events for finalizer thread due to finalizer
// thread getting initialized too early in the runtime startup before the profiler was set up.
//
// Description of test:
// Ensure that ThreadCreated/ThreadAssignedToOSThread notification is called for finalizer thread.
//
// ======================================================================================

#include "../../ProfilerCommon.h"
#include <vector>
#include <mutex>

atomic<bool> g_postShutdown;

class FinalizerThreadCallback
{
public:

    FinalizerThreadCallback(IPrfCom * pPrfCom);
    ~FinalizerThreadCallback();

    // Called at the end of the tests.  Returning S_OK signifies the test passed and E_FAIL that it failed.
    HRESULT Verify(IPrfCom * pPrfCom);
    static HRESULT ThreadCreatedCallbackWrapper(IPrfCom * pPrfCom, ThreadID threadID)
    {
        if (g_postShutdown)
        {
            return S_OK;
        }

        return STATIC_CLASS_CALL(FinalizerThreadCallback)->ThreadCreatedCallback(pPrfCom, threadID);
    }

    static HRESULT ThreadAssignedToOSThreadCallbackWrapper(IPrfCom * pPrfCom, ThreadID managedThreadID, DWORD osThreadId)
    {
        if (g_postShutdown)
        {
            return S_OK;
        }

        return STATIC_CLASS_CALL(FinalizerThreadCallback)->ThreadAssignedToOSThreadCallback(pPrfCom, managedThreadID, osThreadId);
    }

private:
    HRESULT ThreadCreatedCallback(IPrfCom * pPrfCom, ThreadID threadID);
    HRESULT ThreadAssignedToOSThreadCallback(IPrfCom * pPrfCom, ThreadID managedThreadID, DWORD osThreadId);

    // ThreadID accumulators
    vector<ThreadID> _createdThreadIds;
    vector<ThreadID> _osAssignedThreadIds;
    ThreadID _lastThreadCreated;

    mutex _threadLock;
};

FinalizerThreadCallback::FinalizerThreadCallback(IPrfCom * /*pPrfCom*/)
{
    _lastThreadCreated = 0;
    g_postShutdown.store(false);
}

FinalizerThreadCallback::~FinalizerThreadCallback()
{
}

HRESULT FinalizerThreadCallback::ThreadCreatedCallback(IPrfCom * pPrfCom,
    ThreadID threadID)
{
    lock_guard<mutex> lock(_threadLock);
    _createdThreadIds.push_back(threadID);
    return S_OK;
}


HRESULT FinalizerThreadCallback::ThreadAssignedToOSThreadCallback(IPrfCom * pPrfCom,
    ThreadID managedThreadId,
    DWORD osThreadId)
{
    lock_guard<mutex> lock(_threadLock);
    _osAssignedThreadIds.push_back(managedThreadId);
    return S_OK;
}


HRESULT FinalizerThreadCallback::Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"FinalizerThreadCallback Verification...");

    // Signal that we are shutting down so the callbacks stop processing any
    // more threads. Otherwise we can run in to race conditions.
    g_postShutdown.store(true);

    lock_guard<mutex> lock(_threadLock);

    if (_createdThreadIds.size() < 2)
    {
        FAILURE(L"Expected at least 2 CreateThreadId callbacks (main + finalizer) but only received " << _createdThreadIds.size() << " callbacks");
    }

    if (_osAssignedThreadIds.size() < 2)
    {
        FAILURE(L"Expected at least 2 ThreadAssignedToOSThread callbacks (main + finalizer) but only received " << _osAssignedThreadIds.size() << " callbacks");
    }

    for (ThreadID threadId : _createdThreadIds)
    {
        if (std::find(_osAssignedThreadIds.begin(), _osAssignedThreadIds.end(), threadId) == _osAssignedThreadIds.end()
            // Threads can be created at any point during the lifecycle of the process,
            // so it's possible that we are doing this validation in the small window
            // between the ThreadCreated and ThreadAssignedToOSThread callbacks
            && threadId != _lastThreadCreated)
        {
            FAILURE(L"Found a thread that was not assigned to an OS thread: " << HEX(threadId));
            return E_FAIL;
        }
    }

    printf("Success!\n");
    return S_OK;
}


// static
HRESULT FinalizerThreadCallback_Verify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(FinalizerThreadCallback);
    HRESULT hr = pFinalizerThreadCallback->Verify(pPrfCom);
    FREE_CLASS_POINTER(FinalizerThreadCallback);

    return hr;
}


// static
void FinalizerThreadCallback_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize FinalizerThreadCallback");

    // Create and save an instance of test class
    SET_CLASS_POINTER(new FinalizerThreadCallback(pPrfCom));

    // Initialize MethodTable
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_THREADS;

    REGISTER_CALLBACK(THREADCREATED, FinalizerThreadCallback::ThreadCreatedCallbackWrapper);
    REGISTER_CALLBACK(THREADASSIGNEDTOOSTHREAD, FinalizerThreadCallback::ThreadAssignedToOSThreadCallbackWrapper);
    REGISTER_CALLBACK(VERIFY, FinalizerThreadCallback_Verify);

    return;
}
