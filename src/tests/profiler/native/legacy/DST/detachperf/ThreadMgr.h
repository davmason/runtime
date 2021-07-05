#pragma once

class ThreadMgr
{
 public:

    ThreadMgr(
        DWORD dwThreads
        ); 

    ~ ThreadMgr(
        );

    HRESULT SpawnThreads(
        BOOL bCreateSuspended,
        BOOL bWaitForThreadExit,
        DWORD dwWaitTimeOut,
        void *  tparams,
        unsigned (* start_address)(void *)
        ); 

    HRESULT SuspendThreads(
        );

    HRESULT ResumeThreads(
        );

    HRESULT GetExitCodes(
        DWORD ** ppdwExitCodes        
        );

    HRESULT Reset(
        )   {
                return E_NOTIMPL;            
            }


 private:

    // count of worker threads
    DWORD m_cThreads;

    // array of thread handles
    HANDLE * m_phThreads;

    // array of exit codes from worker threads
    DWORD * m_pdwExitCodes;

    BOOL m_bInit;
};
