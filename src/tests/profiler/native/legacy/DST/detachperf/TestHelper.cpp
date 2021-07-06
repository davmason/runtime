// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "common.hxx"
#include "ThreadMgr.h"
#include "TestHelper.h"


TestHelper::TestHelper()
:   m_pThrMgr           (NULL),
    m_cThreads          (0)
{
    srand((unsigned)GetTickCount());

    // how many threads to spawn?
    while (0 == m_cThreads) {
        // continue to loop till we get a non-zero random number
        m_cThreads = rand();

        // we want to limit threads to THREAD_LIMIT
        if (m_cThreads > THREADS_MAX_LIMIT) {
            m_cThreads %= THREADS_MAX_LIMIT;
        }
    }
}


TestHelper::~TestHelper()
{
    if (NULL != m_pThrMgr) {
        delete m_pThrMgr;
        m_pThrMgr = NULL;
    }
}




//*********************************************************************
//  Descripton: Spawns a pool of worker threads, each of which issues
//              a profiler detach request.
//  
//  Params:     [IN] ICorProfilerInfo3 * pInfo - The interface via 
//                  which the detach request is made.
//              BOOL bCreateSuspended - Should the worker threads be
//                  created in a suspended mode initially. If yes, then
//                  the caller is required to call TestHelper::
//                  ResumeWorkerThreads at a later stage.
//              BOOL bWaitForThreadExit - Should the ThreadMgr object
//                  block, waiting for the worker threads to exit?
//              [IN] void * pParams - Parameters to be passed onto
//                  each of the worker threads. Can be NULL.
//
//  Return:     S_OK if method successful, else appropriate error code.
//  
//  Notes:      The RequestProfilerDetach can return various error 
//              codes which can then be verified by the caller via
//              TestHelper::VerifyDetachResults().  
//*********************************************************************

HRESULT TestHelper::DetachFromMultipleThreads(ICorProfilerInfo3 * pInfo, 
    BOOL bCreateSuspended, BOOL bWaitForThreadExit, void * pParams)
{
    TRACEENTRY(L"TestHelper::DetachFromMultipleThreads")

    /////////////////////////////////////////////////////

    HRESULT hr = S_OK;
    DWORD dwRet = 0;
    ThreadParams thrParams = {0};


    // pre-condition check
    if (NULL == pInfo) {
        hr = E_INVALIDARG;
        TRACEERROR(hr, L"Invalid arg passed into DetachPerfTester::Initialize -> pInfo");
        goto ErrReturn;    
    }    
    if (NULL == pParams) {
        hr = E_INVALIDARG;
        TRACEERROR(hr, L"Invalid arg passed into DetachPerfTester::Initialize -> pParams");
        goto ErrReturn;    
    }    

    // create a thread manager
    m_pThrMgr = new ThreadMgr(m_cThreads);
    if (NULL == m_pThrMgr) {
        hr = E_OUTOFMEMORY;
        TRACEERROR(hr, L"Failed to allocate memory for thread manager");                
        goto ErrReturn;
    }    

    // spawn the threads 
    hr = m_pThrMgr->SpawnThreads(
                        bCreateSuspended, 
                        bCreateSuspended ? FALSE : bWaitForThreadExit, 
                        300 * 10000, // 5 mins is a reasonable timeout
                        pParams, 
                        TestHelper::WorkerThreadFunc);

    TRACENORMAL(L"Spawning %d worker threads ...", m_cThreads);

    if (S_OK != hr) {
        TRACEERROR(hr, L"ThreadManager::SpawnThreads");
        goto ErrReturn;
    }

ErrReturn:
    return hr;
}



HRESULT TestHelper::ResumeWorkerThreads()
{
    TRACEENTRY(L"TestHelper::ResumeWorkerThreads")

    /////////////////////////////////////////////////////

    return m_pThrMgr->ResumeThreads();
}



HRESULT TestHelper::ForceGCRuntimeSuspension(void * p)
{
    TRACEENTRY(L"TestHelper::ForceGCRuntimeSuspension")

    /////////////////////////////////////////////////////

    HRESULT hr = S_OK;
    DWORD dwRetval = 0;
    DWORD dwThreadExitCode = 0;
    HANDLE hForceGCThread;

    // initialize the worker thread which forces GC runtime suspensions
    hForceGCThread = (HANDLE) _beginthreadex(
                                        NULL,
                                        0,
                                        & ForceGCFunc,
                                        (void *) p,
                                        0,
                                        NULL);

    if (NULL == hForceGCThread && INVALID_HANDLE_VALUE != hForceGCThread) {
        hr = E_FAIL;
        TRACEERROR(hr, "Failed to spawn Runtime suspension thread");
        goto ErrReturn;
    }        

    // A timeout of 60 seconds is more than reasonable
    dwRetval = WaitForSingleObject(hForceGCThread, 1000 * 60);

    if (WAIT_OBJECT_0 != dwRetval) {

        if (WAIT_TIMEOUT == dwRetval) {
            hr = (unsigned) HRESULT_FROM_WIN32(WAIT_TIMEOUT);
        }
        else {
            hr = (unsigned) HRESULT_FROM_WIN32(GetLastError());
        }

        TRACEERROR(hr, L"Kernel32!WaitForSingleObject");
        goto ErrReturn;
    }

    // retrieve the exit code
    dwRetval = GetExitCodeThread(hForceGCThread, & dwThreadExitCode);

    if (0 == dwRetval) {
        // we failed to get a thread exit code for some reason.
        hr = HRESULT_FROM_WIN32(GetLastError());
        TRACEERROR(hr, L"Kernel32!GetExitCodeThread");
        goto ErrReturn;
    }
                
    hr = (HRESULT)dwThreadExitCode;
    

ErrReturn:

    if (NULL != hForceGCThread) {
        CloseHandle(hForceGCThread);
        hForceGCThread = NULL;
    } 

    return hr;   
}



//*********************************************************************
//  Descripton: This purpose of this method is to call the
//              ICorProfilerInfo3::RequestProfilerDetach API after
//              some pre-determined time-delay (which is specified
//              via the params passed in).
//  
//  Params:     void * p - Parameters passed to the thread.  The
//                  method expects a valid "ThreadParams" struct to be
//                  passed in else the outcome of this method is
//                  unpredictable.        
//
//  Return:     We'll try to pass the return value from
//              ICorProfilerInfo3::RequestProfilerDetach as this 
//              thread's exit code. This means S_OK if method is 
//              successful, else the appropriate error code.    
//  
//  Notes:      Even though this is a re-entrant method, we are not
//              guarding this with critical sections etc since no
//              global variables are being accessed. All vars here are 
//              local stack variables.
//*********************************************************************

unsigned __stdcall TestHelper::WorkerThreadFunc(void * p)
{
    TRACEENTRY(L"TestHelper::WorkerThreadFunc. [Thread id = 0x%X]", GetCurrentThreadId())

    /////////////////////////////////////////////////////

    unsigned ulRet = 0;
    HANDLE hTimer = NULL;
    LARGE_INTEGER liDueTime;
    ICorProfilerInfo3 * pInfo = ((ThreadParams *)p)->pInfo;
    DWORD dwTimeOut = ((ThreadParams *)p)->dwTimeOut;
    static int cDetachSucceeded = 0;

    // create the waitable timer object
    TRACENORMAL(L"Setting Wait-time to %d secs", dwTimeOut)

/*
    hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
    if (NULL == hTimer) {
        ulRet = (unsigned) HRESULT_FROM_WIN32(GetLastError());
        TRACEERROR(ulRet, L"Kernel32!CreateWaitableTimer");
        goto ErrReturn;
    }
    
    // set the timer to wait for specified time
    liDueTime.QuadPart= -10000000 * dwTimeOut;
    ulRet = SetWaitableTimer(
                hTimer,
                (const LARGE_INTEGER*) & liDueTime,
                0, // signal only once
                NULL,
                NULL,
                0);
    if (0 == ulRet) {
        ulRet = (unsigned) HRESULT_FROM_WIN32(GetLastError());
        TRACEERROR(ulRet, L"Kernel32!SetWaitableTimer");
        goto ErrReturn;
    }            

    //wait for the timer to elapse
    ulRet = WaitForSingleObject(hTimer, dwTimeOut * 2); // safety factor of 2
    if (WAIT_OBJECT_0 != ulRet) {
        if (WAIT_TIMEOUT == ulRet) {
            ulRet = (unsigned) HRESULT_FROM_WIN32(WAIT_TIMEOUT);
        }
        else {
            ulRet = (unsigned) HRESULT_FROM_WIN32(GetLastError());
        }
        TRACEERROR(ulRet, L"Kernel32!WaitForSingleObject");
        goto ErrReturn;
    }        
*/

    Sleep(1000 * dwTimeOut);
    
    // Requesting a detach now. If runtime thread is still  
    // stuck in a Initialize* callback, then we'll get a return
    // value of CORPROF_E_RUNTIME_UNINITIALIZED. If runtime thread
    // is stuck in any other (legal) callback or not in any 
    // callback at all, then only the first request for detach gets
    // an S_OK, while other requests will be met with a 
    // CORPROF_E_PROFILER_DETACHING error code. However we are not
    // explicitly checking the return values here, we pass it along 
    // as the thread exit code to the caller (which is responsible
    // for the verification).
    ulRet = pInfo->RequestProfilerDetach(2000);
    
    if (S_OK == ulRet || CORPROF_E_PROFILER_DETACHING == ulRet)
    {
        TRACENORMAL(L"RequestProfilerDetach returns 0x%X. [Thread id = 0x%X]", ulRet, GetCurrentThreadId());
    }
    else
    {
        TRACEERROR2(L"RequestProfilerDetach returns 0x%X. [Thread id = 0x%X]", ulRet, GetCurrentThreadId());
    }
    
    /////////////////////////////////////////////////////

ErrReturn:

    return ulRet;        
}


//*********************************************************************
//  Descripton: This purpose of this method is to call the
//              ICorProfilerInfo::ForceGC API and trigger the runtime
//              to suspend execution.
//  
//  Params:     void * p - Parameters passed to the thread.  The
//                  method expects a valid "ThreadParams" struct to be
//                  passed in else the outcome of this method is
//                  unpredictable.        
//
//  Return:     We'll try to pass the return value from
//              ICorProfilerInfo::ForceGC as this thread's exit code.
//              This means S_OK if method is successful, else the 
//              appropriate error code.    
//  
//  Notes:      This method is always called in a worker thread and
//              never on the main profiler (callback) thread.    
//*********************************************************************

unsigned __stdcall TestHelper::ForceGCFunc(void * p)
{
    TRACEENTRY(L"TestHelper::ForceGCFunc")

    /////////////////////////////////////////////////////

    unsigned ulRet = 0;
    ICorProfilerInfo3 * pInfo = ((ThreadParams *)p)->pInfo;

    // Now we force the runtime suspension by triggering the GC ....
    ulRet = (unsigned) pInfo->ForceGC(); 
    if (S_OK != ulRet) {
        TRACEERROR(ulRet, L"ICorProfilerInfo!ForceGC");
        goto ErrReturn;
    }


ErrReturn:
    return ulRet;
}

///////////////////////////////////////////////////////////////////////////////
