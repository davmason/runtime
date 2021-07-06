// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
//
// JITCommon.h
//
// ======================================================================================

#include "../ProfilerCommon.h"

#ifndef __PROFILE_JIT_COMMON__
#define __PROFILE_JIT_COMMON__

class IJITCom;
class FunctionInfo;

typedef std::map<FunctionID, FunctionInfo *> FunctionMap;

class FunctionInfo
{
  public:
    FunctionInfo(FunctionID funcId);
    ~FunctionInfo();

    HRESULT SetFunctionFinishedJIT(BOOL finished = TRUE);
    HRESULT SetFunctionCached(BOOL cached = TRUE);
    FunctionID GetFunctionId();
    wstring GetFunctionName();
    HRESULT SetFunctionHooked(BOOL hooked = TRUE);
    VOID IncrementJITCompilationStarted()
    {
        ++(this->m_lJITCompilationStarted);
    }
    VOID IncrementJITCompilationFinished()
    {
        ++(this->m_lJITCompilationFinished);
    }
    LONG GetJITCompilationStarted()
    {
        return this->m_lJITCompilationStarted;
    }
    LONG GetJITCompilationFinished()
    {
        return this->m_lJITCompilationFinished;
    }

  private:
    BOOL m_bIsCompiled;
    BOOL m_bIsCached;
    BOOL m_bIsHooked;
    std::atomic<LONG> m_lJITCompilationStarted;
    std::atomic<LONG> m_lJITCompilationFinished;
    FunctionID m_functionID;
    std::wstring m_functionName;

    // This is just for the DISPLAY macros to work without having to redefine them
  public:
    static IPrfCom *s_IPrfCom;

  private:
    IPrfCom *pPrfCom;
};

class ThreadInfo
{
  public:
    ThreadInfo(ThreadID theadId);
    ~ThreadInfo();

    HRESULT AddFunctionToStack(FunctionID funcId);
    HRESULT AddFunctionToStack(FunctionInfo *funcInfo);
    HRESULT PopFunctionFromStack(FunctionID funcId = NULL);
    HRESULT CompareStackSnapshotToShadow();

    // Moved to public for debug output
    ThreadID m_threadId;

  private:
    typedef vector<FunctionID> FunctionStack;
    FunctionStack m_FunctionStack;

    // This is just for the DISPLAY macros to work without having to redefine them
  public:
    static IPrfCom *s_IPrfCom;

  private:
    IPrfCom *pPrfCom;
};

class DECLSPEC_NOVTABLE IJITCom
{
  public:
    virtual HRESULT EnableFunctionMapper(PMODULEMETHODTABLE pModuleMethodTable) = 0;

    virtual HRESULT AddThreadToMap(ThreadID threadId) = 0;
    virtual HRESULT RemoveThreadFromMap(ThreadID threadId) = 0;
    virtual ThreadInfo *GetThreadFromMap(ThreadID threadId) = 0;

    virtual HRESULT AddFunctionToMap(FunctionID funcId) = 0;
    virtual HRESULT RemoveFunctionFromMap(FunctionID funcId) = 0;
    virtual FunctionInfo *GetFunctionFromMap(FunctionID funcId, UINT_PTR clientData = NULL) = 0;
    virtual HRESULT FunctionMapVerification() = 0;

    virtual HRESULT GetThreadMapLock() = 0;
    virtual HRESULT ReleaseThreadMapLock() = 0;
    virtual HRESULT GetFunctionMapLock() = 0;
    virtual HRESULT ReleaseFunctionMapLock() = 0;

    // Moved to public interface for debug output
    FunctionMap m_functionMap;
};

class JITCommon : public PrfCommon,
                  public IJITCom
{
  public:
    JITCommon(ICorProfilerInfo3 *pInfo);
    ~JITCommon();

    virtual HRESULT EnableFunctionMapper(PMODULEMETHODTABLE pModuleMethodTable);

    virtual HRESULT AddThreadToMap(ThreadID threadId);
    virtual HRESULT RemoveThreadFromMap(ThreadID threadId);
    virtual ThreadInfo *GetThreadFromMap(ThreadID threadId);

    virtual HRESULT AddFunctionToMap(FunctionID mappedFunctionId);
    virtual HRESULT RemoveFunctionFromMap(FunctionID mappedFunctionId);
    virtual FunctionInfo *GetFunctionFromMap(FunctionID mappedFunctionId, UINT_PTR clientData = NULL);
    virtual HRESULT FunctionMapVerification();

    UINT_PTR FunctionIDMapper(FunctionID funcId, BOOL *pbHookFunction);
    static UINT_PTR __stdcall FunctionIDMapperWrapper(FunctionID funcId, BOOL *pbHookFunction)
    {
        return Instance()->FunctionIDMapper(funcId, pbHookFunction);
    }

    HRESULT StackSnapshotCallback(
        FunctionID funcId,
        UINT_PTR ip,
        COR_PRF_FRAME_INFO frameInfo,
        ULONG32 contextSize,
        BYTE context[],
        void *clientData);
        
    static HRESULT __stdcall DoStackSnapshotStackSnapShotCallbackWrapper(
        FunctionID funcId,
        UINT_PTR ip,
        COR_PRF_FRAME_INFO frameInfo,
        ULONG32 contextSize,
        BYTE context[],
        void *clientData)
    {
        return Instance()->StackSnapshotCallback(funcId, ip, frameInfo, contextSize, context, clientData);
    }

    virtual HRESULT GetThreadMapLock()
    {
        this->m_threadMapCritSec.lock();
        return S_OK;
    }

    virtual HRESULT ReleaseThreadMapLock()
    {
        this->m_threadMapCritSec.unlock();
        return S_OK;
    }

    virtual HRESULT GetFunctionMapLock()
    {
        this->m_functionMapCritSec.lock();
        return S_OK;
    }

    virtual HRESULT ReleaseFunctionMapLock()
    {
        this->m_functionMapCritSec.unlock();
        return S_OK;
    }

  private:
    typedef std::map<ThreadID, ThreadInfo *> ThreadMap;
    ThreadMap m_threadMap;

    std::mutex m_threadMapCritSec;
    std::mutex m_functionMapCritSec;

    BOOL m_bMappingFunctionIDs;

    static JITCommon *This;
    static JITCommon *Instance()
    {
        return This;
    }

    // This is just for the DISPLAY macros to work without having to redefine them
    IPrfCom *pPrfCom;
};

#endif // __PROFILE_JIT_COMMON__
