#pragma once

#define THREADS_MAX_LIMIT           50         

////////////////////////////////////
// class declaration : TestHelper //
////////////////////////////////////

class TestHelper
{
 public:
    
    TestHelper(
        );

    ~TestHelper(
        );

    HRESULT DetachFromMultipleThreads(
        /*[IN]*/ ICorProfilerInfo3 * pInfo,
                 BOOL bCreateSuspended,
                 BOOL bWaitForThreadExit,
                 void * pParams
        );

    HRESULT ResumeWorkerThreads(
        );

    static unsigned __stdcall WorkerThreadFunc(
        void * p
        );

    static unsigned __stdcall ForceGCFunc(
        void * p
        );

    HRESULT ForceGCRuntimeSuspension(
        void * p
        );

 private:

    ThreadMgr * m_pThrMgr;

    int m_cThreads;
};

