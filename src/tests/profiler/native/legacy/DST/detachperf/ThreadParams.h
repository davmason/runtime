// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#pragma once 

///////////////////////////////////////
// struct declaration : ThreadParams //
///////////////////////////////////////

typedef struct _ThreadParams {
    ICorProfilerInfo3 * pInfo;
    DWORD dwTimeOut;  // in seconds
}
ThreadParams, * pThreadParams;

