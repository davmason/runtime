#pragma once 

///////////////////////////////////////
// struct declaration : ThreadParams //
///////////////////////////////////////

typedef struct _ThreadParams {
    ICorProfilerInfo3 * pInfo;
    DWORD dwTimeOut;  // in seconds
}
ThreadParams, * pThreadParams;

