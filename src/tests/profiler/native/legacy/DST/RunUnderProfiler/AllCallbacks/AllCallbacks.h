// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// ======================================================================================
//
// AllCallbacks.h
//
// ======================================================================================

#ifndef __PROFILER_TEST_ALL_CALLBACKS__
#define __PROFILER_TEST_ALL_CALLBACKS__


// Windows includes
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0403
#include <windows.h>
#endif

#include <comdef.h>

#define _ATL_FREE_THREADED
#define _ATL_ALL_WARNINGS
#define _ATL_STATIC_REGISTRY

#include <atlbase.h>
#include <atlcom.h>

using namespace ATL;

// Runtime includes
#include "cor.h"
#include "corhdr.h"     // Needs to be included before corprof
#include "corprof.h"    // Definitions for ICorProfilerCallback and friends

extern const GUID CLSID_ALLCALLBACKS = { 0x19de3edd, 0xf1e9, 0x41f1, { 0xa0, 0xcf, 0xc1, 0x6e, 0xb7, 0xf1, 0x46, 0xb5 } };

/***************************************************************************************
 ********************                AllCallbacks Declaration           ********************
 ***************************************************************************************/

class CAllCallbacks 
  : public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<CAllCallbacks, &CLSID_ALLCALLBACKS>,
    public ICorProfilerCallback2
{
public:
    DECLARE_REGISTRY_RESOURCE(AllCallbacks);

    DECLARE_CLASSFACTORY()
    BEGIN_COM_MAP(CAllCallbacks)
        COM_INTERFACE_ENTRY_IID(IID_ICorProfilerCallback, ICorProfilerCallback)
        COM_INTERFACE_ENTRY_IID(IID_ICorProfilerCallback2,ICorProfilerCallback2)
    END_COM_MAP()

    virtual HRESULT STDMETHODCALLTYPE Initialize(IUnknown *pProfilerInfoUnk);

    virtual HRESULT STDMETHODCALLTYPE Shutdown() { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE AppDomainCreationStarted(     /* [in] */ AppDomainID appDomainId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE AppDomainCreationFinished(    /* [in] */ AppDomainID appDomainId,
                                                                    /* [in] */ HRESULT hrStatus) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE AppDomainShutdownStarted(     /* [in] */ AppDomainID appDomainId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE AppDomainShutdownFinished(    /* [in] */ AppDomainID appDomainId,
                                                                    /* [in] */ HRESULT hrStatus) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE AssemblyLoadStarted(          /* [in] */ AssemblyID assemblyId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE AssemblyLoadFinished(         /* [in] */ AssemblyID assemblyId,
                                                                    /* [in] */ HRESULT hrStatus) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE AssemblyUnloadStarted(        /* [in] */ AssemblyID assemblyId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE AssemblyUnloadFinished(       /* [in] */ AssemblyID assemblyId,
                                                                    /* [in] */ HRESULT hrStatus) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ModuleLoadStarted(            /* [in] */ ModuleID moduleId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ModuleLoadFinished(           /* [in] */ ModuleID moduleId,
                                                                    /* [in] */ HRESULT hrStatus) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ModuleUnloadStarted(          /* [in] */ ModuleID moduleId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ModuleUnloadFinished(         /* [in] */ ModuleID moduleId,
                                                                    /* [in] */ HRESULT hrStatus) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ModuleAttachedToAssembly(     /* [in] */ ModuleID moduleId,
                                                                    /* [in] */ AssemblyID AssemblyId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ClassLoadStarted(             /* [in] */ ClassID classId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ClassLoadFinished(            /* [in] */ ClassID classId,
                                                                    /* [in] */ HRESULT hrStatus) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ClassUnloadStarted(           /* [in] */ ClassID classId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ClassUnloadFinished(          /* [in] */ ClassID classId,
                                                                    /* [in] */ HRESULT hrStatus) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE FunctionUnloadStarted(        /* [in] */ FunctionID functionId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE JITCompilationStarted(        /* [in] */ FunctionID functionId,
                                                                    /* [in] */ BOOL fIsSafeToBlock) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE JITCompilationFinished(       /* [in] */ FunctionID functionId,
                                                                    /* [in] */ HRESULT hrStatus,
                                                                    /* [in] */ BOOL fIsSafeToBlock) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE JITCachedFunctionSearchStarted(/*[in] */ FunctionID functionId,
                                                                   /* [out] */ BOOL *pbUseCachedFunction) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE JITCachedFunctionSearchFinished(/*[in]*/ FunctionID functionId,
                                                                    /* [in] */ COR_PRF_JIT_CACHE result) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE JITFunctionPitched(           /* [in] */ FunctionID functionId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE JITInlining(                  /* [in] */ FunctionID callerId,
                                                                    /* [in] */ FunctionID calleeId,
                                                                    /* [out] */ BOOL *pfShouldInline) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ThreadCreated(                /* [in] */ ThreadID threadID ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ThreadDestroyed(              /* [in] */ ThreadID threadID ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ThreadAssignedToOSThread(     /* [in] */  ThreadID managedThreadId,
                                                                    /* [in] */ DWORD osThreadId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ThreadNameChanged(            /* [in] */  ThreadID managedThreadId,
                                                                    /* [in] */ ULONG cchName,
                                                                    /* [in] */ WCHAR name[]) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE RemotingClientInvocationStarted( ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE RemotingClientSendingMessage( /* [in] */ GUID *pCookie,
                                                                    /* [in] */ BOOL fIsAsync) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE RemotingClientReceivingReply( /* [in] */ GUID *pCookie,
                                                                    /* [in] */ BOOL fIsAsync) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE RemotingClientInvocationFinished( ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE RemotingServerReceivingMessage(/*[in] */ GUID *pCookie,
                                                                    /* [in] */ BOOL fIsAsync) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE RemotingServerInvocationStarted( ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE RemotingServerInvocationReturned( ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE RemotingServerSendingReply(   /* [in] */ GUID *pCookie,
                                                                    /* [in] */ BOOL fIsAsync) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE UnmanagedToManagedTransition( /* [in] */ FunctionID functionId,
                                                                    /* [in] */ COR_PRF_TRANSITION_REASON reason) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ManagedToUnmanagedTransition( /* [in] */ FunctionID functionId,
                                                                    /* [in] */ COR_PRF_TRANSITION_REASON reason) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE RuntimeSuspendStarted(        /* [in] */ COR_PRF_SUSPEND_REASON suspendReason) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE RuntimeSuspendFinished(  ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE RuntimeSuspendAborted(  ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE RuntimeResumeStarted(  ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE RuntimeResumeFinished(  ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE RuntimeThreadSuspended(       /* [in] */ ThreadID threadID ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE RuntimeThreadResumed(         /* [in] */ ThreadID threadID ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE MovedReferences(              /* [in] */ ULONG cMovedObjectIDRanges,
                                                                    /* [in] */ ObjectID oldObjectIDRangeStart[  ],
                                                                    /* [in] */ ObjectID newObjectIDRangeStart[  ],
                                                                    /* [in] */ ULONG cObjectIDRangeLength[  ]) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ObjectAllocated(              /* [in] */ ObjectID objectId,
                                                                    /* [in] */ ClassID classId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ObjectsAllocatedByClass(      /* [in] */ ULONG cClassCount,
                                                                    /* [in] */ ClassID classIds[  ],
                                                                    /* [in] */ ULONG cObjects[  ]) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ObjectReferences(             /* [in] */ ObjectID objectId,
                                                                    /* [in] */ ClassID classId,
                                                                    /* [in] */ ULONG cObjectRefs,
                                                                    /* [in] */ ObjectID objectRefIds[  ]) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE RootReferences(               /* [in] */ ULONG cRootRefs,
                                                                    /* [in] */ ObjectID rootRefIds[  ]) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ExceptionThrown(              /* [in] */ ObjectID thrownObjectId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ExceptionSearchFunctionEnter( /* [in] */ FunctionID functionId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ExceptionSearchFunctionLeave(  ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ExceptionSearchFilterEnter(   /* [in] */ FunctionID functionId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ExceptionSearchFilterLeave(  ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ExceptionSearchCatcherFound(  /* [in] */ FunctionID functionId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ExceptionOSHandlerEnter(      /* [in] */ UINT_PTR __unused) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ExceptionOSHandlerLeave(      /* [in] */ UINT_PTR __unused) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ExceptionUnwindFunctionEnter( /* [in] */ FunctionID functionId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ExceptionUnwindFunctionLeave(  ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ExceptionUnwindFinallyEnter(  /* [in] */ FunctionID functionId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ExceptionUnwindFinallyLeave( ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ExceptionCatcherEnter(        /* [in] */ FunctionID functionId,
                                                                    /* [in] */ ObjectID objectId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ExceptionCatcherLeave( ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ExceptionCLRCatcherFound( ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE ExceptionCLRCatcherExecute( ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE COMClassicVTableCreated(      /* [in] */ ClassID wrappedClassID,
                                                                    /* [in] */ REFGUID implementedIID,
                                                                    /* [in] */ VOID *pVTable,
                                                                    /* [in] */ ULONG cSlots ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE COMClassicVTableDestroyed(    /* [in] */ ClassID wrappedClassID,
                                                                    /* [in] */ REFGUID implementedIID,
                                                                    /* [in] */ VOID *pVTable ) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE GarbageCollectionStarted(     /* [in] */ INT cGenerations,
                                                                    /* [in] */ BOOL generationCollected[],
                                                                    /* [in] */ COR_PRF_GC_REASON reason) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE GarbageCollectionFinished() { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE FinalizeableObjectQueued(     /* [in] */ DWORD finalizerFlags,
                                                                    /* [in] */ ObjectID objectID) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE RootReferences2(              /* [in] */ ULONG    cRootRefs,
                                                                    /* [in] */ ObjectID rootRefIds[],
                                                                    /* [in] */ COR_PRF_GC_ROOT_KIND rootKinds[],
                                                                    /* [in] */ COR_PRF_GC_ROOT_FLAGS rootFlags[],
                                                                    /* [in] */ UINT_PTR rootIds[]) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE HandleCreated(                /* [in] */ GCHandleID handleId,
                                                                    /* [in] */ ObjectID initialObjectId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE HandleDestroyed(              /* [in] */ GCHandleID handleId) { return S_OK; };

    virtual HRESULT STDMETHODCALLTYPE SurvivingReferences(          /* [in] */ ULONG    cSurvivingObjectIDRanges,
                                                                    /* [in] */ ObjectID objectIDRangeStart[],
                                                                    /* [in] */ ULONG    cObjectIDRangeLength[]) { return S_OK; };
public:
    //
    // Helpers
    //
    static HRESULT STDMETHODCALLTYPE CreateObject( REFIID riid, VOID **ppInterface );
};

OBJECT_ENTRY_AUTO(CLSID_ALLCALLBACKS, CAllCallbacks);

#endif //__PROFILER_TEST_EVENT_LOG__
