// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
//
// JITCommon.cpp
//
// ======================================================================================

#include "JITCommon.h"

IPrfCom *ThreadInfo::s_IPrfCom = NULL;
IPrfCom *FunctionInfo::s_IPrfCom = NULL;
JITCommon *JITCommon::This = NULL;

ThreadInfo::ThreadInfo(ThreadID threadId) : m_threadId(threadId)
{
    pPrfCom = s_IPrfCom;
}

ThreadInfo::~ThreadInfo()
{
}

HRESULT ThreadInfo::AddFunctionToStack(FunctionID funcId)
{
    this->m_FunctionStack.push_back(funcId);
    return S_OK;
}

HRESULT ThreadInfo::AddFunctionToStack(FunctionInfo *funcInfo)
{
    AddFunctionToStack(funcInfo->GetFunctionId());
    return S_OK;
}

HRESULT ThreadInfo::PopFunctionFromStack(FunctionID funcId)
{
    HRESULT retval = S_OK;
    if ((0 != funcId) && (this->m_FunctionStack.back() != funcId))
    {
        this->CompareStackSnapshotToShadow();
        FAILURE(L"RemoveFunctionFromShadowStack called on " << funcId << L" while top of stack is " << this->m_FunctionStack.back());
        retval = E_FAIL;
    }
    this->m_FunctionStack.pop_back();
    return retval;
}

HRESULT ThreadInfo::CompareStackSnapshotToShadow()
{
    HRESULT retval = S_OK;

    IJITCom *pJITCom = dynamic_cast<IJITCom *>(pPrfCom);

    // avoid stomping and deadlocks by keeping everyone else out of the FunctionMap while
    // we're dumping the shadow stack
    pJITCom->GetFunctionMapLock();

    DWORD dwEventMask = 0;
    PINFO->GetEventMask(&dwEventMask);
    if (FALSE == (dwEventMask & COR_PRF_ENABLE_FUNCTION_RETVAL))
    {
        DISPLAY(L"Didn't ask for function args and retval - pinfo functions may not work");
    }
    DISPLAY(L"Dump of current Shadow Stack:");
    for (INT i = (INT)this->m_FunctionStack.size() - 1; i >= 0; i--)
    {
        FunctionID funcId = this->m_FunctionStack.at(i);
        FunctionInfo *fi = pJITCom->GetFunctionFromMap(funcId);
        if (NULL == fi)
        {
            DISPLAY(L"\t" << HEX(funcId) << L" - Function not in function table!");
        }
        else
        {
            DISPLAY(L"\t" << HEX(funcId) << L" - " << fi->GetFunctionName());
        }
    }

    DISPLAY(L"\nDump of Stack as reported by DoStackSnapshot:");
    HRESULT hr = PINFO->DoStackSnapshot(NULL,
                                        JITCommon::DoStackSnapshotStackSnapShotCallbackWrapper,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL);

    if (hr == E_INVALIDARG)
    {
        DISPLAY(L"DoStackSnapshot returned E_INVALIDARG");
    }

    pJITCom->ReleaseFunctionMapLock();

    return retval;
}

FunctionInfo::FunctionInfo(FunctionID funcId) : m_bIsCompiled(FALSE),
                                                m_bIsCached(FALSE),
                                                m_lJITCompilationStarted(0),
                                                m_lJITCompilationFinished(0),
                                                m_functionID(funcId),
                                                m_functionName(L"")
{
    pPrfCom = s_IPrfCom;
}

FunctionInfo::~FunctionInfo()
{
}

HRESULT FunctionInfo::SetFunctionFinishedJIT(BOOL finished)
{
    if ((TRUE == finished) && (TRUE == this->m_bIsCached))
    {
        FAILURE(L"FunctionInfo::FunctionFinishedJIT(): Function " << this->m_functionName << L"(" << HEX(this->m_functionID) << L") is already cached - invalid for it to finish JIT");
        return E_FAIL;
    }
    else
    {
        this->m_bIsCompiled = finished;
        return S_OK;
    }
}
HRESULT FunctionInfo::SetFunctionCached(BOOL cached)
{
    if ((TRUE == cached) && (TRUE == this->m_bIsCompiled))
    {
        FAILURE(L"FunctionInfo::FunctionCached(): Function " << this->m_functionName << L"(" << HEX(this->m_functionID) << L") is already JITted - invalid for it to be cached");
        return E_FAIL;
    }
    else
    {
        this->m_bIsCached = cached;
        return S_OK;
    }
}
HRESULT FunctionInfo::SetFunctionHooked(BOOL hooked)
{
    this->m_bIsHooked = hooked;
    return S_OK;
}

FunctionID FunctionInfo::GetFunctionId()
{
    return this->m_functionID;
}

wstring FunctionInfo::GetFunctionName()
{
    // If we've already gotten the name, just return it.
    if (this->m_functionName.size() > 0)
    {
        return this->m_functionName;
    }

    // If the FunctionID is 0, we could be dealing with a native function.
    if (this->m_functionID == 0)
    {
        this->m_functionName = L"Unknown_Native_Function";
    }
    else
    {
        this->m_functionName.reserve(STRING_LENGTH);
        HRESULT hr = pPrfCom->GetFunctionIDName(this->m_functionID, this->m_functionName);
        if (FAILED(hr))
        {
            FAILURE(L"GetFunctionIDName returned " << HEX(hr) << L" for FunctionID " << HEX(m_functionID));
            // Return the error name but don't store it so that if they call us again we may get the name next time
            this->m_functionName = L"";
            return L"Error_Fetching_Name";
        }
    }
    return this->m_functionName;
}

JITCommon::JITCommon(ICorProfilerInfo3 *pInfo)
    : PrfCommon(pInfo)
{
    // This is just for the DISPLAY macros to work without having to redefine them
    pPrfCom = this;
    ThreadInfo::s_IPrfCom = this;
    FunctionInfo::s_IPrfCom = this;

    // Used for the Function ID Mapper
    JITCommon::This = this;

    // By default we don't want function mapping
    this->m_bMappingFunctionIDs = FALSE;
}

JITCommon::~JITCommon()
{
    FreeAndEraseMap(m_threadMap);
    FreeAndEraseMap(m_functionMap);
    
    ThreadInfo::s_IPrfCom = NULL;
    FunctionInfo::s_IPrfCom = NULL;
    JITCommon::This = NULL;
}

// This method should be called from the Initialize routine
HRESULT JITCommon::EnableFunctionMapper(PMODULEMETHODTABLE pModuleMethodTable)
{
    this->m_bMappingFunctionIDs = TRUE;
    REGISTER_CALLBACK(FUNCTIONIDMAPPER, JITCommon::FunctionIDMapperWrapper);
    return S_OK;
}

HRESULT JITCommon::AddThreadToMap(ThreadID threadId)
{
    HRESULT retval = S_OK;

    this->GetThreadMapLock();

    // Validate thread id has never been seen before
    ThreadMap::iterator iTM = this->m_threadMap.find(threadId);
    if (iTM == this->m_threadMap.end())
    {
        ThreadInfo *ti = new ThreadInfo(threadId);
        this->m_threadMap.insert(make_pair(threadId, ti));
    }
    else
    {
        FAILURE(L"CreateThread called for a ThreadID (" << HEX(threadId) << L") which has already been created!");
        retval = E_FAIL;
    }

    this->ReleaseThreadMapLock();

    return retval;
}

HRESULT JITCommon::RemoveThreadFromMap(ThreadID threadId)
{
    HRESULT retval = S_OK;

    this->GetThreadMapLock();

    ThreadMap::iterator iTM = this->m_threadMap.find(threadId);
    if (iTM != this->m_threadMap.end())
    {
        delete iTM->second;
        this->m_threadMap.erase(iTM);
    }
    else
    {
        FAILURE(L"RemoveThread called for a ThreadID (" << HEX(threadId) << L") which was never created!");
        retval = E_FAIL;
    }

    this->ReleaseThreadMapLock();

    return retval;
}

ThreadInfo *JITCommon::GetThreadFromMap(ThreadID threadId)
{
    this->GetThreadMapLock();

    ThreadInfo *retval = NULL;

    ThreadMap::iterator iTM = this->m_threadMap.find(threadId);
    if (iTM != this->m_threadMap.end())
    {
        retval = iTM->second;
    }
    else
    {
        DISPLAY(L"GetThreadFromMap called on an unknown thread");
        if (m_threadMap.begin() == m_threadMap.end())
        {
            DISPLAY(L"\tNo known threads")
        }
        else
        {
            DISPLAY(L"Known threads");
            for (iTM = m_threadMap.begin(); iTM != m_threadMap.begin(); iTM++)
            {
                DISPLAY(L"\tThreadID: " << HEX(iTM->second->m_threadId));
            }
        }
        FAILURE(L"GetThreadFromMap called for a ThreadID (" << HEX(threadId) << L") which was never created!");
    }

    this->ReleaseThreadMapLock();

    return retval;
}

HRESULT JITCommon::AddFunctionToMap(FunctionID mappedFunctionId)
{
    HRESULT retval = S_OK;

    this->GetFunctionMapLock();

    // Check to see if this function id has already been added - only add to map on 1st hit
    FunctionMap::iterator iFM = this->m_functionMap.find(mappedFunctionId);
    if (iFM == this->m_functionMap.end())
    {
        FunctionInfo *fi = new FunctionInfo(mappedFunctionId);
        this->m_functionMap.insert(make_pair(mappedFunctionId, fi));
    }

    this->ReleaseFunctionMapLock();

    return retval;
}

// NOTE: This is called when a JITCacheSearchFinished event indicates the function was not
// found. There's a small race condition possibility that a function could be not found, then
// JITCompilationStarted could com in for it before the JITCachedSearchFinished event resulting
// in the function being removed from the map *after* the JITCompilationStarted which would make
// the JITCompilationFinshed event fail.
HRESULT JITCommon::RemoveFunctionFromMap(FunctionID mappedFunctionId)
{
    HRESULT retval = S_OK;

    this->GetFunctionMapLock();

    FunctionMap::iterator iFM = this->m_functionMap.find(mappedFunctionId);
    if (iFM != this->m_functionMap.end())
    {
        delete iFM->second;
        this->m_functionMap.erase(iFM);
    }

    this->ReleaseFunctionMapLock();

    return retval;
}

FunctionInfo *JITCommon::GetFunctionFromMap(FunctionID mappedFunctionId, UINT_PTR clientData)
{
    this->GetFunctionMapLock();

    FunctionInfo *retval = NULL;

    if ((0 != clientData) && (this->m_bMappingFunctionIDs))
    {
        // Function Mapping is enabled and we were passed non-null clientData
        // clientData should therefore be the pointer to the desired FunctionInfo
        retval = reinterpret_cast<FunctionInfo *>(clientData);
        if (retval->GetFunctionId() != mappedFunctionId)
        {
            // Uh-oh - something is rotten
            retval = NULL;
            FAILURE(L"FunctionInfo->FunctionID " << HEX(retval->GetFunctionId()) << L" != Mapped FunctionID " << HEX(mappedFunctionId));
        }
    }
    else
    {
        // we're not doing function mapping or we were passed a null clientData pointer
        // so look up the function id in the map the normal way
        FunctionMap::iterator iFM = this->m_functionMap.find(mappedFunctionId);
        if (iFM == this->m_functionMap.end())
        {
            retval = NULL;
        }
        else
        {
            retval = iFM->second;
        }
    }

    this->ReleaseFunctionMapLock();

    return retval;
}

HRESULT JITCommon::FunctionMapVerification()
{
    HRESULT retval = S_OK;
    FunctionMap::iterator iFM;
    for (iFM = this->m_functionMap.begin(); iFM != this->m_functionMap.end(); iFM++)
    {
        FunctionInfo *fi = iFM->second;
        if (fi->GetJITCompilationStarted() != fi->GetJITCompilationFinished())
        {
            FAILURE(L"JITCompilationStarted (" << fi->GetJITCompilationStarted() << L") != JITCompilationFinished (" << fi->GetJITCompilationFinished() << L") for Function " << HEX(fi->GetFunctionId()) << L" - " << fi->GetFunctionName());
        }
    }
    return retval;
}

UINT_PTR JITCommon::FunctionIDMapper(FunctionID funcId, BOOL *pbHookFunction)
{
    this->GetFunctionMapLock();

    *pbHookFunction = TRUE;
    FunctionMap::iterator iFM = this->m_functionMap.find(funcId);
    FunctionInfo *fi;
    if (iFM == this->m_functionMap.end())
    {
        // new function that hasn't been mapped
        fi = new FunctionInfo(funcId);
        this->m_functionMap.insert(make_pair(funcId, fi));
    }
    else
    {
        fi = iFM->second;
    }
    fi->SetFunctionHooked(*pbHookFunction);

    DISPLAY(L"mapping " << HEX(funcId) << L"-->" << HEX(fi));

    this->ReleaseFunctionMapLock();

    return reinterpret_cast<UINT_PTR>(fi);
}

/*
 *  Callback routine for DoStackSnapshot
 */
HRESULT JITCommon::StackSnapshotCallback(FunctionID funcId,
                                         UINT_PTR /*ip*/,
                                         COR_PRF_FRAME_INFO /*frameInfo*/,
                                         ULONG32 /*contextSize*/,
                                         BYTE[] /*context[]*/,
                                         void * /*clientData */)
{
    // If the FunctionID is not 0, it is a managed function.
    if (funcId != 0)
    {
        FunctionInfo *fi = this->GetFunctionFromMap(funcId);
        if (NULL == fi)
        {
            DISPLAY(L"\t" << HEX(funcId) << L" - Function not in function table!");
        }
        else
        {
            DISPLAY(L"\t" << HEX(funcId) << L" - " << fi->GetFunctionName());
        }
    }
    else
    {
        DISPLAY(L"\t" << HEX(funcId) << L" - Unmanaged run");
    }
    return S_OK;
}
