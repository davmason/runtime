
#include "common.hxx"
#include "ThreadMgr.h"

//*********************************************************************
//  Descripton: Parameterized ctor which allocates required space for
//              caching the thread handles and exit codes at a later
//              time.
//  
//  Params:     DWORD dwThreads - Number of worker threads to be
//                  managed.
//
//  Return:     n/a
//  
//  Notes:      n/a
//*********************************************************************

ThreadMgr::ThreadMgr(DWORD dwThreads)
:   m_cThreads      (dwThreads),
    m_phThreads     (NULL),
    m_pdwExitCodes  (NULL),
    m_bInit         (FALSE)
{
    HRESULT hr = S_OK;

    if (0 == m_cThreads) {
        hr = E_INVALIDARG;
        TRACEERROR2(L"Invalid param passed into ThreadMgr::ThreadMgr -> cThreads");
        goto ErrReturn;        
    }

    // allocate space for caching the thread handles and exit codes

    m_phThreads = new HANDLE[m_cThreads];
    if (NULL == m_phThreads) {
        hr = E_OUTOFMEMORY;
        TRACEERROR2(L"Failed to allocate memory to cache thread handles");
        goto ErrReturn;
    }
    ZeroMemory((PVOID)m_phThreads, m_cThreads * sizeof(m_phThreads[0]));

    m_pdwExitCodes = new DWORD[m_cThreads];
    if (NULL == m_pdwExitCodes) {
        hr = E_OUTOFMEMORY;
        TRACEERROR(hr, L"Failed to allocate memory for thread exit codes");                
        goto ErrReturn;
    }
    ZeroMemory((PVOID)m_pdwExitCodes, m_cThreads * sizeof(m_pdwExitCodes[0]));


ErrReturn:

    if (S_OK == hr) {
        m_bInit = TRUE;            
    }    
}


//*********************************************************************
//  Descripton: Terminates any running worker threads and de-allocates
//              the buffers used for caching thread handles and exit
//              codes
//  
//  Params:     none.
//
//  Return:     n/a
//  
//  Notes:      n/a
//*********************************************************************

ThreadMgr::~ThreadMgr()
{
    // forcibly terminate any active worker threads and close thread handles
    for (int i=0; i<m_cThreads; i++) {
        if (NULL != m_phThreads[i] && INVALID_HANDLE_VALUE != m_phThreads[i]) {
            TerminateThread(m_phThreads[i], 0);
            CloseHandle(m_phThreads[i]);
        }
    }    

    // free the buffers    
    if (NULL != m_phThreads) {
        delete[] m_phThreads;
        m_phThreads = NULL;
    }
    if (NULL != m_pdwExitCodes) {
        delete[] m_pdwExitCodes;
        m_pdwExitCodes = NULL;
    }
}


//*********************************************************************
//  Descripton: Spawns worker threads, lets them execute in a method
//              specified by the caller and can wait for all threads to
//              exit. Optionally it can also create the worker threads 
//              in a suspended state. 
//  
//  Params:     BOOL bCreateSuspended - Should the worker threads be
//                  created in a suspended state? [If yes, the caller
//                  is then required to call ThreadMgr::ResumeThreads
//                  at a later stage] 
//              BOOL bWaitForThreadExit - Should this method block
//                  (by calling WaitForMultipleObjects) until all the
//                  worker threads have exited? Note that this 
//                  parameter is ignored if the bCreateSuspended 
//                  paramter is true.
//              DWORD dwWaitTimeOut - How long should this method wait
//                  for all worker threads to exit? Note that this 
//                  parameter is ignored if the bCreateSuspended 
//                  paramter is true.
//              [IN] void * tparams - Parameters to be passed in to
//                  to all the worker thread. This parameter can be
//                  null
//              unsigned (* start_address)(void *) - The method in
//                  which all worker threads will execute. This 
//                  parameter cannot be null.
//
//  Return:     S_OK if method successful, else appropiate error code.
//  
//  Notes:      n/a
//*********************************************************************

HRESULT ThreadMgr::SpawnThreads(BOOL bCreateSuspended, BOOL bWaitForThreadExit,
    DWORD dwWaitTimeOut, void * tparams, unsigned (* start_address)(void *))
{
    HRESULT hr = S_OK;
    DWORD dwRet = 0;


    // pre-condition checks
    if (FALSE == m_bInit) {
        hr = E_FAIL;
        TRACEERROR2(L"Pre-condition check failed");
        goto ErrReturn;
    }
    if (NULL == tparams) {
        hr = E_INVALIDARG;
        TRACEERROR2(L"Invalid param passed into ThreadMgr::SpawnThreads -> tparams");
        goto ErrReturn;
    }
    if (NULL == start_address) {
        hr = E_INVALIDARG;
        TRACEERROR2(L"Invalid param passed into ThreadMgr::SpawnThreads -> start_address");
        goto ErrReturn;
    }

    // spawn the worker threads now
    for (int i=0; i<m_cThreads; i++) 
    {
        // Clear any previously cached handles and exit codes
        m_phThreads[i] = NULL;
        m_pdwExitCodes[i] = 0;   

        m_phThreads[i] = (HANDLE) _beginthreadex(
                                        NULL, 
                                        0, 
                                        start_address, 
                                        (void*) tparams, 
                                        bCreateSuspended ? CREATE_SUSPENDED : NULL, 
                                        NULL);

        if (NULL == m_phThreads[i] && INVALID_HANDLE_VALUE != m_phThreads[i]) {
            hr = E_FAIL;
            TRACEERROR(hr, "Failed to spawn worker threads");
            break;
        } 
    }

    // should we wait for all threads to exit
    if (bWaitForThreadExit && !bCreateSuspended) {

        dwRet = WaitForMultipleObjectsEx(
                    m_cThreads, 
                    m_phThreads, 
                    TRUE, // wait till all handles are signaled      
                    dwWaitTimeOut,
					FALSE);  

        if (WAIT_OBJECT_0 != dwRet) {
            if (WAIT_TIMEOUT == dwRet) {
                hr = (unsigned) HRESULT_FROM_WIN32(WAIT_TIMEOUT);
            }
            else {
                hr = (unsigned) HRESULT_FROM_WIN32(GetLastError());
            }
            TRACEERROR(hr, L"Kernel32!WaitForSingleObject");
            goto ErrReturn;
        }
    }

ErrReturn:
    return hr;    
}


//*********************************************************************
//  Descripton: Retrieves the thread exit codes from the worker threads
//  
//  Params:     [OUT] DWORD ** ppdwExitCodes - Pointer to buffer
//              containing thread exit codes. The buffer is allocated
//              by the method and must be freed by the caller.
//
//  Return:     S_OK if method successful, else appropiate error code.
//  
//  Notes:      Caller's responsibility to free the buffer by calling
//              delete[] on it.
//*********************************************************************

HRESULT ThreadMgr::GetExitCodes(DWORD ** ppdwExitCodes)
{
    HRESULT hr = S_OK;
    DWORD dwRet = 0;

    // pre-condition checks
    if (FALSE == m_bInit) {
        hr = E_FAIL;
        TRACEERROR2(L"Pre-condition check failed");
        goto ErrReturn;
    }
    if (NULL == ppdwExitCodes) {
        hr = E_FAIL;
        TRACEERROR2(L"Invalid param passed into ThreadMgr::GetExitCodes -> ppdwExitCodes");
        goto ErrReturn;        
    }

    // we'll retrieve all exit codes into our member buffer
    for (int i=0; i<m_cThreads; i++) 
    {
        dwRet = GetExitCodeThread(m_phThreads[i], & m_pdwExitCodes[i]);
        if (0 == dwRet) {
            // we failed to get a thread exit code for some reason.
            hr = HRESULT_FROM_WIN32(GetLastError());
            TRACEERROR(hr, L"Kernel32!GetExitCodeThread");
            goto ErrReturn;
        }
        // NOTE: Some threads might be STILL_ACTIVE. But we don't 
        // really care about that.
    }    

    // we'll create a duplicate buffer for the exit codes
    *ppdwExitCodes = new DWORD[m_cThreads];
    if (NULL == *ppdwExitCodes) {
        hr = E_OUTOFMEMORY;
        TRACEERROR(hr, L"Failed to allocate memory for thread exit codes");                
        goto ErrReturn;
    }
    CopyMemory(
        (PVOID)*ppdwExitCodes, 
        (const PVOID)m_pdwExitCodes, 
        m_cThreads * sizeof(m_pdwExitCodes[0]));

ErrReturn:
    return hr;    
}

//*********************************************************************
//  Descripton: Self-explanatory method
//  
//  Params:     none.
//
//  Return:     S_OK if method successful, else appropiate error code.
//  
//  Notes:      n/a
//*********************************************************************

HRESULT ThreadMgr::SuspendThreads()
{
    HRESULT hr = S_OK;
    DWORD dwRet = 0;

    // pre-condition checks
    if (FALSE == m_bInit) {
        hr = E_FAIL;
        TRACEERROR2(L"Pre-condition check failed");
        goto ErrReturn;
    }

    for (int i=0; i<m_cThreads; i++) 
    {
        if (NULL != m_phThreads[i] && INVALID_HANDLE_VALUE != m_phThreads[i]) {

            dwRet = SuspendThread(m_phThreads[i]);
            if ((DWORD)-1 == dwRet) {

                hr = HRESULT_FROM_WIN32(GetLastError());
                TRACEERROR(hr, L"Kernel32!CreateThread");
                goto ErrReturn;
            }
        }
    }

ErrReturn:
    return hr;    
}


//*********************************************************************
//  Descripton: Self-explantory method
//  
//  Params:     none.
//
//  Return:     S_OK if method successful, else appropiate error code.
//  
//  Notes:      n/a
//*********************************************************************

HRESULT ThreadMgr::ResumeThreads()
{
    HRESULT hr = S_OK;
    DWORD dwRet = 0;

    // pre-condition checks
    if (FALSE == m_bInit) {
        hr = E_FAIL;
        TRACEERROR2(L"Pre-condition check failed");
        goto ErrReturn;
    }

    for (int i=0; i<m_cThreads; i++) 
    {
        if (NULL != m_phThreads[i] && INVALID_HANDLE_VALUE != m_phThreads[i]) {

            dwRet = ResumeThread(m_phThreads[i]);
            if ((DWORD)-1 == dwRet) {

                hr = HRESULT_FROM_WIN32(GetLastError());
                TRACEERROR(hr, L"Kernel32!CreateThread");
                goto ErrReturn;
            }
        }
    }

ErrReturn:
    return hr;    
}


///////////////////////////////////////////////////////////////////////////////