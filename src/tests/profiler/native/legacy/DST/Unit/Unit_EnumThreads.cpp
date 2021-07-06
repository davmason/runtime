// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "../../ProfilerCommon.h"
#ifdef WINDOWS
#include <process.h>
#endif

#include <vector>
#include <algorithm>
using namespace std;


// Used to print extra information about the internal thread map
// to debug this test. Disabled by default
// #define DEBUG_TEST

map<ThreadID, bool> g_allThreadsMap;
std::mutex g_cs;

// global variable to track test failures.
static BOOL g_FailedTest = FALSE;

// Enumerate all the threads using ICorProfilerInfo4::EnumThreads and verify whether 
// a specific thread (threadId) is expected in the enumeration or not.
HRESULT EnumerateAllThreads(IPrfCom * pPrfCom, ThreadID threadId, bool bExpectedInEnumeration, bool bIsInitializingAtProfilerAttachComplete)
{
    HRESULT hr = S_OK;
    ULONG cObjFetched = 0;
    ObjectID * pObjects = NULL;
    ULONG cThreads = 0;
    COMPtrHolder<ICorProfilerThreadEnum> pThreadEnum;
	bool bFoundInEnumeration = false;
    
    
    // pre-condition checks
    if (NULL == pPrfCom)
    {
        FAILURE(L"Pre-condition check failed. pPrfCom is NULL");
        hr = E_INVALIDARG;
        goto ErrReturn;
    }
    if (0 == threadId)
    {
        FAILURE(L"Pre-condition check failed. threadId is NULL");
        hr = E_INVALIDARG;
        goto ErrReturn;
    }
    
                 
    // now let us start enumerating the loaded threads ......
    hr = pPrfCom->m_pInfo4->EnumThreads(&pThreadEnum);
    if (FAILED(hr))
    {
        FAILURE(L"ICorProfilerInfo::EnumThreads returned 0x" << std::hex << hr);
        goto ErrReturn;
    }

    // check the thread count reported by the enumeration. 
    hr = pThreadEnum->GetCount(&cThreads);
    if (FAILED(hr))
    {
        FAILURE(L"ICorProfilerThreadEnum::GetCount returned 0x" << std::hex << hr);
        goto ErrReturn;
    }

    // Retrieve all the thread info ..........
    pObjects = new ObjectID[cThreads];
    if (NULL == pObjects)
    {
        FAILURE(L"Failed to allocate memory");
        hr = E_OUTOFMEMORY;
        goto ErrReturn;
    }
    
    hr = pThreadEnum->Next(cThreads, pObjects, &cObjFetched);
    if (FAILED(hr))
    {
        FAILURE(L"ICorProfilerThreadEnum::Next returned 0x" << std::hex << hr);
        goto ErrReturn;
    }
    if (cObjFetched != cThreads)
    {
        FAILURE(L"cObjFetched != * pcThreads");
        hr = E_FAIL;
        goto ErrReturn;
    }

    // For each thread in the enumeration .......
    for (unsigned int idx = 0; idx < cObjFetched; idx++)
    {    
        lock_guard<mutex> guard(g_cs);

        bool threadIsValid = true;

        ThreadID curThreadId = (ThreadID) pObjects[idx];

        if (bIsInitializingAtProfilerAttachComplete)
        {
            // We are initializing inside profiler attach complete callback
            // Add the thread into the map
            g_allThreadsMap[curThreadId] = true;
        }
        else
        {
            // We are inside thread create/destroy 
            if (g_allThreadsMap.find(curThreadId) == g_allThreadsMap.end())
            {
                // If the thread is not in the map, either 
                // 1) it is a new thread that just become available and its thread created event is not fired yet (or fired but not yet added into the map), 
                // 2) it is a bug
                // We can verify that bug calling GetThreadInfo
                // In either case, continue with assuming that this is a valid ThreadID                                
            }
            else 
            {
                if (g_allThreadsMap[curThreadId])
                {
                    // The thread is active, and the thread id is valid
                }
                else
                {
                    // The thread id has been destroyed. Either
                    // 1) The thread id we just enumerated is the dead thread
                    // 2) The thread is a new thread that just become available but not yet added into the global map
                    // 3) This is a bug
                    // Unfortunately we can't tell which case so let's not touch it
                    threadIsValid = false;
                }
            }

        }
        
        // If the current thread ID is valid, retrieve its Win32 thread ID
        DWORD dwWin32ThreadId = 0;
        if (threadIsValid)
        {
            hr = pPrfCom->m_pInfo->GetThreadInfo(
                curThreadId,
                &dwWin32ThreadId);
            if (FAILED(hr))
            {
                FAILURE(L"ICorProfilerInfo::GetThreadInfo returned 0x" << std::hex << hr);
                goto ErrReturn;
            }

            if (dwWin32ThreadId <= 0)
            {
                FAILURE(L"ICorProfilerInfo::GetThreadInfo returned an invalid win32 thread ID = " << std::hex << dwWin32ThreadId);
                goto ErrReturn;                
            }
        }

        if (threadId == curThreadId)
        {
            bFoundInEnumeration = true;
        }

        DISPLAY(L"Thread ID = 0x" << hex << curThreadId);
        if (threadIsValid)
        {
            DISPLAY(L"Win32 Thread ID = 0x" << hex << dwWin32ThreadId);
        }
        else
        {
            DISPLAY(L"Win32 Thread ID ignored as the thread may be already dead.");
        }

		DISPLAY(L"");
    }
    
    if (bFoundInEnumeration != bExpectedInEnumeration)
    {
        DISPLAY(L"The following thread was ");
      
        if (!bExpectedInEnumeration)
            DISPLAY(L"not ");

        DISPLAY(L"expected in the enumeration : " << threadId);
      
        hr = E_FAIL;
        goto ErrReturn;
    }
    
    DISPLAY(L"");

    
 ErrReturn:

    DISPLAY(L"");
 
    if (NULL != pObjects) {
        delete[] pObjects; 
        pObjects = NULL;
    }
        
    if (FAILED(hr))
        g_FailedTest = TRUE;

    return hr;
}

#ifdef DEBUG_TEST

HRESULT PrintMap(IPrfCom * pPrfCom)
{
    for (map<ThreadID, bool>::const_iterator it = g_allThreadsMap.begin(); it != g_allThreadsMap.end(); ++it)
    {
        DISPLAY(L"Thread ID = 0x" << std::hex << it->first << ", " << L"Active = " << it->second);
    }

    return S_OK;
}

#endif

HRESULT Unit_EnumThreads_ThreadCreated(IPrfCom * pPrfCom, ThreadID threadId) 
{
    DISPLAY(L"Enumerating inside ThreadCreated callback\n");

    //
    // Mark the thread as active
    //
    {
        lock_guard<mutex> guard(g_cs);
        DISPLAY(L"Mark Thread 0x" << std::hex << threadId << L" as active");
#ifdef DEBUG_TEST
        PrintMap(pPrfCom);
#endif
        g_allThreadsMap[threadId] = true;
    }
    
    HRESULT hr = EnumerateAllThreads(
        pPrfCom, 
        threadId, 
        true,           // bExpectedInEnumeration
        false           // bIsInitializingAtProfilerAttachComplete
        );
    if (FAILED(hr))
    {
        FAILURE(L"EnumerateAllThreads() returned 0x" << std::hex << hr);
        goto ErrReturn;
    }

#ifdef DEBUG_TEST    
    {
        lock_guard<mutex> guard(g_cs);
        PrintMap(pPrfCom);
    }
#endif

 ErrReturn:

    DISPLAY(L"");
    return hr;

}

HRESULT Unit_EnumThreads_ThreadDestroyed(IPrfCom * pPrfCom, ThreadID threadId) 
{
    DISPLAY(L"Enumerating inside ThreadDestroyed callback\n");

    //
    // Mark the thread as dead
    //
    {
        lock_guard<mutex> guard(g_cs);
        DISPLAY(L"Mark Thread 0x" << std::hex << threadId << L" as dead");
#ifdef DEBUG_TEST        
        PrintMap(pPrfCom);
#endif        
        g_allThreadsMap[threadId] = false;
    }

    HRESULT hr = EnumerateAllThreads(
        pPrfCom, 
        threadId,
        false,          // bExpectedInEnumeration
        false           // bIsInitializingAtProfilerAttachComplete
        );
    if (FAILED(hr))
    {
        FAILURE(L"EnumerateAllThreads() returned 0x" << std::hex << hr);
        goto ErrReturn;
    }

#ifdef DEBUG_TEST
    {
        lock_guard<mutex> guard(g_cs);
        PrintMap(pPrfCom);
    }
#endif

 ErrReturn:

    DISPLAY(L"");
    return hr;
}

HRESULT Unit_EnumThreads_ProfilerAttachComplete( IPrfCom * pPrfCom )
{
    DISPLAY(L"Enumerating inside ProfilerAttachComplete callback\n");

    // Enum all threads and initialize the global thread map
    HRESULT hr = EnumerateAllThreads(
        pPrfCom, 
        -1,
        false,          // bExpectedInEnumeration
        true            // bIsInitializingAtProfilerAttachComplete
        );
    if (FAILED(hr))
    {
        FAILURE(L"EnumerateAllThreads() returned 0x" << std::hex << hr);
        goto ErrReturn;
    }

#ifdef DEBUG_TEST
    {
        lock_guard<mutex> guard(g_cs);
        PrintMap(pPrfCom);
    }
#endif
    
 ErrReturn:

    DISPLAY(L"");
    return hr;
}
                
//*********************************************************************
//  Descripton: Overall verification to ensure ???
//  
//  Params:     IPrfCom * pPrfCom   - 
//                  pointer to test module containing helper functions
//
//  Return:     S_OK if method successful, else E_FAIL
//  
//  Notes:      n/a
//*********************************************************************

HRESULT Unit_EnumThreads_Verify(IPrfCom * pPrfCom)
{
    HRESULT hr = S_OK;

    if (g_FailedTest)
    {
        FAILURE(L"Unit_EnumThreads has failed.");
        hr = E_FAIL;
        goto ErrReturn;
    }    

ErrReturn:
    return hr;
}


//*********************************************************************
//  Descripton: In this initialization step, we subscribe to profiler 
//              callbacks and also specify the necessary event mask. 
//  
//  Params:     IPrfCom * pPrfCom   - 
//                  pointer to test module containing helper functions
//              PMODULEMETHODTABLE pModuleMethodTable - 
//                  pointer to the method table which enables you
//                  to subscribe to profiler callbacks.
//
//  Return:     n/a
//  
//  Notes:      n/a
//*********************************************************************

void Unit_EnumThreads_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize UnitTests extension for EnumThreads")

    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_THREADS;
    pModuleMethodTable->VERIFY = (FC_VERIFY) &Unit_EnumThreads_Verify;

    pModuleMethodTable->THREADCREATED = (FC_THREADCREATED) &Unit_EnumThreads_ThreadCreated;
    pModuleMethodTable->THREADDESTROYED = (FC_THREADDESTROYED) &Unit_EnumThreads_ThreadDestroyed;
    pModuleMethodTable->PROFILERATTACHCOMPLETE = (FC_PROFILERATTACHCOMPLETE) &Unit_EnumThreads_ProfilerAttachComplete;

    lock_guard<mutex> guard(g_cs);

    return;
}
