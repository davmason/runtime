// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "common.hxx"
#include "DetachPerfTester.h"
#include "TestHelper.h"
#include "ThreadMgr.h"
#include "COMGoo.h"


// Set the global configurations
DWORD g_Verbosity = TRACE_ERROR;


///////////////////////////////////////////////////////////////////////////////


DetachPerfTester::DetachPerfTester() 
:   m_cRef              (1),
    m_pThrParams        (NULL),
    m_pInfo             (NULL),
    m_detachWaitTime    (0),
    m_hAsyncThread      (NULL)    
{
    m_pThrParams = new ThreadParams();    
    
    wchar_t wszVerbosityOptions[MAX_PATH] = {0};

    // check the env-var TRACE_NORMAL
    if (0 != GetEnvironmentVariable(ENVVAR_TRACE_NORMAL, wszVerbosityOptions, MAX_PATH))
        g_Verbosity |= TRACE_NORMAL;

    ZeroMemory((PVOID) wszVerbosityOptions, (SIZE_T) MAX_PATH);
    
    // check the env-var TRACE_ENTRY
    if (0 != GetEnvironmentVariable(ENVVAR_TRACE_ENTRY, wszVerbosityOptions, MAX_PATH))
        g_Verbosity |= TRACE_ENTRY;
        
    ZeroMemory((PVOID) wszVerbosityOptions, (SIZE_T) MAX_PATH);
    
    // check the env-var TRACE_EXIT
    if (0 != GetEnvironmentVariable(ENVVAR_TRACE_EXIT, wszVerbosityOptions, MAX_PATH))
        g_Verbosity |= TRACE_EXIT;
}


DetachPerfTester::~DetachPerfTester() 
{
    if (NULL != m_pThrParams) {
        delete m_pThrParams;
    }
    if (NULL != m_pInfo) {
        m_pInfo->Release();
        m_pInfo = NULL;
    }
}


// This is ultimately derived from IUnknown. So....

HRESULT __stdcall DetachPerfTester::QueryInterface(const IID & iid, void** ppv)
{
    if (iid == IID_IUnknown) {
        * ppv = static_cast<ICorProfilerCallback*>(this); // original implementation
    }
    else if (iid == IID_ICorProfilerCallback) {
        * ppv = static_cast<ICorProfilerCallback*>(this);
    }
    else if (iid == IID_ICorProfilerCallback2) {
        * ppv = static_cast<ICorProfilerCallback2*>(this);
    }
    else if (iid == IID_ICorProfilerCallback3) {
        * ppv = static_cast<ICorProfilerCallback3*>(this);
    }
    else {
        * ppv = NULL;
        return E_NOINTERFACE;
    }
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    return S_OK;
}


ULONG __stdcall DetachPerfTester::AddRef()
{
    return InterlockedIncrement(& m_cRef);
}


ULONG __stdcall DetachPerfTester::Release()
{
    if (0 == InterlockedDecrement(& m_cRef)) {
        delete this;
        return 0;
    }
    return m_cRef;
}

//*********************************************************************
//  Descripton: Common routine for DetachPerfTester to initialize itself.
//  
//  Params:     [OUT] LPWSTR wszTestName - Name of the test.
//              [IN] void* pvClientData - Initialization data passed
//                  from the client. Optional and can be NULL. To be 
//                  used only if calling from InitializeForAttach()
//                  callback.
//
//  Return:     S_OK if method successful, else appropriate error code.
//  
//  Notes:      This is not a profiler callback.
//*********************************************************************

HRESULT DetachPerfTester::CommonInit(LPWSTR wszTestName, void* pvClientData)
{
    TRACEENTRY(L"DetachPerfTester::CommonInit")

    /////////////////////////////////////////////////////

    HRESULT hr = S_OK;    
    DWORD dwRet = 0;
    DWORD dwMask = 0;    
    
    wchar_t wszDetachTime[MAX_PATH] = {0};
    
    // check the env-var DETACH_WAITTIME
    dwRet = GetEnvironmentVariable(ENVVAR_DETACH_WAITTIME, wszDetachTime, MAX_PATH);
    if (0 == dwRet)
        return HRESULT_FROM_WIN32(GetLastError());
    
    m_detachWaitTime = _wtoi((const wchar_t *) wszDetachTime); 
    
    TRACENORMAL(L"Detach Wait-time set to %d seconds", m_detachWaitTime);


    // set the event mask and subscribe for the profiler callback notifications
    dwMask = (COR_PRF_MONITOR_ALL ) & ~(COR_PRF_MONITOR_IMMUTABLE | COR_PRF_MONITOR_CODE_TRANSITIONS | COR_PRF_MONITOR_ENTERLEAVE);
       
    hr = m_pInfo->SetEventMask(dwMask);
    HR_CHECK(hr, S_OK, L"ICorProfilerInfo3::SetEventMask");

    TRACENORMAL(L"Event mask set to 0x%X", dwMask);


    // prepare whatever needs to be passed onto the worker threads 
    m_pThrParams->pInfo =  m_pInfo;
    m_pThrParams->dwTimeOut = m_detachWaitTime;

    // start the async thread
    m_hAsyncThread = (HANDLE) _beginthreadex(
                                    NULL, 
                                    0, 
                                    & DetachPerfTester::AsyncThreadFunc, 
                                    (void*) this, 
                                    NULL, 
                                    NULL);

    
ErrReturn:

    TRACEEXIT(L"DetachPerfTester::CommonInit")

    return hr;  
} 

//*********************************************************************
//  Descripton: Profiler intialize callback. This method does all the
//              necessary plumbing work (setting up pipe connection 
//              with RunProf, creating profilee sync event etc) and
//              then kicks off the specified profiler detach test. 
//  
//  Params:     [IN] IUnknown * pICorProfilerInfoUnk - An IUnknown 
//                  interface pointer which can be QI'ed for the
//                  ICorProfilerInfo3 interface.
//
//  Return:     S_OK if method successful, else appropriate error code.
//  
//  Notes:      n/a
//*********************************************************************

HRESULT  __stdcall DetachPerfTester::Initialize(IUnknown * pICorProfilerInfoUnk)
{
    TRACEENTRY(L"DetachPerfTester::Initialize")

    /////////////////////////////////////////////////////

    HRESULT hr = S_OK;
    DWORD dwMask = 0;
    wchar_t wszTestName[MAX_PATH] = {0};

    // pre-condition checks
    if (NULL == m_pThrParams) {
        hr = E_FAIL;
        TRACEERROR(hr, L"Failed a pre-condition check");
        goto ErrReturn;
    }
    if (NULL != m_pInfo) {
        hr = E_FAIL;
        TRACEERROR(hr, L"Failed a precondition check -> m_pInfo");
        goto ErrReturn;
    }
    if (NULL == pICorProfilerInfoUnk) {
        hr = E_INVALIDARG;
        TRACEERROR(hr, L"Invalid arg passed into DetachPerfTester::Initialize -> pICorProfilerInfoUnk");
        goto ErrReturn;
    }
   
    hr = pICorProfilerInfoUnk->QueryInterface(IID_ICorProfilerInfo3, (void **) & m_pInfo);
    HR_CHECK(hr, S_OK, L"IUnknown::QI");
    m_pInfo->AddRef();

    hr = CommonInit(wszTestName, NULL);
    HR_CHECK(hr, S_OK, L"DetachPerfTester::CommonInit");

    /////////////////////////////////////////////////////


ErrReturn:

    TRACEEXIT(L"DetachPerfTester::Initialize")

    return hr;
}


//*********************************************************************
//  Descripton: First callback received when trigger app tries to 
//              attach to running process. This method does all the
//              necessary plumbing work (setting up pipe connection 
//              with RunProf, creating profilee sync event etc) and
//              then kicks off the specified profiler detach test. 
//  
//  Params:     [IN] IUnknown * pICorProfilerInfoUnk - An IUnknown 
//                  interface pointer which can be QI'ed for the
//                  ICorProfilerInfo3 interface.
//              [IN] void * pvClientData - 
//              [IN] UINT cbClientData - 
//
//  Return:     S_OK if method successful, else appropriate error code.
//  
//  Notes:      n/a
//*********************************************************************

HRESULT  __stdcall DetachPerfTester::InitializeForAttach(
    IUnknown * pICorProfilerInfoUnk, void * pvClientData,
    UINT cbClientData)
{
    TRACEENTRY(L"DetachPerfTester::InitializeForAttach")

    /////////////////////////////////////////////////////

    HRESULT hr = S_OK;
    DWORD dwMask = 0;
    wchar_t wszTestName[MAX_PATH] = {0};

    // pre-condition checks
    if (NULL == m_pThrParams) {
        hr = E_FAIL;
        TRACEERROR(hr, L"Failed a pre-condition check");
        goto ErrReturn;
    }
    if (NULL != m_pInfo) {
        hr = E_FAIL;
        TRACEERROR(hr, L"Failed a precondition check -> m_pInfo");
        goto ErrReturn;
    }
    if (NULL == pICorProfilerInfoUnk) {
        hr = E_INVALIDARG;
        TRACEERROR(hr, L"Invalid arg passed into DetachPerfTester::Initialize -> pICorProfilerInfoUnk");
        goto ErrReturn;
    }
   
    hr = pICorProfilerInfoUnk->QueryInterface(IID_ICorProfilerInfo3, (void **) & m_pInfo);
    HR_CHECK(hr, S_OK, L"IUnknown::QI");
    m_pInfo->AddRef();

    hr = CommonInit(wszTestName, pvClientData);
    HR_CHECK(hr, S_OK, L"TestHelper::CommonInit");

    /////////////////////////////////////////////////////

ErrReturn:

    TRACEEXIT(L"DetachPerfTester::InitializeForAttach")

    return hr;

} 


//*********************************************************************
//  Descripton: callback from profiler indicating that attach is now
//              complete and that it is safe to resume normal profiler
//              duties.
//  
//  Params:     none.
//
//  Return:     S_OK if method successful, else appropriate error code.
//  
//  Notes:      n/a
//*********************************************************************

HRESULT  __stdcall DetachPerfTester::ProfilerAttachComplete()
{
    TRACEENTRY(L"DetachPerfTester::ProfilerAttachComplete")

    /////////////////////////////////////////////////////

    HRESULT hr = S_OK;

ErrReturn:

    return hr;
}


//*********************************************************************
//  Descripton: Profiler callback when sealing + evacuation phases are
//              complete. 
//  
//  Params:     none.
//
//  Return:     S_OK if method successful, else appropriate error code.
//  
//  Notes:      n/a
//*********************************************************************

HRESULT  __stdcall DetachPerfTester::ProfilerDetachSucceeded()
{
    TRACEENTRY(L"DetachPerfTester::ProfilerDetachSucceeded")

    /////////////////////////////////////////////////////

    HRESULT hr = S_OK;

ErrReturn:

    return hr;    
}


//*********************************************************************
//  Descripton: Self-explanatory profiler callback. 
//  
//  Params:     none.
//
//  Return:     S_OK if method successful, else appropriate error code.
//  
//  Notes:      n/a
//*********************************************************************

HRESULT  __stdcall DetachPerfTester::Shutdown()
{
    TRACEENTRY(L"DetachPerfTester::Shutdown")

    /////////////////////////////////////////////////////

    HRESULT hr = S_OK;


ErrReturn:

    return hr;
}



///////////////////////////////////////////////////////////////////////////////


//*********************************************************************
//  Descripton: This purpose of this method is to call the
//              ICorProfilerInfo3::RequestProfilerDetach API in an
//              async manner.
//  
//  Params:     void * p - Parameters passed to the thread.  
//
//  Notes:      Even though this is a re-entrant method, we are not
//              guarding this with critical sections etc since no
//              global variables are being accessed. All vars here are 
//              local stack variables.
//*********************************************************************

unsigned __stdcall DetachPerfTester::AsyncThreadFunc(void * p)
{
    TRACEENTRY(L"DetachPerfTester::AsyncThreadFunc")

    /////////////////////////////////////////////////////

    HRESULT hr = S_OK;
    DetachPerfTester * pTest = (DetachPerfTester*) p;
        
    
    // we'll request detach asynchronously
    
    hr = pTest->m_helper.DetachFromMultipleThreads(pTest->m_pInfo, TRUE, FALSE, (PVOID) pTest->m_pThrParams);
    HR_CHECK(hr, S_OK, L"TestHelper::DetachFromMultipleThreads");

    hr = pTest->m_helper.ResumeWorkerThreads();
    HR_CHECK(hr, S_OK, L"TestHelper::ResumeThreads");

    /////////////////////////////////////////////////////

ErrReturn:

    return hr;
}







