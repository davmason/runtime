#pragma once

//
// MODULE DESCRIPTION
// ==================
// The DetachPerfTester module was written to stress test all the profiler detach
// scenarios. The DetachPerfTester module also provides access to all the "hard-to-
// reach" corners like InitializeForAttach, AttachSucceeded, ProfilerDetachSucceeded 
// etc. Anything that is difficult to test with TestProfiler must go in here.
//
// The test design is best described by the UML diagram below -
//   __________________              ____________
//  |/                 |            |/           |
//  | DetachPerfTester |<>----------| TestHelper |---------
//  |__________________|            |____________|        |
//         |                           /\        ____\|/______
//         |           ___________     \/       |/            |
//         |          |/          |    |        | PCL library |
//         ---------->| ThreadMgr |----|        |_____________|
//                    |___________|
//
// The DetachPerfTester utilizes a "TestHelper" class, which is responsible for crucial 
// plumbing operations like synchronization with the managed profilee app (via
// an auto-reset event) and thread pool management for stress testing via the 
// "ThreadMgr" class whihc provides abstractions for sync/async thread pool 
// management.
// 

#define ENVVAR_DETACH_WAITTIME  L"DETACH_WAITTIME"
#define ENVVAR_TRACE_NORMAL     L"TRACE_NORMAL"
#define ENVVAR_TRACE_ERROR      L"TRACE_ERROR"
#define ENVVAR_TRACE_ENTRY      L"TRACE_ENTRY"
#define ENVVAR_TRACE_EXIT       L"TRACE_EXIT"


//////////////////////////////////////////
// class declaration : DetachPerfTester //
//////////////////////////////////////////

class DetachPerfTester : public ICorProfilerCallback3
{
  public:

    DetachPerfTester(
        );

    ~ DetachPerfTester(
        );

    // ICorProfilerCallback implementation

    virtual HRESULT __stdcall Initialize(
        IUnknown * pICorProfilerInfoUnk
        );

    virtual HRESULT __stdcall Shutdown(
        );

    virtual HRESULT __stdcall AppDomainCreationFinished(
        AppDomainID appDomainId,
        HRESULT hrStatus
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall AppDomainCreationStarted(
        AppDomainID appDomainId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall AppDomainShutdownFinished(
        AppDomainID appDomainId,
        HRESULT hrStatus
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall AppDomainShutdownStarted(
        AppDomainID appDomainId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall AssemblyLoadFinished(
        AssemblyID assemblyId,
        HRESULT hrStatus
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall AssemblyLoadStarted(
        AssemblyID assemblyId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall AssemblyUnloadFinished(
        AssemblyID assemblyId,
        HRESULT hrStatus
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall AssemblyUnloadStarted(
        AssemblyID assemblyId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ClassLoadFinished(
        ClassID classId,
        HRESULT hrStatus
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ClassLoadStarted(
        ClassID classId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ClassUnloadFinished(
        ClassID classId,
        HRESULT hrStatus
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ClassUnloadStarted(
        ClassID classId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall COMClassicVTableCreated(
        ClassID wrappedClassId,
        REFGUID implementedIID,
        void * pVTable,
        ULONG cSlots
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall COMClassicVTableDestroyed(
        ClassID wrappedClassId,
        REFGUID implementedIID,
        void * pVTable
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ExceptionCatcherEnter(
        FunctionID functionId,
        ObjectID objectId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ExceptionCatcherLeave(
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ExceptionCLRCatcherExecute(
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ExceptionCLRCatcherFound(
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ExceptionOSHandlerEnter(
        UINT_PTR __unused
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ExceptionOSHandlerLeave(
        UINT_PTR __unused
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ExceptionSearchCatcherFound(
        FunctionID functionId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ExceptionSearchFunctionEnter(
        FunctionID functionId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ExceptionSearchFunctionLeave(
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ExceptionSearchFilterEnter(
        FunctionID functionId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ExceptionSearchFilterLeave(
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ExceptionThrown(
        ObjectID thrownObjectId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ExceptionUnwindFinallyEnter(
        FunctionID functionId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ExceptionUnwindFinallyLeave(
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ExceptionUnwindFunctionEnter(
        FunctionID functionId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ExceptionUnwindFunctionLeave(
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall FunctionUnloadStarted(
        FunctionID functionId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall JITCachedFunctionSearchFinished(
        FunctionID functionId,
        COR_PRF_JIT_CACHE result
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall JITCachedFunctionSearchStarted(
        FunctionID functionId,
        BOOL * pbUseCachedFunction
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall JITCompilationFinished(
        FunctionID functionId,
        HRESULT hrStatus,
        BOOL fIsSafeToBlock
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall JITCompilationStarted(
        FunctionID functionId,
        BOOL fIsSafeToBlock
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall JITFunctionPitched(
        FunctionID functionId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall JITInlining(
        FunctionID callerId,
        FunctionID calleeId,
        BOOL * pfShouldInline
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ManagedToUnmanagedTransition(
        FunctionID functionId,
        COR_PRF_TRANSITION_REASON reason
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ModuleAttachedToAssembly(
        ModuleID moduleId,
        AssemblyID AssemblyId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ModuleLoadFinished(
        ModuleID moduleId,
        HRESULT hrStatus
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ModuleLoadStarted(
        ModuleID moduleId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ModuleUnloadFinished(
        ModuleID moduleId,
        HRESULT hrStatus
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ModuleUnloadStarted(
        ModuleID moduleId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall MovedReferences(
        ULONG cMovedObjectIDRanges,
        ObjectID oldObjectIDRangeStart[] ,
        ObjectID newObjectIDRangeStart[] ,
        ULONG cObjectIDRangeLength[]
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ObjectAllocated(
        ObjectID objectId,
        ClassID classId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ObjectReferences(
        ObjectID objectId,
        ClassID classId,
        ULONG cObjectRefs,
        ObjectID objectRefIds[]
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ObjectsAllocatedByClass(
        ULONG cClassCount,
        ClassID classIds[],
        ULONG cObjects[]
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ThreadCreated(
        ThreadID threadId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ThreadDestroyed(
        ThreadID threadId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall ThreadAssignedToOSThread(
        ThreadID managedThreadId,
        DWORD osThreadId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall RemotingClientInvocationStarted(
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall RemotingClientSendingMessage(
        GUID * pCookie,
        BOOL fIsAsync
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall RemotingClientReceivingReply(
        GUID * pCookie,
        BOOL fIsAsync
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall RemotingClientInvocationFinished(
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall RemotingServerReceivingMessage(
        GUID * pCookie,
        BOOL fIsAsync
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall RemotingServerInvocationStarted(
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall RemotingServerInvocationReturned(
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall RemotingServerSendingReply(
        GUID * pCookie,
        BOOL fIsAsync
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall UnmanagedToManagedTransition(
        FunctionID functionId,
        COR_PRF_TRANSITION_REASON reason
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall RuntimeSuspendStarted(
        COR_PRF_SUSPEND_REASON suspendReason        
        )   {
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall RuntimeSuspendFinished(
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall RuntimeSuspendAborted(
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall RuntimeResumeStarted(
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall RuntimeResumeFinished(
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall RuntimeThreadSuspended(
        ThreadID threadId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall RuntimeThreadResumed(
        ThreadID threadId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall RootReferences(
        ULONG    cRootRefs,
        ObjectID rootRefIds[]
        )   { 
                return E_NOTIMPL; 
        }


    // ICorProfilerCallBack2 Implementation

    virtual HRESULT __stdcall ThreadNameChanged(
        ThreadID threadId,
        ULONG cchName,
        WCHAR name[]
        )   { 
                return E_NOTIMPL; 
        }   

    virtual HRESULT __stdcall GarbageCollectionStarted(
        int cGenerations,
        BOOL generationCollected[],
        COR_PRF_GC_REASON reason
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall SurvivingReferences(
        ULONG cSurvivingObjectIDRanges,
        ObjectID objectIDRangeStart[] ,
        ULONG cObjectIDRangeLength[]
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall GarbageCollectionFinished(
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall FinalizeableObjectQueued(
        DWORD finalizerFlags,
        ObjectID objectID
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall RootReferences2(
        ULONG cRootRefs,
        ObjectID rootRefIds[],
        COR_PRF_GC_ROOT_KIND rootKinds[],
        COR_PRF_GC_ROOT_FLAGS rootFlags[],
        UINT_PTR rootIds[]
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall HandleCreated(
        GCHandleID handleId,
        ObjectID initialObjectId
        )   { 
                return E_NOTIMPL; 
        }

    virtual HRESULT __stdcall HandleDestroyed(
        GCHandleID handleId
        )   { 
                return E_NOTIMPL; 
        }


    // ICorProfilerCallBack3 Implementation
    
    virtual HRESULT __stdcall InitializeForAttach(
        IUnknown * pICorProfilerInfoUnk,
        void * pvClientData,
        UINT cbClientData
        );
    
    virtual HRESULT __stdcall ProfilerAttachComplete(
        );

    virtual HRESULT __stdcall ProfilerDetachSucceeded(
        );


    // This is ultimately derived from IUnknown. So....

    virtual HRESULT __stdcall QueryInterface(
        const IID & iid, 
        void** ppv
        );

    virtual ULONG __stdcall AddRef(
        );

    virtual ULONG __stdcall Release(
        );


    // utility helpers

    TestHelper m_helper;
    ThreadParams * m_pThrParams;

    // cache the ICorProfilerInfo pointer
    ICorProfilerInfo3 * m_pInfo; 


 protected:

    // reference count 
    long m_cRef; 

    // wait-time till detach (in milliseconds)
    int m_detachWaitTime;

    // handle to async helper thread
    HANDLE m_hAsyncThread;
    
    // common initialization routine
    HRESULT CommonInit(
        /*[OUT]*/ LPWSTR wszTestName,
        /*[IN]*/  void* pvClientData 
        ); 

    static unsigned __stdcall AsyncThreadFunc(
        void * p
        );


        
};



