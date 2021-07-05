// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// Prf3_SamplingProfiler.cpp
//
// ======================================================================================

#include "../../ProfilerCommon.h"
#include "simplerwlock.hpp"

#define MYDISPLAY( message ) { \
                                 IPrfCom *pPrfCom = m_pPrfCom; \
                                 DISPLAY(message); \
                             }

#define MYDISPLAY( message )     { \
                                 IPrfCom *pPrfCom = m_pPrfCom; \
                                 DISPLAY(message); \
                             }

#define MYFAILURE( message ) { \
                                 IPrfCom *pPrfCom = m_pPrfCom; \
                                 FAILURE(message); \
                             }

HRESULT __stdcall DoStackSnapshotCallbackWrapper(
    FunctionID funcId,
    UINT_PTR ip,
    COR_PRF_FRAME_INFO frameInfo,
    ULONG32 contextSize,
    BYTE context[],
    void *clientData);

DWORD WINAPI AsynchronousThreadFuncWrapper(LPVOID lpParameter);

struct ThreadInfo
{
    ThreadID m_threadId;
    HANDLE m_threadHandle;
    DWORD m_win32ThreadId;
    CRITICAL_SECTION m_crstThreadLock;
};

class SamplingProfiler
{
public:

    SamplingProfiler(IPrfCom * pPrfCom);
    ~SamplingProfiler();
    HRESULT Verify(IPrfCom * pPrfCom);

#pragma region static_wrapper_methods
    static HRESULT AssemblyLoadStartedWrapper(IPrfCom * pPrfCom,
        AssemblyID assemblyId)
    {
        return STATIC_CLASS_CALL(SamplingProfiler)->AssemblyLoadStarted(pPrfCom, assemblyId);
    }

    static HRESULT ClassLoadStartedWrapper(IPrfCom * pPrfCom,
        ClassID classId)
    {
        return STATIC_CLASS_CALL(SamplingProfiler)->ClassLoadStarted(pPrfCom, classId);
    }

    static HRESULT ModuleLoadStartedWrapper(IPrfCom * pPrfCom,
        ModuleID moduleId)
    {
        return STATIC_CLASS_CALL(SamplingProfiler)->ModuleLoadStarted(pPrfCom, moduleId);
    }

    static HRESULT ObjectAllocatedWrapper(IPrfCom * pPrfCom,
        ObjectID objectId,
        ClassID classId)
    {
        return STATIC_CLASS_CALL(SamplingProfiler)->ObjectAllocated(pPrfCom, objectId, classId);
    }

    static HRESULT ThreadCreatedWrapper(IPrfCom * pPrfCom,
        ThreadID managedThreadId)
    {
        return STATIC_CLASS_CALL(SamplingProfiler)->ThreadCreated(pPrfCom, managedThreadId);
    }

    static HRESULT ThreadDestroyedWrapper(IPrfCom * pPrfCom,
        ThreadID threadID)
    {
        return STATIC_CLASS_CALL(SamplingProfiler)->ThreadDestroyed(pPrfCom, threadID);
    }

#pragma endregion

    VOID AsynchSamplingThreadFunc();
    VOID StartSampling();
    VOID StopSampling();

    HRESULT StackSnapshotCallback(
                    FunctionID funcId,
                    UINT_PTR ip,
                    COR_PRF_FRAME_INFO frameInfo,
                    ULONG32 contextSize,
                    BYTE context[],
                    void *clientData);

    static SamplingProfiler* Instance()
    {
        return This;
    }

    static BOOL s_bAsynchThreadRunning;

private:
#pragma region callback_handler_prototypes
    HRESULT SamplingProfiler::AssemblyLoadStarted(
        IPrfCom * /* pPrfCom */ ,
        AssemblyID /* assemblyId */ );

    HRESULT SamplingProfiler::ClassLoadStarted(
        IPrfCom * /* pPrfCom */ ,
        ClassID /* classId */ );

    HRESULT SamplingProfiler::ModuleLoadStarted(
        IPrfCom * /* pPrfCom */ ,
        ModuleID /* moduleId */ );

    HRESULT SamplingProfiler::ObjectAllocated(
        IPrfCom * /* pPrfCom */ ,
        ObjectID /* objectId */ ,
        ClassID /* classId */ );

    HRESULT SamplingProfiler::ThreadCreated(
        IPrfCom * /* pPrfCom */ ,
        ThreadID /* managedThreadId */ );

    HRESULT SamplingProfiler::ThreadDestroyed(
        IPrfCom * /* pPrfCom */ ,
        ThreadID /* threadID */ );

#pragma endregion

    IPrfCom *m_pPrfCom;
    static SamplingProfiler* This;

   	typedef std::map<ThreadID, ThreadInfo *> ThreadMap;
	ThreadMap m_threadMap;
    SimpleRWLock *m_prwlThreadMapLock;

    HANDLE GetHandleForThread(ThreadID threadId);
    VOID AddThreadToList(ThreadID threadId);
    VOID RemoveThreadFromList(ThreadID threadId);

    HRESULT SnapshotOneThread(ThreadInfo *threadInfo);
    HRESULT SamplingProfiler::InterestingEventCallback(IPrfCom * pPrfCom);

    LARGE_INTEGER m_liSampleDueTime;
    DWORD m_dwAysnchThreadId;
    HANDLE m_hAsyncThread;
    BOOL m_bInitializedTLS;
    BOOL m_bInitializedStressLog;

    // Total Count of Aynchronous WaitObject Events
    #define ASYNCHRONOUS_EVENTS 2
    /* Event 0 = Timer Event for Asynch Sample */
    #define ASYNCHRONOUS_TIMER 0
    /* Event 1 = Signal to Asynch Thread to Stop */
    #define ASYNCHRONOUS_STOP 1

    HANDLE m_hEvents[ASYNCHRONOUS_EVENTS];

    ULONG m_ulDSSTotalCalls;
    ULONG m_ulDSSSuccess;
    ULONG m_ulDSSUnmanaged;
    ULONG m_ulDSSUnsafe;
    ULONG m_ulDSSAborted;
    ULONG m_ulDSSFailed;
    ULONG m_ulDSSOtherHr;
    ULONG m_ulDSSEFail;

    ULONG m_ulSuspendThreadFailed;
};

SamplingProfiler* SamplingProfiler::This = NULL;
BOOL SamplingProfiler::s_bAsynchThreadRunning = FALSE;

/*
*  Initialize the SamplingProfiler class.  
*/
SamplingProfiler::SamplingProfiler(IPrfCom * pPrfCom)
{
    SamplingProfiler::This = this;

    this->m_pPrfCom = pPrfCom;

    m_bInitializedTLS = FALSE;
    m_bInitializedStressLog = FALSE;
    m_ulDSSTotalCalls = 0;
    m_ulDSSUnmanaged = 0;
    m_ulDSSUnsafe = 0;
    m_ulDSSAborted = 0;
    m_ulDSSEFail = 0;
    m_ulDSSFailed = 0;
    m_ulDSSSuccess = 0;
    m_ulDSSOtherHr = 0;

    m_ulSuspendThreadFailed = 0;

    // Create the SimpleRWLock that will be used for locking ThreadMap
    m_prwlThreadMapLock = new SimpleRWLock();

    // Create the Timer for the Asynch Sampling Thread
    m_hEvents[ASYNCHRONOUS_TIMER] = CreateWaitableTimer(NULL, FALSE, NULL);
    if (!m_hEvents[ASYNCHRONOUS_TIMER])
    {
        FAILURE(L"CreateWaitableTimer failed" << GetLastError());
        return;
    }

    // Create the Event that will be used to signal the asynch thread to stop
    m_hEvents[ASYNCHRONOUS_STOP] = CreateEvent(NULL,
        FALSE,
        FALSE,
        NULL);
    if (!m_hEvents[ASYNCHRONOUS_STOP])
    {
        FAILURE(L"CreateEvent failed for AsynchronousStopEvent " << GetLastError());
        return;
    }

    // Create the asynchronous sampling thread and set it to highest priority
    m_hAsyncThread = CreateThread(
        NULL,
        0,
        (LPTHREAD_START_ROUTINE)&AsynchronousThreadFuncWrapper,
        NULL,
        0,
        &m_dwAysnchThreadId);
    if (NULL == m_hAsyncThread)
    {
        FAILURE(L"CreateThread failed to create the Asynchronous thread");
    }
    if (!SetThreadPriority(m_hAsyncThread, THREAD_PRIORITY_HIGHEST))
    {
        FAILURE(L"SetThreadPriority failed trying to set the Async thread to THREAD_PRIORITY_HIGHEST");
    }
}

/*
*  Clean up
*/
SamplingProfiler::~SamplingProfiler()
{
    if (m_hEvents[ASYNCHRONOUS_TIMER])
        CloseHandle(m_hEvents[ASYNCHRONOUS_TIMER]);
    if (m_hEvents[ASYNCHRONOUS_STOP])
        CloseHandle(m_hEvents[ASYNCHRONOUS_STOP]);
    if (m_hAsyncThread)
        CloseHandle(m_hAsyncThread);
}

HRESULT SamplingProfiler::AssemblyLoadStarted(
    IPrfCom * pPrfCom,
    AssemblyID /* assemblyId */ )
{
    this->InterestingEventCallback(pPrfCom);
    return S_OK;
}

HRESULT SamplingProfiler::ClassLoadStarted(
    IPrfCom * pPrfCom,
    ClassID /* classId */ )
{
    this->InterestingEventCallback(pPrfCom);
    return S_OK;
}

HRESULT SamplingProfiler::ModuleLoadStarted(
    IPrfCom * pPrfCom,
    ModuleID /* moduleId */ )
{
    this->InterestingEventCallback(pPrfCom);
    return S_OK;
}

HRESULT SamplingProfiler::ObjectAllocated(
    IPrfCom * pPrfCom,
    ObjectID /* objectId */ ,
    ClassID /* classId */ )
{
    this->InterestingEventCallback(pPrfCom);
    return S_OK;
}

HRESULT SamplingProfiler::ThreadCreated(
                                        IPrfCom * /* pPrfCom */,
                                        ThreadID managedThreadId )
{
    AddThreadToList(managedThreadId);
    return S_OK;
}

HRESULT SamplingProfiler::ThreadDestroyed(
    IPrfCom * /* pPrfCom */ ,
    ThreadID threadId )
{
    RemoveThreadFromList(threadId);
    return S_OK;
}

HANDLE SamplingProfiler::GetHandleForThread(ThreadID threadId)
{
    HRESULT hr;
    HANDLE hTempThreadHandle;
    HANDLE hProcessHandle;
    HANDLE hReturnValue;

    hr = this->m_pPrfCom->m_pInfo->GetHandleFromThread(threadId, &hTempThreadHandle);
    if (FAILED(hr))
    {
        MYFAILURE(L"GetHandleFromThread failed for ThreadID " << threadId);
        return NULL;
    }
    hProcessHandle = GetCurrentProcess();
    if (!DuplicateHandle(hProcessHandle,
        hTempThreadHandle,
        hProcessHandle,
        &hReturnValue,
        0, 
        FALSE,
        DUPLICATE_SAME_ACCESS) )
    {
        MYFAILURE(L"DuplicateHandle failed in ThreadCreated: " << HEX(GetLastError()));
        return NULL;
    }
    return hReturnValue;
}

VOID SamplingProfiler::AddThreadToList(ThreadID threadId)
{
    if (0 == threadId)
    {
        MYFAILURE(L"AddThreadToList called with a threadId of zero");
        return;
    }

    // Acquire Writer Lock to ThreadMap
    m_prwlThreadMapLock->EnterWrite();

    // Ensure that the map doesn't already contain an entry for the
    // thread being added
    ThreadMap::iterator iTM = this->m_threadMap.find(threadId);
    if (iTM != this->m_threadMap.end())
    {
        MYFAILURE(L"A duplicate thread id has been created. Thread ID " << threadId);
    }
    else
    {
        // Create a new ThreadInfo for threadId and add it to the map
        ThreadInfo *pNewThreadInfo;
        pNewThreadInfo = new ThreadInfo();
        pNewThreadInfo->m_threadId = threadId;
        this->m_pPrfCom->m_pInfo->GetThreadInfo(threadId, &pNewThreadInfo->m_win32ThreadId);
        pNewThreadInfo->m_threadHandle = this->GetHandleForThread(threadId);
        if (NULL == pNewThreadInfo->m_threadHandle)
        {
            MYFAILURE(L"NULL thread handle. Thread's stack will be skipped during sampling.");
            delete pNewThreadInfo;
        }
        else
        {
            InitializeCriticalSection(&pNewThreadInfo->m_crstThreadLock);
            this->m_threadMap.insert(make_pair(threadId, pNewThreadInfo));
        }
    }

    // Release the Writer Lock
    m_prwlThreadMapLock->LeaveWrite();
}

VOID SamplingProfiler::RemoveThreadFromList(ThreadID threadId )
{
    if (0 == threadId)
    {
        MYFAILURE(L"RemoveThreadFromList called with a threadId of zero");
        return;
    }

    // Acquire Writer Lock to ThreadMap
    m_prwlThreadMapLock->EnterWrite();

    // Ensure that the map contains an entry for the thread being removed
    ThreadMap::iterator iTM = this->m_threadMap.find(threadId);
    if (iTM == this->m_threadMap.end())
    {
        MYFAILURE(L"RemoveThreadFromList called for a ThreadID not in the map. Thread ID " << threadId);
    }
    else
    {
        // Remove the ThreadInfo from the map, releasing resources
        ThreadInfo *pThreadInfo = iTM->second;
        CloseHandle(pThreadInfo->m_threadHandle);
        DeleteCriticalSection(&pThreadInfo->m_crstThreadLock);
        m_threadMap.erase(iTM);
        delete pThreadInfo;
    }

    // Release the Writer Lock
    m_prwlThreadMapLock->LeaveWrite();
}

HRESULT SamplingProfiler::InterestingEventCallback(IPrfCom * pPrfCom)
{
    // Get the ThreadID of the currently executing thread so that
    // we can lock the associated ThreadInfo (to prevent the asynch
    // thread from trying to walk it at the same time).
    ThreadID threadId = NULL;
    if (FAILED(pPrfCom->m_pInfo->GetCurrentThreadID(&threadId)))
    {
        DISPLAY(L"FAILED calling GetCurrentThreadID in InterestingEventCallback");
        return S_OK;
    }

    // Acquire Reader Lock to ThreadMap
    m_prwlThreadMapLock->EnterRead();

    // Ensure that the map contains an entry for the current thread
    ThreadMap::iterator iTM = this->m_threadMap.find(threadId);
    if (iTM == this->m_threadMap.end())
    {
        MYDISPLAY(L"InterestingEventCallback called for a ThreadID not in the map. Thread ID " << threadId);
    }
    else
    {
        ThreadInfo *pThreadInfo = iTM->second;

        // Acquire the CriticalSection for the ThreadInfo to force
        // serialization with the asynch DSS thread
        EnterCriticalSection(&pThreadInfo->m_crstThreadLock);

        // Snapshot the current thread
        this->SnapshotOneThread(NULL);

        // Release the CriticalSection for the ThreadInfo
        LeaveCriticalSection(&pThreadInfo->m_crstThreadLock);
    }

    // Release the Reader Lock
    m_prwlThreadMapLock->LeaveRead();

    return S_OK;
}

HRESULT SamplingProfiler::SnapshotOneThread(ThreadInfo *pThreadInfo)
{
    HRESULT hr;
    ThreadID threadId;
    BOOL bThreadSuspended = FALSE;
    BOOL bUnmanagedErrorAllowed = FALSE;
    BOOL bGetThreadContextError = FALSE;
    DWORD dwGetThreadContextError = 0;
    HRESULT hrGetFunctionFromIP = S_OK;


    if (NULL == pThreadInfo)
    {
        threadId = NULL;
    }
    else
    {
        threadId = pThreadInfo->m_threadId;
        FunctionID functionId = 0;
        if (TRUE == this->m_bInitializedStressLog)
        {
            // Do NOT SuspendThread or call PInfo functions on the first
            // Asynch call to DSS or we'll deadlock in a variety of ways.
            // Once an Asynch DSS has returned S_OK once then we can start
            // doing all the extra stuff...

            // Walking a thread asynchronously, we need to get the seed
            // Note that for this test, I'm not actually getting the seed
            // because I'm not actually going to walk the native stack.
            // But, I do still want to call GetFunctionFromIP to see if
            // the top of the stack is native or managed.

            // Step 1: Suspend the thread
            // IMPORTANT NOTE: DO NOT USE ANY OF THE LOGGING
            // METHODS (DISPLAY/FAILURE/LOG) WHILE A THREAD IS SUSPENDED
            // BECAUSE THE SUSPENDED THREAD MAY BE IN THE CRITICAL SECTION
            // IN WHICH CASE WE'RE DEAD.
            DWORD ret = SuspendThread(pThreadInfo->m_threadHandle);
            bThreadSuspended = TRUE;
            if (ret == (DWORD)-1)
            {
                m_ulSuspendThreadFailed++;
                return E_FAIL;
            }

            // Step 2: Get the CONTEXT and the IP of the top frame
            CONTEXT threadContext;
            memset(&threadContext, 0, sizeof(CONTEXT));
            threadContext.ContextFlags = CONTEXT_ALL;
            if (!GetThreadContext(pThreadInfo->m_threadHandle,  &threadContext))
            {
                bGetThreadContextError = TRUE;
                dwGetThreadContextError = GetLastError();
            }

            if (FALSE == bGetThreadContextError)
            {
                BYTE *threadIp;

#if defined(_X86_)
                DWORD dwIp = threadContext.Eip;
                threadIp = (BYTE *)dwIp;
#elif defined(_IA64_)
                threadIp = (BYTE *)((CONTEXT *)&threadContext)->StIIP;
#elif defined(_AMD64_)
                threadIp = (BYTE *)((CONTEXT *)&threadContext)->Rip;
#elif defined(_ARM_)
                threadIp = (BYTE *)((CONTEXT *)&threadContext)->Pc;
#endif

                hrGetFunctionFromIP = this->m_pPrfCom->m_pInfo->GetFunctionFromIP(threadIp, &functionId);
                if (FAILED(hrGetFunctionFromIP))
                {
                    functionId = 0;
                }
            }
        }
        if (0 == functionId)
        {
            // if GetFunctionFromIP indicates that a managed function is on the top of the stack
            // then DSS is not allowed to fail with a claim that the top function is unmanaged
            bUnmanagedErrorAllowed = TRUE;
        }
    }

    hr = this->m_pPrfCom->m_pInfo->DoStackSnapshot(
        threadId,
        &DoStackSnapshotCallbackWrapper,
        COR_PRF_SNAPSHOT_REGISTER_CONTEXT,
        NULL,
        NULL,
        NULL);

    if (TRUE == bThreadSuspended)
    {
        // The thread was suspended before calling DSS, resume it
        ResumeThread(pThreadInfo->m_threadHandle);

        // Check to see if we have an error message from earlier to display that
        // we couldn't safely write out while another thread was suspended.
        if (bGetThreadContextError)
        {
            MYDISPLAY(L"GetThreadContext returned " << dwGetThreadContextError << " for thread " << pThreadInfo->m_threadId
                << L", Handle " << pThreadInfo->m_threadHandle);
        }
        if (FAILED(hrGetFunctionFromIP) && hrGetFunctionFromIP != E_FAIL)
        {
            MYDISPLAY(L"GetFunctionFromIP failed with HRESULT " << HEX(hrGetFunctionFromIP));
        }
    }

    m_ulDSSTotalCalls++;

    // Check return value of DoStackSnapshot
    if (hr == CORPROF_E_STACKSNAPSHOT_UNMANAGED_CTX)
    {
        if (FALSE == bUnmanagedErrorAllowed)
        {
            MYFAILURE(L"DSS returned CORPROF_E_STACKSNAPSHOT_UNMANAGED_CTX with a managed frame on the top of the stack");
        }
        m_ulDSSUnmanaged++;
    }
    else if (hr == CORPROF_E_STACKSNAPSHOT_UNSAFE)
    {
        m_ulDSSUnsafe++;
    }
    else if (hr == CORPROF_E_STACKSNAPSHOT_ABORTED)
    {
        m_ulDSSAborted++;
    }
    else if (hr == E_FAIL)
    {
        m_ulDSSEFail++;
    }
    else if (FAILED(hr))
    {
        m_ulDSSFailed++;
        MYDISPLAY(L"DSS Failed " << HEX(hr));
    }
    else if (hr == S_OK)
    {
        m_ulDSSSuccess++;
        if ((FALSE == this->m_bInitializedStressLog) && (NULL != pThreadInfo))
        {
            MYDISPLAY(L"TLS and StressLog Initialized, now safe to suspend thread around call to DSS");
            this->m_bInitializedStressLog = TRUE;
        }
    }
    else
    {
        m_ulDSSOtherHr++;
    }
    return S_OK;
}

// Asynchronous Sampling Thread
VOID SamplingProfiler::AsynchSamplingThreadFunc()
{
    MYDISPLAY(L"Asynchronous Sampling Worker Thread Starting");

    ThreadMap::iterator iTM;
    DWORD dwEvent;
    BOOL bLoop = TRUE;

    while(TRUE == bLoop)
    {
        dwEvent = WaitForMultipleObjects(
            ASYNCHRONOUS_EVENTS,
            m_hEvents,
            FALSE,
            INFINITE);

        switch(dwEvent)
        {
        // Timer Event
        case WAIT_OBJECT_0 + ASYNCHRONOUS_TIMER:

            if (FALSE == this->m_bInitializedTLS)
            {
                // Call an ICorProfilerInfo2 method before sampling any threads.
                // The first time an ICorProfilerInfo2 method is called on this
                // thread, CLR will allocate ThreadLocalStorage. If the first call
                // is made during a stack walk it the TLS will be allocated while
                // a managed thread is suspended. If that thread has the heap
                // locked then we'll deadlock.
                // Note that this call will fail but we don't care. The point is
                // just to make the introduction of our thread to the CLR so it
                // will allocate the TLS.
                ThreadID dummy;
                this->m_pPrfCom->m_pInfo->GetCurrentThreadID(&dummy);
                this->m_bInitializedTLS = TRUE;
            }

            // Acquire Reader Lock to ThreadMap
            m_prwlThreadMapLock->EnterRead();

            // Iterate over all elements of the ThreadMap
            iTM = this->m_threadMap.begin();
	        for (iTM = this->m_threadMap.begin(); iTM != this->m_threadMap.end(); iTM++ )
	        {
                ThreadInfo *pThreadInfo = iTM->second;

                // Acquire the CriticalSection for the ThreadInfo to force
                // serialization with the asynch DSS thread
                EnterCriticalSection(&pThreadInfo->m_crstThreadLock);

                // Snapshot the thread
                this->SnapshotOneThread(pThreadInfo);

                // Release the CriticalSection for the ThreadInfo
                LeaveCriticalSection(&pThreadInfo->m_crstThreadLock);
	        }

            // Release Reader Lock to ThreadMap
            m_prwlThreadMapLock->LeaveRead();

            // Reset the timer
            if (!SetWaitableTimer(m_hEvents[ASYNCHRONOUS_TIMER],
                &m_liSampleDueTime,
                0,
                NULL,
                NULL,
                0))
            {
                MYFAILURE(L"SetWaitableTimer on Asynchronous Timer failed " << GetLastError());
            }
            break;

        // Stop Event
        case WAIT_OBJECT_0 + ASYNCHRONOUS_STOP:
            bLoop = FALSE;
            break;

        case WAIT_ABANDONED_0:
            MYFAILURE(L"WaitForMultipleObjects returned WAIT_ABANDONED_0");
            break;

        case WAIT_FAILED:
            MYFAILURE(L"WaitForMultipleObjects returned WAIT_FAILED, GetLastError = " << HEX(GetLastError()));
            break;

        default:
            MYFAILURE(L"WaitForMultipleObjects returned unexpectedly: " << HEX(dwEvent));
            bLoop = FALSE;
            break;
        }
    }
}

// Start the WaitableTimer that triggers the asynchronous sampling thread
// to take a sample
VOID SamplingProfiler::StartSampling()
{
    // Start the timer (timer resolution is in 100 nSec units, i.e. 100*10-9 == 1*10-7)
    // m_liSampleDueTime.QuadPart = -20000000; // 2 seconds (2*10^7 * 1*10^-7 == 2*10^0 == 2.0)
    m_liSampleDueTime.QuadPart = -100000; // 0.01 seconds (1*10^5 * 1*10^-7 == 1*10^-2 == 0.01)
    // m_liSampleDueTime.QuadPart = -10000; // 0.001 seconds (1*10^4 * 1*10^-7 == 1*10^-3 == 0.001)

    if (!SetWaitableTimer(m_hEvents[ASYNCHRONOUS_TIMER],
        &m_liSampleDueTime,
        0,
        NULL,
        NULL,
        0))
    {
        MYFAILURE(L"SetWaitableTimer on Asynchronous Timer failed " << GetLastError());
    }
}

// Cancel the WaitableTimer and signal the asynchronous sampling thread
// to stop/exit
VOID SamplingProfiler::StopSampling()
{
    // Cancel any pending Samples
    CancelWaitableTimer(m_hEvents[ASYNCHRONOUS_TIMER]);

    // Signal Asynchronous Thread to Stop
    if (!SetEvent(m_hEvents[ASYNCHRONOUS_STOP]))
    {
        MYFAILURE(L"Failure setting stop event for asynchronous thread\n");
    }
    for (int i = 0; i < 1000; i++)
    {
        if (SamplingProfiler::s_bAsynchThreadRunning == FALSE)
            break;
        // Async Sampling thread is still running, relinquish CPU
        Sleep(0);
    }
}

/*
 *  Callback routine for DoStackSnapshot
 */
HRESULT SamplingProfiler::StackSnapshotCallback(FunctionID /*funcId*/,
                                               UINT_PTR /*ip*/,
                                               COR_PRF_FRAME_INFO /*frameInfo*/,
                                               ULONG32 /*contextSize*/,
                                               BYTE[] /*context[]*/,
                                               void * /* clientData */ )
{
    return S_OK;
}

/*
*  Verification
*/
HRESULT SamplingProfiler::Verify(IPrfCom * pPrfCom)
{
    StopSampling();
    DISPLAY(L"This stress profiler does not perform verification so it implicitly passed!");
    DISPLAY(L"");
    DISPLAY(L"m_ulDSSTotalCalls " << m_ulDSSTotalCalls);
    DISPLAY(L"m_ulDSSSuccess    " << m_ulDSSSuccess);
    DISPLAY(L"m_ulDSSUnmanaged  " << m_ulDSSUnmanaged);
    DISPLAY(L"m_ulDSSUnsafe     " << m_ulDSSUnsafe);
    DISPLAY(L"m_ulDSSAborted    " << m_ulDSSAborted);
    DISPLAY(L"m_ulDSSEFail      " << m_ulDSSEFail);
    DISPLAY(L"m_ulDSSFailed     " << m_ulDSSFailed);
    DISPLAY(L"m_ulDSSOtherHr    " << m_ulDSSOtherHr);
    DISPLAY(L"Failed Suspends   " << m_ulDSSOtherHr);
    DISPLAY(L"");
    if (FALSE == this->m_bInitializedStressLog)
    {
        DISPLAY(L"Asynch DSS never returned S_OK!");
    }
    return S_OK;
}

/*
*  Verification routine called by TestProfiler
*/
HRESULT SamplingProfiler_Verify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(SamplingProfiler);
    HRESULT hr = pSamplingProfiler->Verify(pPrfCom);

    // Clean up instance of test class
    if (SamplingProfiler::s_bAsynchThreadRunning == TRUE)
    {
        DISPLAY(L"AsyncSampling Thread is still running! Deleting the class instance will likely cause an AV!");
    }
    delete pSamplingProfiler;
    pPrfCom->m_pTestClassInstance = NULL;

    return hr;
}

/*
*  Initialization routine called by TestProfiler
*/

void SamplingProfiler_Initialize(IPrfCom * pPrfCom,
                                 PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize SamplingProfiler.\n");

    // Create and save an instance of test class
    // pModuleMethodTable->TEST_POINTER is passed into every callback as pPrfCom->m_privatePtr
    SamplingProfiler *pSamplingProfiler = new SamplingProfiler(pPrfCom);
    SET_CLASS_POINTER(pSamplingProfiler);

    // Turn off callback counting to reduce synchronization/serialization in TestProfiler
    // It isn't needed since we aren't using the counts anyway.
    pPrfCom->m_fEnableCallbackCounting = FALSE;

    // Initialize MethodTable - we want to all the callbacks of a typical sampling profiler
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_THREADS |
        COR_PRF_ENABLE_STACK_SNAPSHOT    |
        COR_PRF_MONITOR_ASSEMBLY_LOADS   |
        COR_PRF_MONITOR_CLASS_LOADS      |
        COR_PRF_MONITOR_MODULE_LOADS     |
        COR_PRF_MONITOR_OBJECT_ALLOCATED |
        COR_PRF_ENABLE_OBJECT_ALLOCATED;

    REGISTER_CALLBACK(ASSEMBLYLOADSTARTED, SamplingProfiler::AssemblyLoadStartedWrapper);
    REGISTER_CALLBACK(CLASSLOADSTARTED, SamplingProfiler::ClassLoadStartedWrapper);
    REGISTER_CALLBACK(MODULELOADSTARTED, SamplingProfiler::ModuleLoadStartedWrapper);
    REGISTER_CALLBACK(OBJECTALLOCATED, SamplingProfiler::ObjectAllocatedWrapper);
    REGISTER_CALLBACK(THREADCREATED, SamplingProfiler::ThreadCreatedWrapper);
    REGISTER_CALLBACK(THREADDESTROYED, SamplingProfiler::ThreadDestroyedWrapper);
    REGISTER_CALLBACK(VERIFY, SamplingProfiler_Verify);

    pSamplingProfiler->StartSampling();

    return;
}

/*
*  Static function passed to DoStackSnapshot which invokes our DSS callback.
*/
HRESULT __stdcall DoStackSnapshotCallbackWrapper(
    FunctionID funcId,
    UINT_PTR ip,
    COR_PRF_FRAME_INFO frameInfo,
    ULONG32 contextSize,
    BYTE context[],
    void *clientData)
{
    return SamplingProfiler::Instance()->StackSnapshotCallback(
        funcId,
        ip,
        frameInfo,
        contextSize,
        context,
        clientData);
}

DWORD WINAPI AsynchronousThreadFuncWrapper(LPVOID /* lpParameter */)
{
    SamplingProfiler::s_bAsynchThreadRunning = TRUE;
    SamplingProfiler::Instance()->AsynchSamplingThreadFunc();
    SamplingProfiler::s_bAsynchThreadRunning = FALSE;
    return 0;
}

