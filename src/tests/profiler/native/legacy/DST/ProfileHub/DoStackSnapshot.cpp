// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "DoStackSnapshot.h"


#if defined(_IA64_)

#pragma warning(push, 1)
//1>d:\vbl\ndp\clr\src\inc\corinfo.h(1473) : error C2220: warning treated as error - no 'object' file generated
//1>d:\vbl\ndp\clr\src\inc\corinfo.h(1473) : error C4510: 'CORINFO_String' : default constructor could not be generated
//1>d:\vbl\ndp\clr\src\inc\corinfo.h(1473) : error C4512: 'CORINFO_String' : assignment operator could not be generated
//1>d:\vbl\ndp\clr\src\inc\corinfo.h(1473) : error C4610: struct 'CORINFO_String' can never be instantiated - user defined constructor required

#define PSR_RI 41

struct FunctionDescriptor
{
    LPVOID EntryPoint;
    LPVOID Gp;
};

#pragma warning(pop)

#endif

#ifdef WIN32
//
// Private heap used by heap_allocator
//
PrivateHeap g_privateHeap;
#endif // WIN32

/*
 *  For Thread Local Storage
 */
thread_local ThreadInfoAndStacks *t_infoStacks;

/*
 * How many times we've seen a likely main method.
 */
static std::atomic<DWORD> gdwSeenMainMethod;

/*****************************************************************************************
 *
 *  Global static wrappers used for DoStackSnapshot Callback and function to Hijack to
 *
 *****************************************************************************************/

#ifdef THREAD_HIJACK
/*
 *  HiJackFuncWrapper is used as the HiJack function of the sampled thread
 */
#if defined(_X86_)
    __declspec( naked )
#endif
VOID HiJackFuncWrapper()
{
    // If we are hijacking, sample the current hijacked thread
    if (DoStackSnapshot::Instance()->m_sampleMethod == HIJACK)
        DoStackSnapshot::Instance()->SnapshotOneThread(NULL);

    // Else if we are doing cross thread inspecation, sample all the other threads
    else if (DoStackSnapshot::Instance()->m_sampleMethod == HIJACKANDINSPECT)
        DoStackSnapshot::Instance()->SampleAllThreads();

    // Signal HiJack work is done and to restore original state of thread.
    DoStackSnapshot::Instance()->HiJackSignalDone();

    // Infinite loop to make sure function does not return before thread hijack is undone.
    for(;;)
    {
    }
}
#endif // THREAD_HIJACK



/*
 *  Static this pointer used by wrappers to get instance of DoStackSnapshot class
 */
DoStackSnapshot* DoStackSnapshot::This = NULL;

/*
 *  Initialize the DoStackSnapshot class.  Set the initial state of the counters and initialize the
 *  synchronization objects.
 */ DoStackSnapshot::DoStackSnapshot(IPrfCom *pPrfCom,
                                 SynchronousCallMethod requestedCallMethod,
                                 AsynchronousSampleMethod requestedSampleMethod,
                                 ULONG minimumAvailabilityRate)
{
    // Static this pointer
    DoStackSnapshot::This = this;

    // Pointer to PrfCom used for Print and other utility routines
    m_pPrfCom = pPrfCom;

    // Synchronous or Cross Thread?
    m_callMethod = requestedCallMethod;

    // Sample, Hijack, HiJackAndInspect, or Async Availability
    m_sampleMethod = requestedSampleMethod;

    // EBP Stack Walking
    m_bEbpWalking = pPrfCom->m_bUseDSSWithEBP;
    m_bEbpStack = false;
    m_uMatched = 0;
    m_uNotMatched = 0;
    m_uDssLockThread = 0;
    m_uSnapshotFrames = 0;
    m_uShadowFrames = 0;
    m_uEbpFrames = 0;
    m_bEEStarted = false;
    m_uTImerCount = 0;
    m_ulEbpOnNonX86=0;

    // Are we also loaded an asynchronous thread to sample/hijack this process?
    if (m_sampleMethod != NO_ASYNCH)
    {
        // Sample timeout Negative value makes this relative rather than actual time.  We use the smallest
        // interval possible here, 1 = 100 nanosecond interval.
        m_liWaitTimeOut = 300;

#ifdef THREAD_HIJACK
        // Number of currently active hijacked threads
        m_activeHiJack = 0;
        m_hiJackFinished.Signal();
        m_bActiveSuspension = FALSE;
        m_bActiveHijack = FALSE;
#endif // THREAD_HIJACK

        m_bKeepGoing = TRUE;

        // Lets TestProfiler know asynchronous profiling is active.  When this flag is set in BPD, on
        // interesting callbacks it will check for a HardSuspend before delivering the callback to the loaded
        // profiler extension.
        PPRFCOM->InitializeForAsynch();

        // Now that we have set up all the state necessary to support our asynchronous sampling threads,
        // create the actual threads.
        std::thread sampleThread(SampleThreadFuncHelper, this);
        sampleThread.detach();
        m_sampleThreadRunning = TRUE;
        
#ifdef THREAD_HIJACK
        std::thread sampleNowThread(SampleNowThreadFuncHelper, this);
        sampleNowThread.detach();
        m_sampleNowThreadRunning = TRUE;
#endif // THREAD_HIJACK

        std::thread stopThread(StopThreadFuncHelper, this);
        stopThread.detach();
        m_stopThreadRunning = TRUE;
    }
    else
    {
        m_asynchThread = FALSE;
    }

    // Initialize result counters
    m_ulSuccess = 0;
    m_ulFailure = 0;
    m_ulUnmanaged = 0;
    m_ulInvalidArg=0;
    m_ulUnsafe = 0;
    m_ulAborted = 0;
    m_ulSemaphoreAbandon = 0;
    m_ulWaitForSingleObject = 0;
    m_ulHijackAbortUnmanaged = 0;
    m_ulSOKEmptySnapshot = 0;
    m_ulSOKEmptyShadow = 0;
    m_ulSOKEmptyEbp = 0;
    m_ulEFailEmptySnapshot = 0;
    m_ulSampleTicks = 0;
    m_ulMinAvail = minimumAvailabilityRate;

    gdwSeenMainMethod = 0;
    
    // When multiple profiler callbacks want to sample cross thread, they can not block if another thread is
    // already sampling.  If they do block they can introduce a deadlock if the runtime attempts to suspend
    // the tread for a GC or AppDomain unload.  To avoid this, the test uses a Semaphore with a count of 1.
    // If a thread wants to sample and can not take the Semaphore, it must return immediately.

    if (!m_lockHeapThread.Initialize())
    {
        FAILURE(L"LockHeapThread::Initialize() failed.");
    }
}


/*
 *  Clean up the static this pointer and synchronization objects
 */
DoStackSnapshot::~DoStackSnapshot()
{
    // Static this pointer
    DoStackSnapshot::This = NULL;

    if (m_asynchThread)
    {
        StopSampling();
        m_asynchThread = FALSE;
    }
}


/*
 *  This routine adds the current FunctionID to the Shadow Stack of a thread.  The function is added to the
 *  ThreadInfoAndStacks structure allocated for this thread in ThreadCreated.  It gets access to this
 *  structure using Thread Local Storage instead of iteration through the collection of threads to avoid
 *  deadlocks with the GC if this function were to block to obtain sole access to the collection.
 *  The name of the function is also added to the collection for this thread.
 */
HRESULT DoStackSnapshot::AddFunctionToShadowStack(FunctionID mappedFuncId)
{
    HRESULT hr = S_OK;

    // Get the TLS stack
    ThreadInfoAndStacks *pThreadInfo = t_infoStacks;

    // Make sure TLS has been set up for this thread.
    if (pThreadInfo == NULL)    
    {
        if (TRUE == m_pPrfCom->m_bAttachMode)     
        {
            // If running in attach-mode, we almost always miss threadcreation events by the time we attach.
            // Hence we may not have any cached info for some managed threads. The best thing to do in this
            // case is to skip past these.
            return S_OK;
        }
        else
        {
            // if this is a startup-mode test, then we have encountered a genuine failure
            FAILURE(L"NULL ThreadInfoAndStacks struct found in TLS for current thread in FunctionEnter2.");
            return E_FAIL;
        }        
    }

    // Search for function info already on thread
    ThreadFunctionMap::iterator iTFM = pThreadInfo->functionMap.find(mappedFuncId);

    // This is the first time the function has been called on this thread.  Update function info.
    if (iTFM == pThreadInfo->functionMap.end())
    {
        // Allocate new function structure
        FunctionNameAndInfo *pFuncInfo = new FunctionNameAndInfo();

        // Update the function Name
        ClassID classId = NULL;
        MUST_PASS(PINFO->GetFunctionInfo2(mappedFuncId,
                                          NULL,
                                          &classId,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL));
        WCHAR_STR( functionName );
        WCHAR_STR( className );
        if (classId != 0)
        {
            MUST_PASS(PPRFCOM->GetClassIDName(classId, className, FALSE));
        }
        MUST_PASS(PPRFCOM->GetFunctionIDName(mappedFuncId, functionName, NULL, FALSE));
        pFuncInfo->funcName += className.append(L"::").append(functionName);

        // Insert the function into the cache
        pThreadInfo->functionMap.insert(make_pair(mappedFuncId, pFuncInfo));
    }

    // Add function to shadow stack
    pThreadInfo->shadowStack.push_back(mappedFuncId);
    return hr;
}


HRESULT DoStackSnapshot::RemoveExceptionFromShadowStack()
{
    COR_PRF_EX_CLAUSE_INFO exInfo;
    HRESULT hr = PINFO->GetNotifiedExceptionClauseInfo(&exInfo);
    if (!SUCCEEDED(hr))
    {
        FAILURE(L"GetNotifiedExceptionClauseInfo returned hr = " << HEX(hr));
        return hr;
    }

    if (hr != S_FALSE)
    {
        FunctionID functionId;

        MUST_PASS(PINFO->GetFunctionFromIP((LPCBYTE)exInfo.programCounter, &functionId));
        return RemoveFunctionFromShadowStack(functionId, FALSE);
    }
    else
    {
        return RemoveFunctionFromShadowStack(NULL, TRUE);
    }
}


/*
 *  This routine removes the current FunctionID from the Shadow Stack of a thread.  The function is removed
 *  from the ThreadInfoAndStacks structure allocated for this thread in ThreadCreated.  It gets access to this
 *  structure using Thread Local Storage instead of iteration through the collection of threads to avoid 
 *  deadlocks with the GC if this function were to block to obtain sole access to the collection.
 */ 
HRESULT DoStackSnapshot::RemoveFunctionFromShadowStack(FunctionID mappedFuncId, BOOL bUnwind)
{
    HRESULT hr = S_OK;

    // Get the TLS stack
    ThreadInfoAndStacks *pThreadInfo = t_infoStacks;

    // Make sure TLS has been set up for this thread.
    if (pThreadInfo == NULL)
    {
        if (TRUE == m_pPrfCom->m_bAttachMode)     
        {
            // If running in attach-mode, we almost always miss threadcreation events by the time we attach.
            // Hence we may not have any cached info for some managed threads. The best thing to do in this
            // case is to skip past these.
            return S_OK;
        }
        else
        {
            // if this is a startup-mode test, then we have encountered a genuine failure
            FAILURE(L"NULL ThreadInfoAndStacks struct found in TLS for current thread in FunctionEnter2.");
            return E_FAIL;
        }        
    }

    // Only perform this check if we have the functionID.  FunctionID is not available in Unwind
    if ((bUnwind == FALSE) && (pThreadInfo->shadowStack.back() != mappedFuncId))
    {
        DISPLAY(L"RemoveFunctionFromShadowStack called on " << mappedFuncId << L" while top of stack is " << pThreadInfo->shadowStack.back());

        FunctionID funcId;

        DISPLAY(L"Current Shadow Stack:\n");
        for (INT i = (INT)pThreadInfo->shadowStack.size() - 1; i >= 0; i--)
        {
            funcId = pThreadInfo->shadowStack.at(i);
            ThreadFunctionMap::iterator iTFM = pThreadInfo->functionMap.find(funcId);
            if (iTFM == pThreadInfo->functionMap.end())
            {
                DISPLAY(funcId << L" - Function not yet in function table.");
            }
            else
            {
                DISPLAY(funcId << L" - " << iTFM->second->funcName);
            }
        }

        hr = E_FAIL;
    }

    pThreadInfo->shadowStack.pop_back();

    //
    // !!HACK HACK HACK!!
    //
    // For CoreCLR, as we are always missing the profiler shutdown notification, profilers tests have 
    // to use DLL_PROCESS_DETACH to stop sampling and do verification. However, this can be a problem
    // because before the profiler DLL gets DLL_PROCESS_DETACH, when CoreCLR gets DLL_PROCESS_DETACH,
    // another thread could be already suspended with the internal CRT heap lock taken and 
    // during shutdown CRT may try to take the same lock (when freeing FLS) and may need to wait. 
    // When OS sees that we are about to wait inside DLL_PROCESS_DETACH, it will call TerminateProcess
    // as it believes we are about to deadlock, and as a result the verification never gets a chance
    // to run. Fortunately this usually happens with our own test cases and not in profilers written 
    // by customers as they generally don't have to perform any special task in shut down.
    //
    // The work around at this moment is to stop the test case from doing sampling if it sees that we
    // are about to leave the static "Main" function. We should only see exactly one main otherwise 
    // we would fail. I could also use the entry point token but using static main is easier.
    //
    if (m_sampleMethod != NO_ASYNCH)
    {
        ModuleID moduleId;
        mdToken methodDef;
        if (SUCCEEDED(PINFO->GetFunctionInfo(mappedFuncId, NULL, &moduleId, &methodDef)))
        {
            IMetaDataImport *pImport = NULL;
            if (SUCCEEDED(PINFO->GetModuleMetaData(moduleId, ofRead, IID_IMetaDataImport, (IUnknown **)&pImport)) && pImport != NULL)
            {
                PROFILER_WCHAR wszMethodNameTemp[STRING_LENGTH];
                PLATFORM_WCHAR wszMethodName[STRING_LENGTH];

                DWORD cchMethodName;
                DWORD dwMethodAttr;
                if (SUCCEEDED(pImport->GetMethodProps(methodDef, NULL, wszMethodNameTemp, 255, &cchMethodName, &dwMethodAttr, NULL, NULL, NULL, NULL)))
                {
                    ConvertProfilerWCharToPlatformWChar(wszMethodName, STRING_LENGTH, wszMethodNameTemp, STRING_LENGTH);
                    ConvertWStringToLower(wszMethodName, STRING_LENGTH);

                    // A static Main. Most likely the entry point
                    // Even though we could've looked up the entry point from IMAGE_COR20_HEADER 
                    // but static Main is good enough
                    if (wcscmp(wszMethodName, L"main") == 0 && (dwMethodAttr & mdStatic) != 0)
                    {
                        DISPLAY(L"HACK: exiting " << wszMethodName << L"@" << std::hex << mappedFuncId << L" and it is most likely the entry point.");
                        DISPLAY(L"HACK: We need to stop sampling now to avoid deadlock/early termination during shutdown.");

                        // The locking here isn't perfect but good enough (we may end up failure more than once in different threads but who cares)
                        ++gdwSeenMainMethod;
                        if (gdwSeenMainMethod > 1)
                        {
                            FAILURE(L"HACK: More than one static Main method is seen. Please fix the hack to work with this test case. Failing test...");                            
                            return E_FAIL;
                        }                        
                        else
                        {
                            StopSampling();
                        }
                    }
                }
            }
        }
    }
    
    return hr;
}


/*
 *  A new managed thread was created.  Allocate a new ThreadInfoAndStacks structure to be our bookkeeping
 *  structure for this thread.  Add the structure to the thread Map used by the Sample routine as well as add
 *  the pointer to Thread Local Storage for use by the AddFuctionToShadowStack and Remove
 *  functionFromShadowStack routines. This function waits on the synchronization semaphore with an infinite
 *  timeout. It must be able to add the function to the map, we can not loose this event.  And the runtime is
 *  aware that we can block from this function, so it will not be called from cooperative mode where a GC
 *  deadlock could occur
 */
HRESULT DoStackSnapshot::ThreadCreated(ThreadID managedThreadId)
{
    HRESULT hr = S_OK;

    // Wait on semaphore with an infinite  time out.  We need control and it is safe to block from here.
    m_semaphore.Take();

    // look to see if thread already exists in map
    ThreadInfoStackMap::iterator iTISM = m_threadInfoStackMap.find(managedThreadId);

    // if thread already exists, we have a problem.  why are we seeing this thread created again?
    if (iTISM != m_threadInfoStackMap.end())
    {
        FAILURE(L"ERROR: Second ThreadCreated callback received for ThreadID " << managedThreadId);
        // Increment the count of the semaphore.
        m_semaphore.Release();
        return E_FAIL;
    }

    // Allocate the new structure for this tread
    ThreadInfoAndStacks *pThreadInfo = new ThreadInfoAndStacks();
    if (pThreadInfo == NULL)
    {
        FAILURE(L"Allocation for FunctionStack failed\n");
        // Increment the count of the semaphore.
        m_semaphore.Release();
        return E_FAIL;
    }

    // Fill out the initial information about this new thread.
    pThreadInfo->threadId = managedThreadId;
    PINFO->GetThreadInfo(managedThreadId, &pThreadInfo->win32threadId);

    HANDLE tHandle;
    hr = PINFO->GetHandleFromThread(managedThreadId, &tHandle);

    if (FAILED(hr))
    {
        FAILURE(L"GetHandleFromThread failed for ThreadID " << managedThreadId);
        delete pThreadInfo;
        // Increment the count of the semaphore.
        m_semaphore.Release();
        return E_FAIL;
    }

#ifdef THREAD_HIJACK
    HANDLE processHandle = GetCurrentProcess();

    if (!DuplicateHandle(processHandle,
                                  tHandle,
                                  processHandle,
                                  &pThreadInfo->threadHandle,
                                  0,
                                  FALSE,
                                  DUPLICATE_SAME_ACCESS) )
    {
        FAILURE(L"DuplicateHandle failed " << GetLastError());
        delete pThreadInfo;
        // Increment the count of the semaphore.
        m_semaphore.Release();
        return E_FAIL;
    }
#endif // THREAD_HIJACK

    // Add the thread to our Map to sample later
    m_threadInfoStackMap.insert(make_pair(managedThreadId, pThreadInfo));

    // Insert pointer to ThreadInfoAndStacks for this thread into TLS
    t_infoStacks = pThreadInfo;

    // Increment the count of the semaphore.
    m_semaphore.Release();

    return hr;
}


/*
 *  A managed thread was destroyed.  Remove it from Thread Local Storage and our Map of threads to sample.
 */
HRESULT DoStackSnapshot::ThreadDestroyed(ThreadID managedThreadId)
{
    HRESULT hr = S_OK;

    // Wait on semaphore with an infinite  time out.  We need control and it is safe to block from here.
    m_semaphore.Take();

    // look to see if thread already exists in map
    ThreadInfoStackMap::iterator iTISM = m_threadInfoStackMap.find(managedThreadId);

    if (iTISM == m_threadInfoStackMap.end())
    {
        FAILURE(L"ERROR: Threaddestroyed callback received for thread that did not have a ThreadCreated callback: ThreadID " << managedThreadId);
        // Increment the count of the semaphore.
        m_semaphore.Release();
        return E_FAIL;
    }

    ThreadInfoAndStacks *pThreadInfo = iTISM->second;
    if (pThreadInfo == NULL)
    {
        FAILURE(L"NULL ThreadInfoAndStacks for ThreadID " << managedThreadId);
        // Increment the count of the semaphore.
        m_semaphore.Release();
        return E_FAIL;
    }

    // Did we receive a ThreadDestroyed callback for a thread that still has frames on it?
    if (pThreadInfo->shadowStack.empty() != TRUE)
    {
        DISPLAY(L"Thread " << iTISM->first << L" destroyed while it still has frames on it.");
        while(pThreadInfo->shadowStack.empty() != TRUE)
        {
         DISPLAY(L"FunctionID " << pThreadInfo->shadowStack.back());
            pThreadInfo->shadowStack.pop_back();
        }

        FAILURE(L"Look at output for failure reason\n");
    }

#ifdef THREAD_HIJACK
    CloseHandle(pThreadInfo->threadHandle);
#endif // THREAD_HIJACK

    // Clean up our ThreadInfoAndStacks struct
    delete pThreadInfo;

    // Only clean TLS if the target thread and the current thread are the same thread.
    ThreadID currentThread = NULL;
    PINFO->GetCurrentThreadID(&currentThread);
    if (currentThread == managedThreadId)
    {
        t_infoStacks = NULL;
    }

    m_threadInfoStackMap.erase(iTISM);

    // Increment the count of the semaphore.
    m_semaphore.Release();

    return hr;
}

HRESULT DoStackSnapshot::ModuleLoadFinished(ModuleID moduleId, HRESULT hrStatus)
{
    m_bEEStarted = true;
    return S_OK;
}
/*
 *  Start asynchronous profiler extension.  Asynchronous thread should already exist and be waiting on first
 *  event.
 */ 
VOID DoStackSnapshot::StartSampling()
{
    auto timerThreadCallback = [&]() {
        YieldProc(m_liWaitTimeOut);
        m_sample.Signal();
    };
    
    std::thread timerThread(timerThreadCallback);
    timerThread.detach();
}


/*
 * Signal asynchronous thread to stop sampling.
 */
VOID DoStackSnapshot::StopSampling()
{
    // Signal Asynchronous Thread to Stop
    m_stop.Signal();
    //Wait for the sampling thread to stop.
    while(  m_sampleThreadRunning.load() || 
#ifdef THREAD_HIJACK
            m_sampleNowThreadRunning.load() || 
#endif // THREAD_HIJACK
            m_stopThreadRunning.load())
    {
        YieldProc();
    }

#ifdef THREAD_HIJACK
    // Do not return until previous HiJack operation is done
    while(m_activeHiJack.load() != 0)
    {
        YieldProc();
    }
#endif // THREAD_HIJACK
}


#ifdef THREAD_HIJACK
/*
 * Signal asynchronous thread to take a Sample.
 */
VOID DoStackSnapshot::SampleNow()
{
    // Signal Asynchronous Thread to Stop
    m_sampleNow.Signal();
}
#endif // THREAD_HIJACK

VOID DoStackSnapshot::SampleThreadFuncHelper(DoStackSnapshot *dss)
{
    dss->SampleThreadFunc();
}

#ifdef THREAD_HIJACK
VOID DoStackSnapshot::SampleNowThreadFuncHelper(DoStackSnapshot *dss)
{
    dss->SampleNowThreadFunc();
}
#endif // THREAD_HIJACK

VOID DoStackSnapshot::StopThreadFuncHelper(DoStackSnapshot *dss)
{
    dss->StopThreadFunc();
}

VOID DoStackSnapshot::SampleThreadFunc()
{
    PPRFCOM->m_pInfo4->InitializeCurrentThread();
    DISPLAY(L"SampleThread started");
    
    BOOL firstInfoCallComplete = FALSE;
    ThreadInfoStackMap::iterator iTISM;

    while (m_bKeepGoing)
    {
        m_sample.Wait();

        {
            lock_guard<mutex> guard(m_sampling);
            if ((m_sampleMethod == HIJACK) || (m_sampleMethod == HIJACKANDINSPECT))
            {
                // Signal request for hard suspend
                PPRFCOM->RequestHardSuspend();

                if (firstInfoCallComplete == FALSE)
                {
                    firstInfoCallComplete = TRUE;

                    // We need to set up the TLS for this thread in the runtime.  The first time we make
                    // an ICorProfilerInfo2 call the runtime allocates some Thread Local Storage for our
                    // thread as that is the first time the runtime has seen this thread since we created
                    // it ourselves.
                    //
                    // This step is required before we start doing any actual work since the first call
                    // we make could be while we have a thread suspended to hijack it.  If that suspended
                    // thread has the heap lock we are then deadlocked.
                    //
                    // For more information on this call, see VSWhidbey 486923
                    ThreadID threadId = NULL;
                    PINFO->GetCurrentThreadID(&threadId);
                }
            }
            // Wait on semaphore with an infinite time out.  We need control and it is safe to block
            // from here.
            m_semaphore.Take();

            if (m_sampleMethod == SAMPLE || m_sampleMethod == SAMPLE_AND_TEST_DEADLOCK)
            {
                // Call Sample each thread
                SampleAllThreads();
            }
#ifdef THREAD_HIJACK
            else if ((m_sampleMethod == HIJACK) || (m_sampleMethod == HIJACKANDINSPECT))
            {
                ++m_activeHiJack;

                // Call Hijack Function on each thread
                for (iTISM = m_threadInfoStackMap.begin();
                     iTISM != m_threadInfoStackMap.end();
                     iTISM++)
                {
                    ThreadInfoAndStacks *pThreadInfo = iTISM->second;
                    HiJackThread(pThreadInfo->threadId, pThreadInfo->threadHandle);
                }

                --m_activeHiJack;
            }
            else if (m_sampleMethod == ASYNC_AVAILABILITY)
            {
                m_ulSampleTicks++;

                // Call Availability Function on each thread
                for (iTISM = m_threadInfoStackMap.begin();
                     iTISM != m_threadInfoStackMap.end();
                     iTISM++)
                {

                    SuspendThread(((ThreadInfoAndStacks *)iTISM->second)->threadHandle);
                }

                // Call Availability Function on each thread
                for (iTISM = m_threadInfoStackMap.begin();
                     iTISM != m_threadInfoStackMap.end();
                     iTISM++)
                {
                    MeasureDSSAvailability(((ThreadInfoAndStacks *)iTISM->second)->threadId);
                }

                // Call Availability Function on each thread
                for (iTISM = m_threadInfoStackMap.begin();
                     iTISM != m_threadInfoStackMap.end();
                     iTISM++)
                {
                    ResumeThread(((ThreadInfoAndStacks *)iTISM->second)->threadHandle);
                }
            }
#endif // THREAD_HIJACK

            m_semaphore.Release();

            if ((m_sampleMethod == HIJACK) || (m_sampleMethod == HIJACKANDINSPECT))
            {
                // Signal to resume execution
                PPRFCOM->ReleaseHardSuspend();
            }
        }

        StartSampling();
    }

    m_sampleThreadRunning = FALSE;
}

#ifdef THREAD_HIJACK
VOID DoStackSnapshot::SampleNowThreadFunc()
{
    PPRFCOM->m_pInfo4->InitializeCurrentThread();
    DISPLAY(L"SampleNowThread started");
    
    while (m_bKeepGoing)
    {
        m_sampleNow.Wait();

        {
            lock_guard<mutex> guard(m_sampling);

            if ((m_sampleMethod == HIJACK) || (m_sampleMethod == HIJACKANDINSPECT))
            {
                // Signal request for hard suspend
                PPRFCOM->RequestHardSuspend();
            }
            // Wait on semaphore with an infinite time out.  We need control and it is safe to block
            // from here.
            m_semaphore.Take();

            if (m_sampleMethod == SAMPLE || m_sampleMethod == SAMPLE_AND_TEST_DEADLOCK)
            {
                // Call Sample each thread
                SampleAllThreads();
            }
            else if ((m_sampleMethod == HIJACK) || (m_sampleMethod == HIJACKANDINSPECT))
            {
                ++m_activeHiJack;

                // Call Hijack Function on each thread
                for (ThreadInfoStackMap::iterator iTISM = m_threadInfoStackMap.begin();
                     iTISM != m_threadInfoStackMap.end();
                     iTISM++)
                {
                    ThreadInfoAndStacks *pThreadInfo = iTISM->second;
                    HiJackThread(pThreadInfo->threadId, pThreadInfo->threadHandle);
                }

                --m_activeHiJack;
            }

            m_semaphore.Release();

            if ((m_sampleMethod == HIJACK) || (m_sampleMethod == HIJACKANDINSPECT))
            {
                // Signal to resume execution
                PPRFCOM->ReleaseHardSuspend();
            }
        }
    }
    
    m_sampleNowThreadRunning = FALSE;
}
#endif // THREAD_HIJACK

VOID DoStackSnapshot::StopThreadFunc()
{
    PPRFCOM->m_pInfo4->InitializeCurrentThread();
    DISPLAY(L"StopThread started");

    m_stop.Wait();
    m_bKeepGoing = FALSE;

    m_stopThreadRunning = FALSE;
}

#ifdef THREAD_HIJACK
/*
 *  Managed thread to be sampled must be HiJacked prior to being sampled.
 *
 *  The steps of HiJacking are:
 *      Suspend target managed thread
 *      Get thread context of managed thread
 *      Modify Context.EIP to point at profiler function
 *      Set thread context to new modified context
 *      Resume managed thread
 *
 *  The HiJackThread function then must wait for HiJacked thread to signal that it is finished with the
 *  sampling operation.  Then it suspends the thread again, restores the original thread context.
 *
 */
void DoStackSnapshot::HiJackThread(ThreadID threadId, HANDLE threadHandle)
{
    HRESULT hr = S_OK;
    BOOL result = TRUE;
    FunctionID fId = NULL;
    DWORD ret = 0;

    // Quick Sanity check on the parameters
    if ((threadId == NULL) || (threadHandle == NULL) )
    {
        FAILURE(L"A NULL parameter was passed to PrfAsynchronous::HiJackThread\n ThreadID=" << threadId << L", HANDLE=" << threadHandle);
    }

    // Make sure we can HiJack;
    if (!IsHijackAllowed())
    {
        return;
    }

    // Remember the hijacked thread so we don't try and DSS it without a CONTEXT
    m_hijackedThreadID = threadId;

    // clear the context structure
    memset(&m_threadContext, 0, sizeof(CONTEXT));

    // Suspend the thread
    ret = SuspendThread(threadHandle);
    if (ret == (DWORD)-1)
    {
        FAILURE(L"FAILURE: SuspendThread returned " << GetLastError() << L" for ThreadID " << threadId << L" Handle " << threadHandle);
        // Set the HiJack state
        HijackFinished();
        return;
    }

    // ContextFlags must be set or GetThreadContext will not return any information
    ((CONTEXT *)&m_threadContext)->ContextFlags = CONTEXT_ALL;
    result = GetThreadContext(threadHandle, (CONTEXT *)m_threadContext);
    if (!result)
    {
        FAILURE(L"FAILURE: GetThreadContext returned " << GetLastError() << L" for ThreadID " << threadId << L" Handle " << threadHandle);
        ResumeThread(threadHandle);
        // Set the HiJack state
        HijackFinished();
        return;
    }

    // Perform HiJack Work here!
#if defined(_X86_)
        // Save originals
        m_originalEIP = ((CONTEXT *)&m_threadContext)->Eip;
        m_originalESP = ((CONTEXT *)&m_threadContext)->Esp;

        ((CONTEXT *)&m_threadContext)->Eip = (DWORD)&HiJackFuncWrapper;

        // Only HiJack from Managed Code
        hr = PINFO->GetFunctionFromIP((LPCBYTE)m_originalEIP, &fId);
        if (FAILED(hr))
        {
            m_ulHijackAbortUnmanaged++;
            ResumeThread(threadHandle);
            // Set the HiJack state
            HijackFinished();
            return;
        }

#elif defined(_IA64_)
        // Save originals
        m_originalStIIP = ((CONTEXT *)&m_threadContext)->StIIP;
        m_originalStIPSR = ((CONTEXT *)&m_threadContext)->StIPSR;
        m_originalIntSp = ((CONTEXT *)&m_threadContext)->IntSp;
        m_originalIntGp = ((CONTEXT *)&m_threadContext)->IntGp;

        // Do ia64 magic
        FunctionDescriptor* pFD = (FunctionDescriptor*)&HiJackFuncWrapper;
        ((CONTEXT *)&m_threadContext)->StIIP = (ULONGLONG)pFD->EntryPoint;
        ((CONTEXT *)&m_threadContext)->IntGp = (ULONGLONG)pFD->Gp;

        UINT64 slot = (((UINT64)&HiJackFuncWrapper) & (IA64_BUNDLE_SIZE-1)) >> 2;
        ((CONTEXT *)&m_threadContext)->StIPSR &= ~(INT64(0x3) << PSR_RI);
        ((CONTEXT *)&m_threadContext)->StIPSR |= (slot << PSR_RI);

        // Align the stack
        ((CONTEXT *)&m_threadContext)->IntSp -= 0x400;
        ((CONTEXT *)&m_threadContext)->IntSp &= ~0xF;


        // Only HiJack from Managed Code
        hr = PINFO->GetFunctionFromIP((LPCBYTE)m_originalStIIP, &fId);
        if (FAILED(hr))
        {
            m_ulHijackAbortUnmanaged++;
            ResumeThread(threadHandle);
            // Set the HiJack state
            HijackFinished();
            return;
        }

#elif defined(_AMD64_)
        // Save off the non-hijacked IP
        m_originalRip = ((CONTEXT *)&m_threadContext)->Rip;
        // Save off the non-hijacked SP.  Assume we have to mess with the stack alignment.
        m_originalRsp = ((CONTEXT *)&m_threadContext)->Rsp;

        ((CONTEXT *)&m_threadContext)->Rip = (ULONG64)&HiJackFuncWrapper;
        ((CONTEXT *)&m_threadContext)->Rsp -= 0x400;

        // Align stack to 8 mod 16
        ((CONTEXT *)&m_threadContext)->Rsp &= ~0xF;
        ((CONTEXT *)&m_threadContext)->Rsp += 8;

        // Only HiJack from Managed Code
        hr = PINFO->GetFunctionFromIP((LPCBYTE)m_originalRip, &fId);
        if (FAILED(hr))
        {
            m_ulHijackAbortUnmanaged++;
            ResumeThread(threadHandle);
            // Set the HiJack state
            HijackFinished();
            return;
        }

#endif

    result = SetThreadContext(threadHandle, (CONTEXT *)m_threadContext);
    if (!result)
    {
        FAILURE(L"FAILURE: SetThreadContext returned " << GetLastError() << L" for ThreadID " << threadId << L" Handle " << threadHandle);
        // Set the HiJack state
        ResumeThread(threadHandle);
        HijackFinished();
        return;
    }

    // We've set the thread context to the HiJack function.  Now restore the original IP for when 
    // DoStackSnapshot uses the Context.
#if defined(_X86_)
        ((CONTEXT *)&m_threadContext)->Eip = m_originalEIP;
        ((CONTEXT *)&m_threadContext)->Esp = m_originalESP;

#elif defined(_IA64_)
        ((CONTEXT *)&m_threadContext)->StIIP = m_originalStIIP;
        ((CONTEXT *)&m_threadContext)->StIPSR = m_originalStIPSR;
        ((CONTEXT *)&m_threadContext)->IntSp = m_originalIntSp;
        ((CONTEXT *)&m_threadContext)->IntGp = m_originalIntGp;

#elif defined(_AMD64_)
        ((CONTEXT *)&m_threadContext)->Rip = m_originalRip;
        ((CONTEXT *)&m_threadContext)->Rsp = m_originalRsp;
#endif

    // Now that the EIP of the thread is pointing at our function, resume thread execution.
    ResumeThread(threadHandle);

    // Wait for signal from our HiJack function.
    auto callback = [&]() {
        // We lost our hijack.  (The GC probably stole it)  Simply return.  Do not suspend/resume or muck
        // with the target thread's context.
        DISPLAY(L"WaitForSingleObject timed out. Aborting.\n");
        m_ulWaitForSingleObject++;
        HijackFinished();
        --m_activeHiJack;
        m_semaphore.Release();
        Verify();
        TerminateProcess( GetCurrentProcess(), 0 );
    };
    m_hiJackSignal.Wait(callback);

    // Once we are signaled, restore the original Context by suspending the thread and resetting the EIP 
    // value.
    SuspendThread(threadHandle);

    result = SetThreadContext(threadHandle, (CONTEXT *)m_threadContext);
    if (!result)
    {
        FAILURE(L"FAILURE: SetThreadContext returned " << GetLastError() << L" for ThreadID " << threadId << L" Handle " << threadHandle);
        ResumeThread(threadHandle);
        HijackFinished();
        return;
    }

    // Resume the thread in it's original state
    ResumeThread(threadHandle);

    // Set the HiJack state
    HijackFinished();

}


BOOL DoStackSnapshot::IsHijackAllowed()
{
    lock_guard<mutex> guard(m_csSuspensions);

    if(m_bActiveSuspension == TRUE)
    {
        return FALSE;
    }

    m_hiJackFinished.Reset();
    m_bActiveHijack = TRUE;
    return TRUE;
}


VOID DoStackSnapshot::HijackFinished()
{
    lock_guard<mutex> guard(m_csSuspensions);

    m_bActiveHijack = FALSE;
    m_hiJackFinished.Signal();
}


VOID DoStackSnapshot::ProhibitHijackAndWait()
{
    lock_guard<mutex> guard(m_csSuspensions);
    
    m_bActiveSuspension = TRUE;

    if (m_bActiveHijack == FALSE)
    {
        return;
    }

    m_hiJackFinished.Wait();
}


VOID DoStackSnapshot::AllowHijack()
{
    lock_guard<mutex> guard(m_csSuspensions);

    m_bActiveSuspension = FALSE;
}
#endif // THREAD_HIJACK


/*
 *  Sample all managed threads in the process
 */
VOID DoStackSnapshot::SampleAllThreads()
{
    // Call Sample each thread
    for (ThreadInfoStackMap::iterator iTISM = m_threadInfoStackMap.begin();
        iTISM != m_threadInfoStackMap.end();
        iTISM++)
    {
        ThreadInfoAndStacks *pThreadInfo = iTISM->second;

#ifdef THREAD_HIJACK
        if ((m_sampleMethod == HIJACKANDINSPECT) && (pThreadInfo->threadId == m_hijackedThreadID))
        {
            continue;
        }
#endif // THREAD_HIJACK

        SnapshotOneThread(pThreadInfo->threadId);
    }
}


/*
 *  This function performs a DoStackSnapshot stack walk on all the currently executing managed threads in the
 *  process.  This function is called from many different profiler callbacks.  Calls to DoStackSnapshot must
 *  be serialized, but multiple threads wanting to call DoStackSnapshot can not wait from a Profiler callback
 *  or they could induce a deadlock if a GC were to occur. To avoid this, a Semaphore with a count of 1 is
 *  used instead of a Critical Section.
 */ 
HRESULT DoStackSnapshot::SnapshotAllThreads()
{
    HRESULT hr = S_OK;
    ThreadInfoStackMap::iterator * piTISM = NULL;

    // Wait on semaphore with 0 second time out.  If we can take the semaphore we are the only thread
    // sampling right now.  If we timeout we can not block and must return control to the runtime.
    if(!m_semaphore.TryTake())
    {
            // we cannot sample.  return
            m_ulSemaphoreAbandon++;
            return hr;
    }

    ThreadID threadId;
    hr = PINFO->GetCurrentThreadID(&threadId);

    // First HandleCreated callback occurs before managed code has run
    if (hr == CORPROF_E_NOT_MANAGED_THREAD)
        goto SNAPSHOT_ALLTHREADS_EXIT;

    else if (FAILED(hr))
    {
        FAILURE(L"GetCurrentThreadID returned HR=" << HEX(hr));
        goto SNAPSHOT_ALLTHREADS_EXIT;
    }

    // Sample each of the threads.
    piTISM = new ThreadInfoStackMap::iterator();
    (*piTISM) = m_threadInfoStackMap.begin();
    while((*piTISM) != m_threadInfoStackMap.end())
    {
        // Dont' want our own stack.  That's another test.
        if ((*piTISM)->first != threadId)
        {
            MUST_PASS(SnapshotOneThread((*piTISM)->first));
        }

        // move on to next thread;
        (*piTISM)++;
    }

    SNAPSHOT_ALLTHREADS_EXIT:

    delete piTISM;

    // Increment the count of the semaphore.
    m_semaphore.Release();

    return hr;
}


/*
 *  Performs a DoStackSnapshot on a thread and compares the result to the threads Shadow Stack. No
 *  synchronization objects in this code as it is only called from inside protected code.
 *
 *  NULL threadId means to sample the current thread
 */
HRESULT DoStackSnapshot::SnapshotOneThread(ThreadID threadId)
{
    HRESULT hr = S_OK;
    ThreadInfoAndStacks *pThreadInfo = NULL;

    if (threadId == 0)
    {
        pThreadInfo = t_infoStacks;

        if (pThreadInfo == 0)
        {
            return S_OK;
        }
    }
    else
    {
        // Find the thread info in map
        ThreadInfoStackMap::iterator iTISM = m_threadInfoStackMap.find(threadId);
        if (iTISM == m_threadInfoStackMap.end())
        {
            FAILURE(L"Thread not found in map.\n");
            return E_FAIL;
        }

        // Get the thread info structure
        pThreadInfo = iTISM->second;
        if (pThreadInfo == NULL)
        {
            FAILURE(L"NULL ThreadInfoAndStacks found for thread.\n");
            return E_FAIL;
        }
    }

    // Clear the Snapshot and Ebp Stack
    pThreadInfo->snapStack.clear();
    pThreadInfo->ebpStack.clear();
    

    // Reset counter so we know when first callback is in order to grab the ShadowStack
    if ((m_callMethod == CURRENT_THREAD) || (m_sampleMethod == HIJACK))
    {
        m_bFirstDSSCallBack = FALSE;
    }
    else
    {
        m_bFirstDSSCallBack = TRUE;
        // clear destination shadow stack first.
        m_tsThreadInfo.shadowStack.clear();

        // Make sure we pre-allocate enough memory to avoid doing an allocation during the SnapShotCallback.
        // If we allocate during the callback we can deadlock if the thread we are inspecting is holding the
        // nt heap lock.  Default to 100 frames unless the shadow stack is above that.
        if (pThreadInfo->shadowStack.size() < 100)
            m_tsThreadInfo.shadowStack.reserve(100);
        else
            m_tsThreadInfo.shadowStack.reserve(pThreadInfo->shadowStack.size() + 1);
    }

    if (threadId == 0)
    {
        MUST_PASS(PINFO->GetCurrentThreadID(&m_tCurrentDSSThread))
    }
    else
    {
        m_tCurrentDSSThread = threadId;
    }

#ifdef THREAD_HIJACK
    if (m_sampleMethod == HIJACK)
    {
        m_bEbpStack = false;
        hr = PINFO->DoStackSnapshot(m_tCurrentDSSThread,
                                    &DoStackSnapshotStackSnapShotCallbackWrapper,
                                    COR_PRF_SNAPSHOT_REGISTER_CONTEXT,
                                    pThreadInfo,
                                    m_threadContext,
                                    sizeof(CONTEXT));
        if (SUCCEEDED(hr) && m_bEbpWalking)
        {
            m_bEbpStack = true;
            hr = PINFO->DoStackSnapshot(m_tCurrentDSSThread,
                                        &DoStackSnapshotEbpCallbackWrapper,
                                        COR_PRF_SNAPSHOT_REGISTER_CONTEXT|COR_PRF_SNAPSHOT_X86_OPTIMIZED,
                                        pThreadInfo,
                                        m_threadContext,
                                        sizeof(CONTEXT));

        }
    }
    else 
#endif // THREAD_HIJACK
    if (m_sampleMethod == NO_ASYNCH && m_callMethod == CURRENT_THREAD)
    {
        m_bEbpStack = false;
        hr = PINFO->DoStackSnapshot(threadId,
                                    &DoStackSnapshotStackSnapShotCallbackWrapper,
                                    COR_PRF_SNAPSHOT_REGISTER_CONTEXT,
                                    pThreadInfo,
                                    NULL,
                                    NULL);
        if (SUCCEEDED(hr) && m_bEbpWalking)
        {
            m_bEbpStack = true;
            hr = PINFO->DoStackSnapshot(threadId,
                                        &DoStackSnapshotEbpCallbackWrapper,
                                        COR_PRF_SNAPSHOT_REGISTER_CONTEXT|COR_PRF_SNAPSHOT_X86_OPTIMIZED,
                                        pThreadInfo,
                                        NULL,
                                        NULL);
        }
            
    }
    else
    {
        if (m_sampleMethod == SAMPLE_AND_TEST_DEADLOCK)
        {
            // Take the global heap lock before suspending the thread
            // This makes sure we always take the global heap lock with
            // deadlock here
            // We'll call DoStackSnapshot later and we shouldn't deadlock
            if (!m_lockHeapThread.LockHeap())
            {
                FAILURE(L"LockHeapThread::LockHeap() failed.");
                hr = E_FAIL;
                goto next;
            }
        }

        //
        // Note: This SuspendThread call may lead to early termination problem in CoreCLR 
        //
        // if the following events are happening:
        // 1. Managed thread A calls malloc/free and holds on to the CRT internal heap lock 
        // (not the OS heap lock), which is the 4th in the array and is actually a 
        // critical section
        // 2. Walker thread suspend thread A. A got suspended while holding the lock
        // 3. OS starts to shutdown (Host call ExitProcess)
        // 4. Walker thread gets terminated and didn't get a chance to resume the thread A
        // 5. OS in the main thread calls ProcessFlsData which calls into CRT and CRT 
        // calls free and try to take the internal heap lock
        // 6. OS sees that the internal heap lock is already taken by another thread and 
        // tries to wait
        // 7. Before wait, OS see that we are shutting down and will call TerminateProcess 
        // because the wait could lead to dead lock
        // 8. The process gets terminated before the test could do verification (in DllMain)
        // 
        // If the above didn't happen, the process shutdown will continue and call 
        // DllMain(DLL_PROCESS_DETACH) to the profiler (test) and it will do whatever 
        // verification that is necessary. Then CoreCLR gets shutdown when its own 
        // DllMain(DLL_PROCESS_DETACH) arrives and shutdown CLR with EEShutdown(TRUE). 
        // In this case there are also no ICorProfilerCallback::Shutdown notification
        //
        // Root cause: We don't have ICorProfilerCallback::Shutdown notification in CoreCLR
        // but our test needs a shutdown callback to do verification.
        //
        // For more details, see TFS Dev11 #37142
        //

        // DWORD dwRet = SuspendThread(pThreadInfo->threadHandle);
        // if (dwRet == -1)
        //  goto next;
        
        if (m_bEbpWalking)
        {
            m_bEbpStack = true;
            hr = PINFO->DoStackSnapshot(threadId,
                                        &DoStackSnapshotEbpCallbackWrapper,
                                        COR_PRF_SNAPSHOT_REGISTER_CONTEXT|COR_PRF_SNAPSHOT_X86_OPTIMIZED,
                                        pThreadInfo,
                                        NULL,
                                        NULL);
        }
        else
        {
            m_bEbpStack = false;
            hr = PINFO->DoStackSnapshot(threadId,
                                    &DoStackSnapshotStackSnapShotCallbackWrapper,
                                    COR_PRF_SNAPSHOT_REGISTER_CONTEXT,
                                    pThreadInfo,
                                    NULL,
                                    NULL);
        }

        // ResumeThread(pThreadInfo->threadHandle);

        if (m_sampleMethod == SAMPLE_AND_TEST_DEADLOCK)
        {
            // Release the heap lock
            if (!m_lockHeapThread.UnlockHeap())
            {
                FAILURE(L"LockHeapThread::UnlockHeap() failed.");
                hr = E_FAIL;
            }
        }
    }

next:

#ifndef _X86_
    if (hr == E_INVALIDARG)
    {
        m_ulInvalidArg++;
        if (!m_bEbpWalking)
        {
            FAILURE(L"E_INVALIDARG returned on NON-X86 Machine with default walkiing.");    
            return hr;
        }
        else
        {
            m_ulEbpOnNonX86++;
            return S_OK;
        }
    }
#endif

    // Check return value of DoStackSnapshot
    if (hr == CORPROF_E_STACKSNAPSHOT_UNMANAGED_CTX)
    {
        m_ulUnmanaged++;

        // This should not happen, we can not Hijack unless we are in managed code
        if (m_sampleMethod == HIJACK)
        {
            m_ulFailure++;
            FAILURE(L"CORPROF_E_STACKSNAPSHOT_UNMANAGED_CTX returned from DoStackSnapshot in Hijack case!");
            return E_FAIL;
        }

        return S_OK;
    }
    else if (hr == CORPROF_E_STACKSNAPSHOT_UNSAFE)
    {
        m_ulUnsafe++;
        return S_OK;
    }
    // Aborted is returned when the user or runtime abort the walk.  This is not a failure conditions\
    // even in the cases where we do not do the abort.
    else if (hr == CORPROF_E_STACKSNAPSHOT_ABORTED)
    {
        m_ulAborted++;
        return S_OK;
    }
    else if (FAILED(hr))
    {
        // Check to see if there were any managed frames on the shadow stack
        if ((m_callMethod == CURRENT_THREAD) || (m_sampleMethod == HIJACK))
        {
            if (pThreadInfo->shadowStack.size() == 0)
            {
                // No managed frames yet.  That is why we received E_FAIL.  Oh well.
                m_ulEFailEmptySnapshot++;
                return S_OK;
            }
        }
        else
        {
            if (m_tsThreadInfo.shadowStack.size() == 0)
            {
                // No managed frames yet.  That is why we received E_FAIL.  Oh well.
                m_ulEFailEmptySnapshot++;
                return S_OK;
            }
        }

        // A real Snapshot failre.
        m_ulFailure++;
        FAILURE(L"DoStackSnapshot return hr=" << HEX(hr));
        return hr;
    }

    // reverse snapshot so it matches shadowstack
    reverse (pThreadInfo->snapStack.begin(), pThreadInfo->snapStack.end());
    reverse (pThreadInfo->ebpStack.begin(), pThreadInfo->ebpStack.end());
    //reverse (pThreadInfo->snapStackRegInfo.begin(), pThreadInfo->snapStackRegInfo.end());


    // If we made it this far, DoStackSnapshot returned S_OK.  Now make sure the Snapshot stack we built is 
    // the same as the Shadow stack of the tread.
    if ((m_callMethod == CURRENT_THREAD) || (m_sampleMethod == HIJACK))
    {
        hr = CompareStacks(threadId,
                           &pThreadInfo->shadowStack,
                           &pThreadInfo->snapStack,
                           &pThreadInfo->ebpStack,
                           NULL, //&pThreadInfo->snapStackRegInfo,
                           &pThreadInfo->functionMap);
    }
    else
    {
        hr = CompareStacks(threadId,
                           &m_tsThreadInfo.shadowStack,
                           &pThreadInfo->snapStack,
                           &pThreadInfo->ebpStack,
                           NULL, //&pThreadInfo->snapStackRegInfo,
                           &pThreadInfo->functionMap);
    }
    if (FAILED(hr))
    {
        FAILURE(L"Stack comparison failed!\n");
        m_ulFailure++;
    }
    // DoStackSnapshot returned S_OK, and our Shadow Stack/Snapshot Stack comparison returned S_OK as well.
    // The only check left is to see if we had frames on the stack.
    else
    {
        if (pThreadInfo->snapStack.size() == 0)
        {
            m_ulSOKEmptySnapshot++;
        }
        if (pThreadInfo->shadowStack.size() == 0)
        {
            m_ulSOKEmptyShadow++;
        }
        if (pThreadInfo->ebpStack.size() == 0)
        {
            m_ulSOKEmptyEbp++;
        }
        m_ulSuccess++;
    }

    return hr;
}


/*
 *  Call DoStackSnapshot on the thread and tally the availablity of the return value.
 */
HRESULT DoStackSnapshot::MeasureDSSAvailability(ThreadID threadId)
{
    m_bEbpStack = false;    

    HRESULT hr = PINFO->DoStackSnapshot(threadId,
                                    &DoStackSnapshotStackSnapShotCallbackWrapper,
                                    COR_PRF_SNAPSHOT_DEFAULT,
                                    NULL,
                                    NULL,
                                    NULL);
    if (m_bEbpWalking)
    {
        m_bEbpStack = true;
        hr = PINFO->DoStackSnapshot(threadId,
                                    &DoStackSnapshotEbpCallbackWrapper,
                                    COR_PRF_SNAPSHOT_DEFAULT|COR_PRF_SNAPSHOT_X86_OPTIMIZED,
                                    NULL,
                                    NULL,
                                    NULL);
    }

#ifndef _X86_
    if (hr == E_INVALIDARG)
    {
        m_ulInvalidArg++;
        if (!m_bEbpWalking)
        {
            FAILURE(L"E_INVALIDARG returned on NON-X86 Machine with default walkiing.");    
            return hr;
        }
        else
        {
            m_ulEbpOnNonX86++;
            return S_OK;
        }
    }
#endif

    // Check return value of DoStackSnapshot
    if (hr == S_OK)
    {
        m_ulSuccess++;
    }
    else if (hr == E_FAIL)
    {
        m_ulFailure++;
    }
    else if (hr == CORPROF_E_STACKSNAPSHOT_ABORTED)
    {
        m_ulAborted++;
    }
    else if (hr == CORPROF_E_STACKSNAPSHOT_UNMANAGED_CTX)
    {
        m_ulUnmanaged++;
    }
    else if (hr == CORPROF_E_STACKSNAPSHOT_UNSAFE)
    {
        m_ulUnsafe++;
    }
    else
    {
        FAILURE(L"DoStackSnapshot returned unexpected HR=" << HEX(hr));
    }

    return hr;
}


/*
 *  Callback routine for DoStackSnapshot
 */
HRESULT DoStackSnapshot::StackSnapshotCallback(FunctionID funcId,
                                               UINT_PTR /*ip*/,
                                               COR_PRF_FRAME_INFO /*frameInfo*/,
                                               ULONG32 contextSize,
                                               BYTE context[],
                                               void *clientData)
{
    // If we are measuring availability we don't build a snapshot stack, or bother receiving all the
    // callbacks.  Simply return E_FAIL so no more callbacks for this stack will be delivered
    if ((m_sampleMethod == ASYNC_AVAILABILITY) || (m_callMethod == SYNC_ASP))
    {
        return E_FAIL;
    }

    // If this is the first DoStackSnapshot callback, the target thread is suspended by the runtime. Copy the
    // current Shadow Stack of the suspended thread to compare it to the Snapshot stack.
    if (m_bFirstDSSCallBack == TRUE)
    {
        m_bFirstDSSCallBack = FALSE;

        // Grab the threads shadow stack
        if (!GetThreadInfoAndStack(m_tCurrentDSSThread, &m_tsThreadInfo))
        {
            IPrfCom *pPrfCom = m_pPrfCom;
            if (pPrfCom != NULL)
            {
                FAILURE(L"Failure. GetFunctionStackForThread returned FALSE.\n");
            }
            return FALSE;
        }
    }

    // If the FunctionID is not 0, it is a managed function.  Add it to the Snapshot stack.
    if (funcId != 0)
    {
        ThreadInfoAndStacks *pThreadInfo = (ThreadInfoAndStacks *)clientData;
        FunctionIDAndRegisterInfo info;
        info.funcId = funcId;
#ifdef THREAD_HIJACK
#if defined(_X86_)
        info.EIP = ((CONTEXT *)&(context[0]))->Eip;
        info.ESP = ((CONTEXT *)&(context[0]))->Esp;
        info.EBP = ((CONTEXT *)&(context[0]))->Ebp;
#elif defined(_IA64_)
        info.StIIP = ((CONTEXT *)&(context[0]))->StIIP;
        //pFuncIdAndRegInfo->StSP = ((CONTEXT *)&(context[0]))->StSP;
#elif defined(_AMD64_)
        info.Rip = ((CONTEXT *)&(context[0]))->Rip;
        info.Rsp = ((CONTEXT *)&(context[0]))->Rsp;
#endif
#endif // THREAD_HIJACK
        pThreadInfo->snapStack.push_back(info);
        /*

        // Keeping this blob around for future debugging work.  If this is uncommented this can deadlock
        // since it allocates.  A better solution is to add this info to the structure stored in the
        // snapStack that was already pre-allocated.

                FunctionIDAndRegisterInfo *pFuncIdAndRegInfo = new FunctionIDAndRegisterInfo();
                pFuncIdAndRegInfo->funcId = funcId;

                #if defined(_X86_)
                    pFuncIdAndRegInfo->EIP = ((CONTEXT *)&(context[0]))->Eip;
                    pFuncIdAndRegInfo->ESP = ((CONTEXT *)&(context[0]))->Esp;

                #elif defined(_IA64_)
                    pFuncIdAndRegInfo->StIIP = ((CONTEXT *)&(context[0]))->StIIP;
                    //pFuncIdAndRegInfo->StSP = ((CONTEXT *)&(context[0]))->StSP;

                #elif defined(_AMD64_)
                    pFuncIdAndRegInfo->Rip = ((CONTEXT *)&(context[0]))->Rip;
                    pFuncIdAndRegInfo->Rsp= ((CONTEXT *)&(context[0]))->Rsp;
                #endif

                pThreadInfo->snapStackRegInfo.push_back(pFuncIdAndRegInfo);
        */
    }

    return S_OK;
}

HRESULT DoStackSnapshot::StackEbpCallback(FunctionID funcId,
                                               UINT_PTR /*ip*/,
                                               COR_PRF_FRAME_INFO /*frameInfo*/,
                                               ULONG32 contextSize,
                                               BYTE context[],
                                               void *clientData)
{
    // If we are measuring availability we don't build a snapshot stack, or bother receiving all the
    // callbacks.  Simply return E_FAIL so no more callbacks for this stack will be delivered
    if ((m_sampleMethod == ASYNC_AVAILABILITY) || (m_callMethod == SYNC_ASP))
    {
        return E_FAIL;
    }

    // If this is the first DoStackSnapshot callback, the target thread is suspended by the runtime. Copy the
    // current Shadow Stack of the suspended thread to compare it to the Snapshot stack.
    if (m_bFirstDSSCallBack == TRUE)
    {
        m_bFirstDSSCallBack = FALSE;

        // Grab the threads shadow stack
        if (!GetThreadInfoAndStack(m_tCurrentDSSThread, &m_tsThreadInfo))
        {
            IPrfCom *pPrfCom = m_pPrfCom;
            if (pPrfCom != NULL)
            {
                FAILURE(L"Failure. GetFunctionStackForThread returned FALSE.\n");
            }
            return FALSE;
        }
    }

    // If the FunctionID is not 0, it is a managed function.  Add it to the Snapshot stack.
    if (funcId != 0)
    {
        ThreadInfoAndStacks *pThreadInfo = (ThreadInfoAndStacks *)clientData;
        FunctionIDAndRegisterInfo info;
        info.funcId = funcId;
#ifdef THREAD_HIJACK
#if defined(_X86_)
        info.EIP = ((CONTEXT *)&(context[0]))->Eip;
        info.ESP = ((CONTEXT *)&(context[0]))->Esp;
        info.EBP = ((CONTEXT *)&(context[0]))->Ebp;
#elif defined(_IA64_)
                    info.StIIP = ((CONTEXT *)&(context[0]))->StIIP;
                    //pFuncIdAndRegInfo->StSP = ((CONTEXT *)&(context[0]))->StSP;
#elif defined(_AMD64_)
                    info.Rip = ((CONTEXT *)&(context[0]))->Rip;
                    info.Rsp = ((CONTEXT *)&(context[0]))->Rsp;
#endif
#endif // THREAD_HIJACK
        pThreadInfo->ebpStack.push_back(info);

        /*

        // Keeping this blob around for future debugging work.  If this is uncommented this can deadlock
        // since it allocates.  A better solution is to add this info to the structure stored in the
        // snapStack that was already pre-allocated.

                FunctionIDAndRegisterInfo *pFuncIdAndRegInfo = new FunctionIDAndRegisterInfo();
                pFuncIdAndRegInfo->funcId = funcId;

                #if defined(_X86_)
                    pFuncIdAndRegInfo->EIP = ((CONTEXT *)&(context[0]))->Eip;
                    pFuncIdAndRegInfo->ESP = ((CONTEXT *)&(context[0]))->Esp;

                #elif defined(_IA64_)
                    pFuncIdAndRegInfo->StIIP = ((CONTEXT *)&(context[0]))->StIIP;
                    //pFuncIdAndRegInfo->StSP = ((CONTEXT *)&(context[0]))->StSP;

                #elif defined(_AMD64_)
                    pFuncIdAndRegInfo->Rip = ((CONTEXT *)&(context[0]))->Rip;
                    pFuncIdAndRegInfo->Rsp= ((CONTEXT *)&(context[0]))->Rsp;
                #endif

                pThreadInfo->snapStackRegInfo.push_back(pFuncIdAndRegInfo);
        */
    }

    return S_OK;
}

/*
 *  Called from the first SnapshotCallback call in order to get a copy of the thread's shadow stack and
 *  functionID map if needed.  Since it is called from the DSS callback, the thread is suspended so there is
 *  no need for synchronization.
 */
BOOL DoStackSnapshot::GetThreadInfoAndStack(ThreadID threadId, ThreadInfoAndStacks *threadInfo)
{
    // look to see if thread already exists in map
    ThreadInfoStackMap::iterator iTISM = m_threadInfoStackMap.find(threadId);

    if(iTISM == m_threadInfoStackMap.end())
    {
        FAILURE(L"ERROR: Asynchronous profiler asked for ShadowStack of non-existent thread, ThreadID " << threadId);
        return FALSE;
    }

    // Get the ThreadInfoAndStacks data structure allocated for this thread.
    ThreadInfoAndStacks *pThreadInfo = iTISM->second;
    if (pThreadInfo == NULL)
    {
        FAILURE(L"NULL FunctionStack for ThreadID " << threadId);
        return FALSE;
    }

    // copy current shadow stack into pre-allocated temp shadow stack
    threadInfo->shadowStack = pThreadInfo->shadowStack;

    return TRUE;
}


HRESULT DoStackSnapshot::CompareShadowAndSnapshotStacks(ThreadID threadId,
                                                        FunctionStack *shadowStack,
                                                        FunctionAndRegStack *snapshotStack,
                                                        FunctionAndRegStack *snapshotRegInfoStack,
                                                        ThreadFunctionMap *functionMap)
{
    INT shadowSize = (INT) shadowStack->size();
    INT snapshotSize = (INT) snapshotStack->size();

    HRESULT hr = S_OK;

    BOOL errorDiffStacks = TRUE;

    if (abs(shadowSize - snapshotSize) <= 1)
    {
        // Off by one.  This is ok as the callback for the odd frame may be blocked by the profiler.
        INT minIndx = min(shadowSize, snapshotSize);

        BOOL diffStacks = FALSE;
        for (INT i = 0; i < minIndx; i++)
        {
            if (shadowStack->at(i) != snapshotStack->at(i))
            {
                diffStacks = TRUE;
                ++m_uNotMatched;
            }
            else
            {
                ++m_uMatched;
            }
        }
        // If the stacks are the same up to the smallest stack, call it good.
        errorDiffStacks = diffStacks;
    }

    // Off by one, but still successful
    if (errorDiffStacks == FALSE)
    {           
        hr =  S_OK;
    }
    // Either the stacks are off by more than one, or there are different frames in the off by one 
    // stacks.
    else
    {
        hr = E_FAIL;
    }
    return hr;
}

HRESULT DoStackSnapshot::CompareShadowAndEbpStacks(ThreadID threadId,
                                                        FunctionStack *shadowStack,
                                                        FunctionAndRegStack *ebpStack,
                                                        FunctionAndRegStack *snapshotRegInfoStack,
                                                        ThreadFunctionMap *functionMap)
{
    INT shadowSize = (INT) shadowStack->size();
    INT ebpSize = (INT) ebpStack->size();

    HRESULT hr = S_OK;

    INT i = 0;
    INT j = 0;
    while (i<shadowSize && j<ebpSize)
    {
        if (shadowStack->at(i) != ebpStack->at(j))
        {
            ++i;
            ++m_uNotMatched;
        }
        else
        {
            ++i;
            ++j;
            ++m_uMatched;
        }
    }
    if (i>=shadowSize && j<ebpSize-1)
    {
        DISPLAY(L" CompareShadowAndEbpStacks: ");
        DISPLAY(L" " << i << " >= " << shadowSize <<" && " <<  j << " < " << ebpSize-1);
        hr =  E_FAIL;
    }
    else
    {
        hr = S_OK;
    }
    return hr;
}

HRESULT DoStackSnapshot::CompareSnapshotAndEbpStacks(ThreadID threadId,
                                                     FunctionAndRegStack *snapStack,
                                                     FunctionAndRegStack *ebpStack,
                                                     FunctionAndRegStack *snapshotRegInfoStack,
                                                     ThreadFunctionMap *functionMap)
{
    INT snapSize = (INT) snapStack->size();
    INT ebpSize = (INT) ebpStack->size();

    HRESULT hr = S_OK;

    INT i = 0;
    INT j = 0;
    while (i<snapSize && j<ebpSize)
    {
        if (snapStack->at(i) != ebpStack->at(j))
        {
            ++i;
            ++m_uNotMatched;
        }
        else
        {
            ++i;
            ++j;
            ++m_uMatched;
        }
    }
    if (i>=snapSize && j<ebpSize)
    {
        DISPLAY(L" CompareSnapshotAndEbpStacks ");
        DISPLAY(L" "<<  i << " >= " << snapSize <<" && " <<  j << " < " << ebpSize);
        hr =  E_FAIL;
    }
    else
    {
        hr = S_OK;
    }
    return hr;
}


HRESULT DoStackSnapshot::DisplayStacks(ThreadID threadId,
                                       FunctionStack *shadowStack,
                                       FunctionAndRegStack *snapshotStack,
                                       FunctionAndRegStack *ebpStack,
                                       FunctionAndRegStack *snapshotRegInfoStack,
                                       ThreadFunctionMap *functionMap)
{
    INT shadowSize = (INT) shadowStack->size();
    INT snapshotSize = (INT) snapshotStack->size();
    INT ebpSize = (INT) ebpStack->size();

    DWORD win32ThreadID;
    PINFO->GetThreadInfo(threadId, &win32ThreadID);

    DISPLAY(L"\nThreadID " << threadId << L", Win32 " << win32ThreadID);
    DISPLAY(L"Shadow Stack has " << shadowSize << L" frames, Snapshot Stack has " << snapshotSize << L" frames.");

    INT i;
    FunctionID funcId;

    DISPLAY(L"\nShadow Stack: " << shadowSize);

    for (i = shadowSize - 1; i >= 0; i--)
    {
        funcId = shadowStack->at(i);
        ThreadFunctionMap::iterator iTFM = functionMap->find(funcId);
        if (iTFM == functionMap->end())
        {
            DISPLAY(i << " : " << funcId << L" - Function not yet in function table.");
        }
        else
        {
            DISPLAY(i << " : " << funcId << L" - " << iTFM->second->funcName);
        }
    }

    DISPLAY(L"\nSnapshot Stack: (FunctionId Functionname EIP ESP EBP) stack size:" << snapshotSize);
    for (i = snapshotSize - 1; i >= 0; i--)
    {
        funcId = snapshotStack->at(i).funcId;

        if (snapshotRegInfoStack != NULL)
        {
            FunctionIDAndRegisterInfo pFuncIdAndRegInfo = snapshotRegInfoStack->at(i);
#ifdef THREAD_HIJACK
            #if defined(_X86_)
                DISPLAY(L"EIP "   << HEX(pFuncIdAndRegInfo.EIP));
            #elif defined(_IA64_)
                DISPLAY(L"StIIP " << HEX(pFuncIdAndRegInfo.StIIP));
            #elif defined(_AMD64_)
                DISPLAY(L"Rip "   << HEX(pFuncIdAndRegInfo.Rip));
            #endif
#endif // THREAD_HIJACK
        }

        ThreadFunctionMap::iterator iTFM = functionMap->find(funcId);
        if (iTFM == functionMap->end())
        {
            DISPLAY(i << " : " << funcId << L" - Function not yet in function table.");
        }
        else
        {
#if defined(_X86_)
            DISPLAY(i << " : " << funcId << L" - " << iTFM->second->funcName << " " << HEX(snapshotStack->at(i).EIP) << " " << HEX(snapshotStack->at(i).ESP) << " "<< HEX(snapshotStack->at(i).EBP) );
#else
            DISPLAY(i << " : " << funcId << L" - " << iTFM->second->funcName);
#endif
        }
    }

    if (m_bEbpWalking)
    {
        DISPLAY(L"\nEbp Stack: (FunctionId Functionname EIP ESP EBP) stack size:" << ebpSize);
        for (i = ebpSize - 1; i >= 0; i--)
        {
            funcId = ebpStack->at(i).funcId;
        
            if (snapshotRegInfoStack != NULL)
            {
                FunctionIDAndRegisterInfo FuncIdAndRegInfo = snapshotRegInfoStack->at(i);
#ifdef THREAD_HIJACK
#if defined(_X86_)
                DISPLAY(L"EIP "   << HEX(FuncIdAndRegInfo.EIP));
#elif defined(_IA64_)
                DISPLAY(L"StIIP " << HEX(FuncIdAndRegInfo.StIIP));
#elif defined(_AMD64_)
                DISPLAY(L"Rip "   << HEX(FuncIdAndRegInfo.Rip));
#endif
#endif // THREAD_HIJACK
            }
            
            ThreadFunctionMap::iterator iTFM = functionMap->find(funcId);
            if (iTFM == functionMap->end())
            {
                DISPLAY(i << " : " << funcId << L" - Function not yet in function table.");
            }
            else
            {
#if defined(_X86_)
                DISPLAY(i << " : " << funcId << L" - " << iTFM->second->funcName << " " << HEX(ebpStack->at(i).EIP) << " " << HEX(ebpStack->at(i).ESP) << " "<< HEX(ebpStack->at(i).EBP) );
#else
                DISPLAY(i << " : " << funcId << L" - " << iTFM->second->funcName);
#endif
            }
        }
        
    }
    DISPLAY(L"\n");
    return S_OK;
}

HRESULT DoStackSnapshot::CompareStacks(ThreadID threadId,
                                       FunctionStack *shadowStack,
                                       FunctionAndRegStack *snapshotStack,
                                       FunctionAndRegStack *ebpStack,
                                       FunctionAndRegStack *snapshotRegInfoStack,
                                       ThreadFunctionMap *functionMap)
{
    HRESULT hr;
    
    m_uSnapshotFrames += (ULONG)snapshotStack->size();
    m_uShadowFrames += (ULONG)shadowStack->size();
    m_uEbpFrames += (ULONG)ebpStack->size();
    
    if (m_bEbpWalking)
    {
        HRESULT hr1;
        
        hr = CompareShadowAndEbpStacks(threadId,
                                       shadowStack,
                                       ebpStack,
                                       NULL, //&pThreadInfo->snapStackRegInfo,
                                       functionMap);
        if ((m_sampleMethod == HIJACK) || (m_sampleMethod == NO_ASYNCH && m_callMethod == CURRENT_THREAD))//compare snapshot and ebp only in SYNCSELF and HIJACK
        {
            hr1 = CompareSnapshotAndEbpStacks(threadId,
                                       snapshotStack,
                                       ebpStack,
                                       NULL, //&pThreadInfo->snapStackRegInfo,
                                       functionMap);
        if (FAILED(hr1))
            hr = hr1;
        }
    }
    else
    {
        hr = CompareShadowAndSnapshotStacks(threadId,
                                            shadowStack,
                                            snapshotStack,
                                            NULL, //&pThreadInfo->snapStackRegInfo,
                                            functionMap); 
    }
    if (FAILED(hr))
    {
        // Failure!
        DisplayStacks(threadId, shadowStack, snapshotStack, ebpStack, snapshotRegInfoStack, functionMap);
        FAILURE(L"Shadow Stack and Snapshot Stacks are different.  Look at output for details\n");
    }
    return hr;
}

/*
 *  Verify the results of this run.
 */
HRESULT DoStackSnapshot::Verify()
{
    if (m_bEbpWalking)
    {
        DISPLAY("EBP Stack Walking.");
    }

    // If we started an asynchronous thread, stop that thread from sampling.
    if (m_sampleMethod != NO_ASYNCH)
    {
        // Only check for == 0 here. >0 case will be covered by the check code in RemoveFunctionFromShadowStack    
        if (gdwSeenMainMethod == 0)
        {
            FAILURE(L"HACK: Haven't seen any static Main method. Please fix the hack for this test case.");
            return E_FAIL;
        }
        
        StopSampling();
    }

    // Wait on semaphore with an infinite time out.  We need control and it is safe to block from here.
    // Wait for the semaphore or the 
    m_semaphore.Take();

    // Make sure no threads still have frames on them at shutdown.
    for (ThreadInfoStackMap::iterator iTISM = m_threadInfoStackMap.begin(); iTISM != m_threadInfoStackMap.end(); )
    {
        ThreadInfoAndStacks *pFunctionInfo = iTISM->second;

        // If a thread still has a shadow stack, display the shadow stack before raising the failure dialog.
        if (pFunctionInfo->shadowStack.empty() != TRUE)
        {
            DISPLAY(L"Thread " << iTISM->first << L" still has frames on it.");
            for (INT i = (INT)pFunctionInfo->shadowStack.size() - 1; i >= 0; i--)
            {
                FunctionID funcId = pFunctionInfo->shadowStack.at(i);
                ThreadFunctionMap::iterator iTFM = pFunctionInfo->functionMap.find(funcId);
                if (iTFM == pFunctionInfo->functionMap.end())
                {
                    DISPLAY(funcId << L" - Function not in function table.");
                }
                else
                {
                    DISPLAY(funcId << L" - " << iTFM->second->funcName);
                }
            }

            DISPLAY(L"Did we expect an abnormal termination here?  Why did we exit with active frames?  ASP?\n" );
        }

        // Clean up STL containters
        pFunctionInfo->shadowStack.clear();
        pFunctionInfo->snapStack.clear();
        pFunctionInfo->ebpStack.clear();
        pFunctionInfo->functionMap.clear();
        

        // Close open handles for this thread
#ifdef THREAD_HIJACK
        CloseHandle(pFunctionInfo->threadHandle);
#endif // THREAD_HIJACK

        // Leak this allocated memory - we have to deal with abnormal shutdown which can cause RtlHeapFree calls to AV.
        // The memory the heap allocated is still accesible, it appears only heap bookkeeping datastructures
        // were released. Calling delete here would trigger RtlHeapFree, so rather than kindly trying
        // to clean up we just let the memory leak. 
        // A better fix to this might be to avoid verification during DllUnload... its a sketchy time to try doing
        // a bunch of work. Instead we might explicitly call some cleanup routine from the profilee managed code as
        // it exits. That would take a good bit more work though, and I see no reason that leaking memory at shutdown
        // will cause problems.
        //delete pFunctionInfo;
        pFunctionInfo = NULL;

        // Remove entry from Map
        iTISM = m_threadInfoStackMap.erase(iTISM);
    }

    ULONG totalDSSCalls =  m_ulAborted +
                           m_ulFailure +
                           m_ulSOKEmptySnapshot +
                           m_ulSOKEmptyShadow +
                           m_ulSOKEmptyEbp +
                           m_ulEFailEmptySnapshot +
                           m_ulSuccess +
                           m_ulUnmanaged +
                           m_ulUnsafe +
                           m_ulWaitForSingleObject;

    DISPLAY(L"  Totally " << m_uMatched << " frames matched."  );
    DISPLAY(L"  Totally " << m_uNotMatched << " frames NOT matched."  );
    DISPLAY(L"  Totally " << m_uDssLockThread << " threads locked.");

    DISPLAY(L"  Totally " << m_uSnapshotFrames << " snapshot frames.");
    DISPLAY(L"  Totally " << m_uShadowFrames << " shadow frames.");
    DISPLAY(L"  Totally " << m_uEbpFrames << " ebp frames.");

    // Check result of availability run
    if ((m_sampleMethod == ASYNC_AVAILABILITY) || (m_callMethod == SYNC_ASP))
    {
        DOUBLE successRate   = 0.0;
        DOUBLE failureRate   = 0.0;
        DOUBLE unmanagedRate = 0.0;
        DOUBLE unsafeRate    = 0.0;

        if (totalDSSCalls != 0)
        {
            // Success rate = (aborted + success)/total
            successRate = ((DOUBLE)(m_ulFailure + m_ulSOKEmptySnapshot + m_ulEFailEmptySnapshot + m_ulAborted + m_ulSuccess) / (DOUBLE)totalDSSCalls) * 100.0;

            // Failure rate = (failure + unmanaged + unsafe + waitforso)/total
            failureRate  = ((DOUBLE)(m_ulUnmanaged + m_ulUnsafe + m_ulWaitForSingleObject) / (DOUBLE)totalDSSCalls) * 100.0;

            // Unmanaged failure rate
            unmanagedRate = ((DOUBLE)m_ulUnmanaged/(DOUBLE)totalDSSCalls) * 100.0;

            // Unsafe failure rate
            unsafeRate = ((DOUBLE)m_ulUnsafe/(DOUBLE)totalDSSCalls) * 100.0;
        }

        DISPLAY(L"\n*******************");
        DISPLAY(L"Sample Ticks = " << DEC << m_ulSampleTicks);
        DISPLAY(L"DSS Calls = " << totalDSSCalls);
        DISPLAY(L"SUCCESS = " << FLT << successRate);
        DISPLAY(L"FAILURE = " << failureRate );
        DISPLAY(L"  Unmgd = " << unmanagedRate);
        DISPLAY(L"  Unsafe = " << unsafeRate);
        DISPLAY(L"*******************");

        DISPLAY(L"\n" << m_ulSuccess   << L" Successful stack trace comparisons.");
        DISPLAY(m_ulFailure   << L" Failed stack trace comparisons (Probably Empty Stacks)");
        DISPLAY(m_ulAborted   << L" Aborted stack trace comparisons. (This is done deliberitly)");
        DISPLAY(m_ulUnmanaged << L" Attempts at unmanaged IP points.");
        DISPLAY(m_ulUnsafe    << L" Attempts at unsafe IP points.");

        if (m_callMethod != SYNC_ASP)
        {
            if (unmanagedRate > m_ulMinAvail)
            {
                FAILURE(L"Unmanaged HR returned " << FLT << unmanagedRate << L" percent > max allowed " << DEC << m_ulMinAvail << L" percent.\nFAILURE.");
                return E_FAIL;
            }
            else if (unsafeRate > (DOUBLE)1.0)
            {
                FAILURE(L"Unsafe HR returned " << FLT << unsafeRate << L" percent > max allowed 1.0 percent.\nFAILURE.");
                return E_FAIL;
            }
            else
            {
                DISPLAY(L"\nAvailablity Rates OK\n");
            }
        }

        return S_OK;

    }

    // Check result of run for the case where we were doing asynchronous sampling
    else if (m_sampleMethod != NO_ASYNCH)
    {
        DISPLAY(L"\n" << m_ulSuccess              << L" Successful stack trace comparisons.");
        DISPLAY(L"Empty stacks: S_OK = " << m_ulSOKEmptySnapshot << L", E_FAIL = " << m_ulEFailEmptySnapshot);
        DISPLAY(m_ulFailure              << L" Failed stack trace comparisons.");
        DISPLAY(m_ulAborted              << L" Aborted stack trace comparisons.");
        DISPLAY(m_ulUnmanaged            << L" Attempts at unmanaged IP points.");
        DISPLAY(m_ulUnsafe               << L" Attempts at unsafe IP points.");
        DISPLAY(m_ulHijackAbortUnmanaged << L" Hijacks attempted at unmanaged IP.");
        DISPLAY(m_ulWaitForSingleObject  << L" Hijacks lost, WaitForSingleObject timed out.");

        //?? For Whidbey Beta2, we have not bar for Availability of DoStackSnapshot calls.  So a successful
        //?? run is one that does not AV, does not cause any runtime asserts, and does not encounter a
        //?? successful DoStackSnapshot call that returned a stack trace the differs from our shadow stack.
        if ((m_ulFailure > 0)) // || (m_ulSuccess < 1))
            return E_FAIL;
        else
            return S_OK;
    }

    // Check result of run for the case where we were calling dss synchronously
    else
    {
        DISPLAY(L"\n" << m_ulSuccess          << L" Successful stack trace comparisons.");
        DISPLAY(L"Empty stacks: S_OK = " << m_ulSOKEmptySnapshot << L", E_FAIL = " << m_ulEFailEmptySnapshot << L".");
        DISPLAY(m_ulFailure          << L" Failed stack trace comparisons.");
        DISPLAY(m_ulAborted          << L" Aborted stack trace comparisons.");
        DISPLAY(m_ulSemaphoreAbandon << L" Attempts abandoned by semaphore.");
        DISPLAY(m_ulUnmanaged        << L" Attempts at unmanaged IP points.");
        DISPLAY(m_ulUnsafe           << L" Attempts at unsafe IP points.");

        //?? For Whidbey Beta2, we have not bar for Availability of DoStackSnapshot calls.  So a successful
        //?? run is one that does not AV, does not cause any runtime asserts, and does not encounter a
        //?? successful DoStackSnapshot call that returned a stack trace the differs from our shadow stack.
        if ((m_ulFailure > 0)) // || (m_pPrfAsynchronous->success < 1))
            return E_FAIL;
        else
            return S_OK;
    }
}

/*
 *  Straight function callbacks used by TestProfiler
 */

/*
 *  Verification routine called by TestProfiler
 */
HRESULT DoStackSnapshot_Verify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(DoStackSnapshot);
    HRESULT hr = pDoStackSnapshot->Verify();

    // Clean up instance of test class
    delete pDoStackSnapshot;
    pPrfCom->m_pTestClassInstance = NULL;

    return hr;
}


/*
 *  Initialization routine called by TestProfiler
 */
void DSS_Initialize(IPrfCom *pPrfCom,
                    PMODULEMETHODTABLE pModuleMethodTable,
                    SynchronousCallMethod requestedCallMethod,
                    AsynchronousSampleMethod requestedSampleMethod
                    )
{
    IPrfCom *m_pPrfCom = pPrfCom;

    // Establish our minimum Unmanaged failure rate
    ULONG minAvail = 0;
    if (requestedSampleMethod == ASYNC_AVAILABILITY)
    {
        wstring control = ReadEnvironmentVariable(L"PROFILER_TEST_MIN_AVAIL");
        if(control.size() > 0)
        {
            minAvail = (ULONG)std::stoi(control);
        }
    }

    // Create instance of test class
    SET_CLASS_POINTER(new DoStackSnapshot(pPrfCom, requestedCallMethod, requestedSampleMethod, minAvail));

    // The minimum information that all tests need to know is thread created/destroyed.  And all tests need 
    // to enable DSS.  For the asynchronous availability tests, this is all they need.
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_THREADS |
                                COR_PRF_ENABLE_STACK_SNAPSHOT|
                                COR_PRF_MONITOR_MODULE_LOADS;

    REGISTER_CALLBACK(VERIFY, DoStackSnapshot_Verify);
    REGISTER_CALLBACK(THREADCREATED, DoStackSnapshot::ThreadCreatedWrapper);
    REGISTER_CALLBACK(THREADDESTROYED, DoStackSnapshot::ThreadDestroyedWrapper);
    REGISTER_CALLBACK(MODULELOADFINISHED, DoStackSnapshot::ModuleLoadFinishedWrapper);

    // Everyone other than Asynch Availability needs to know Enter/Leave/Tailcall, and Exceptions to keep the
    // ShadowStack up to date.  Also set up Thread Local Storage.
    if ((requestedSampleMethod != ASYNC_AVAILABILITY) && (requestedCallMethod != SYNC_ASP))
    {
        pModuleMethodTable->FLAGS |= COR_PRF_MONITOR_ENTERLEAVE     |
                                     COR_PRF_ENABLE_FUNCTION_ARGS   |
                                     COR_PRF_ENABLE_FUNCTION_RETVAL |
                                     COR_PRF_ENABLE_FRAME_INFO      |
                                     COR_PRF_DISABLE_INLINING       |
                                     COR_PRF_MONITOR_EXCEPTIONS     |
                                     COR_PRF_USE_PROFILE_IMAGES;

        REGISTER_CALLBACK(FUNCTIONENTER3WITHINFO,       DoStackSnapshot::FunctionEnter3WithInfoWrapper);        
        REGISTER_CALLBACK(FUNCTIONLEAVE3WITHINFO,       DoStackSnapshot::FunctionLeave3WithInfoWrapper);
        REGISTER_CALLBACK(FUNCTIONTAILCALL3WITHINFO,    DoStackSnapshot::FunctionTailCall3WithInfoWrapper);
        REGISTER_CALLBACK(EXCEPTIONSEARCHFILTERENTER,   DoStackSnapshot::ExceptionSearchFilterEnterWrapper);
        REGISTER_CALLBACK(EXCEPTIONSEARCHFILTERLEAVE,   DoStackSnapshot::ExceptionSearchFilterLeaveWrapper);
        REGISTER_CALLBACK(EXCEPTIONUNWINDFUNCTIONLEAVE, DoStackSnapshot::ExceptionUnwindFunctionLeaveWrapper);
    }

    // If Asynchronous is not being tested, use additional callbacks for testing Synchronous same thread and 
    // cross thread.
    if(requestedSampleMethod == NO_ASYNCH)
    {
        // Sanity check that we were not called with nothing to test
        if (requestedCallMethod == NO_SYNCH)
        {
            FAILURE(L"DoStackSnapshot_Initialize called with NO_ASYNCH & NO_SYNCH!");
        }

        pModuleMethodTable->FLAGS |= COR_PRF_MONITOR_ASSEMBLY_LOADS   |
                                     COR_PRF_MONITOR_CACHE_SEARCHES   |
                                     COR_PRF_MONITOR_CLASS_LOADS      |
                                     COR_PRF_MONITOR_CODE_TRANSITIONS |
                                     COR_PRF_MONITOR_GC               |
                                     COR_PRF_MONITOR_JIT_COMPILATION  |
                                     COR_PRF_MONITOR_MODULE_LOADS     |
                                     COR_PRF_MONITOR_OBJECT_ALLOCATED |
                                     COR_PRF_ENABLE_OBJECT_ALLOCATED;

        // Since we are doing the same work for all of these ICorProfilerCallback callbacks, have 
        // TestProfiler call the same function for each of these events, InvokeDoStackSnapshotWrapper.
        // We can get away with casting InvokeDoStackSnapshotWrapper to all these other function pointers
        // since we do not touch the function arguments in this testing.
        REGISTER_CALLBACK(ASSEMBLYLOADSTARTED,              DoStackSnapshot::AssemblyLoadStartedWrapper);
        REGISTER_CALLBACK(CLASSLOADSTARTED,                 DoStackSnapshot::ClassLoadStartedWrapper);
        REGISTER_CALLBACK(EXCEPTIONTHROWN,                  DoStackSnapshot::ExceptionThrownWrapper);
        REGISTER_CALLBACK(GARBAGECOLLECTIONSTARTED,         DoStackSnapshot::GarbageCollectionStartedWrapper);
        REGISTER_CALLBACK(HANDLECREATED,                    DoStackSnapshot::HandleCreatedWrapper);
        REGISTER_CALLBACK(HANDLEDESTROYED,                  DoStackSnapshot::HandleDestroyedWrapper);
        REGISTER_CALLBACK(JITCACHEDFUNCTIONSEARCHSTARTED,   DoStackSnapshot::JITCachedFunctionSearchStartedWrapper);
        REGISTER_CALLBACK(JITCOMPILATIONSTARTED,            DoStackSnapshot::JITCompilationStartedWrapper);
        REGISTER_CALLBACK(MANAGEDTOUNMANAGEDTRANSITION,     DoStackSnapshot::ManagedToUnmanagedTransitionWrapper);
        REGISTER_CALLBACK(MODULELOADSTARTED,                DoStackSnapshot::ModuleLoadStartedWrapper);
        REGISTER_CALLBACK(OBJECTALLOCATED,                  DoStackSnapshot::ObjectAllocatedWrapper);
        REGISTER_CALLBACK(UNMANAGEDTOMANAGEDTRANSITION,     DoStackSnapshot::UnmanagedToManagedTransitionWrapper);
    }

    // Hijacking profilers also need to know about suspensions to synchronize their hijacks and the GC 
    // hijacks.
    
    if ((requestedSampleMethod == HIJACK) || (requestedSampleMethod == HIJACKANDINSPECT))
    {
        pModuleMethodTable->FLAGS |= COR_PRF_MONITOR_SUSPENDS;

        REGISTER_CALLBACK(RUNTIMESUSPENDSTARTED, DoStackSnapshot::RuntimeSuspendStarted);
        REGISTER_CALLBACK(RUNTIMESUSPENDABORTED, DoStackSnapshot::RuntimeResumeStarted);
        REGISTER_CALLBACK(RUNTIMERESUMEFINISHED, DoStackSnapshot::RuntimeResumeStarted);
    }
    
    reinterpret_cast<DoStackSnapshot *>(pModuleMethodTable->TEST_POINTER)->m_dwEventMask = pModuleMethodTable->FLAGS;

    // If we have an asynchronous thread waiting to sample, we start that now.
    if (requestedSampleMethod != NO_ASYNCH)
    {
        reinterpret_cast<DoStackSnapshot *>(pModuleMethodTable->TEST_POINTER)->StartSampling();
    }

    return;
}

/*
 *  Static function used passed to DoStackSnapshot.  This invokes our DSS callback in the DoStackSnapshot
 *  class.
 */
HRESULT __stdcall DoStackSnapshotStackSnapShotCallbackWrapper(
                    FunctionID funcId,
                    UINT_PTR ip,
                    COR_PRF_FRAME_INFO frameInfo,
                    ULONG32 contextSize,
                    BYTE context[],
                    void *clientData)
{
    return DoStackSnapshot::Instance()->StackSnapshotCallback(funcId,
                                                              ip,
                                                              frameInfo,
                                                              contextSize,
                                                              context,
                                                              clientData);
}
HRESULT __stdcall DoStackSnapshotEbpCallbackWrapper(
                    FunctionID funcId,
                    UINT_PTR ip,
                    COR_PRF_FRAME_INFO frameInfo,
                    ULONG32 contextSize,
                    BYTE context[],
                    void *clientData)
{
    return DoStackSnapshot::Instance()->StackEbpCallback(funcId,
                                                         ip,
                                                         frameInfo,
                                                         contextSize,
                                                         context,
                                                         clientData);
}
