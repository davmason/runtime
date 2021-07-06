// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#pragma once
// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// TestProfiler.h
//
// Defines the profiler test framework implementation of ICorProfilerCallback2.
// 
// ======================================================================================


// Windows includes
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#include <windows.h>
#endif

// Printing, logging, booking, helper, and utility routines used by TestProfiler and Satellite Modules.
#include "ProfilerCommon.h"

// Modified print macros for use in TestProfiler.cpp
#undef DISPLAY
#undef FAILURE

// Assert helps locate where these macros are used with a null m_pPrfCom pointer. 
#define DISPLAY( message )  {                                                                       \
                                assert(m_pPrfCom != NULL);                                          \
                                std::wstringstream temp;                                            \
                                temp << message;                                                    \
                                m_pPrfCom->Display(temp);  				                            \
                            }

#define FAILURE( message )  {                                                                       \
                                assert(m_pPrfCom != NULL);                                          \
                                std::wstringstream temp;                                            \
                                temp << message;                                                    \
                                m_pPrfCom->Error(temp) ;  					                        \
                            }

#undef  PPRFCOM
#define PPRFCOM m_pPrfCom
#undef  PINFO
#define PINFO m_pPrfCom->m_pInfo

extern const GUID __declspec( selectany ) CLSID_PROFILER = // {55AF0BED-DA1B-419d-8965-999F41DA4E1A}
{ 0x55af0bed, 0xda1b, 0x419d, { 0x89, 0x65, 0x99, 0x9f, 0x41, 0xda, 0x4e, 0x1a } };

/***************************************************************************************
 ********************                                               ********************
 ********************      TestProfiler Declaration           ********************
 ********************                                               ********************
 ***************************************************************************************/

#include "../profiler.h"

class TestProfiler : public Profiler
{
public:
    TestProfiler();
    virtual ~TestProfiler();

    virtual GUID GetClsid();
    
    virtual HRESULT STDMETHODCALLTYPE Initialize(IUnknown *pProfilerInfoUnk);

    virtual HRESULT STDMETHODCALLTYPE InitializeForAttach(IUnknown * pCorProfilerInfoUnk,
                                                          void * pvClientData,
                                                          UINT cbClientData);
    
    virtual HRESULT STDMETHODCALLTYPE ProfilerAttachComplete();

    virtual HRESULT STDMETHODCALLTYPE ProfilerDetachSucceeded();

    virtual HRESULT STDMETHODCALLTYPE Shutdown();

    virtual HRESULT STDMETHODCALLTYPE AppDomainCreationStarted(     /* [in] */ AppDomainID appDomainId);

    virtual HRESULT STDMETHODCALLTYPE AppDomainCreationFinished(    /* [in] */ AppDomainID appDomainId,
                                                                    /* [in] */ HRESULT hrStatus);

    virtual HRESULT STDMETHODCALLTYPE AppDomainShutdownStarted(     /* [in] */ AppDomainID appDomainId);

    virtual HRESULT STDMETHODCALLTYPE AppDomainShutdownFinished(    /* [in] */ AppDomainID appDomainId,
                                                                    /* [in] */ HRESULT hrStatus);

    virtual HRESULT STDMETHODCALLTYPE AssemblyLoadStarted(          /* [in] */ AssemblyID assemblyId);

    virtual HRESULT STDMETHODCALLTYPE AssemblyLoadFinished(         /* [in] */ AssemblyID assemblyId,
                                                                    /* [in] */ HRESULT hrStatus);

    virtual HRESULT STDMETHODCALLTYPE AssemblyUnloadStarted(        /* [in] */ AssemblyID assemblyId);

    virtual HRESULT STDMETHODCALLTYPE AssemblyUnloadFinished(       /* [in] */ AssemblyID assemblyId,
                                                                    /* [in] */ HRESULT hrStatus);

    virtual HRESULT STDMETHODCALLTYPE ModuleLoadStarted(            /* [in] */ ModuleID moduleId);

    virtual HRESULT STDMETHODCALLTYPE ModuleLoadFinished(           /* [in] */ ModuleID moduleId,
                                                                    /* [in] */ HRESULT hrStatus);

    virtual HRESULT STDMETHODCALLTYPE ModuleUnloadStarted(          /* [in] */ ModuleID moduleId);

    virtual HRESULT STDMETHODCALLTYPE ModuleUnloadFinished(         /* [in] */ ModuleID moduleId,
                                                                    /* [in] */ HRESULT hrStatus);

    virtual HRESULT STDMETHODCALLTYPE ModuleAttachedToAssembly(     /* [in] */ ModuleID moduleId,
                                                                    /* [in] */ AssemblyID AssemblyId);

    virtual HRESULT STDMETHODCALLTYPE ClassLoadStarted(             /* [in] */ ClassID classId);

    virtual HRESULT STDMETHODCALLTYPE ClassLoadFinished(            /* [in] */ ClassID classId,
                                                                    /* [in] */ HRESULT hrStatus);

    virtual HRESULT STDMETHODCALLTYPE ClassUnloadStarted(           /* [in] */ ClassID classId);

    virtual HRESULT STDMETHODCALLTYPE ClassUnloadFinished(          /* [in] */ ClassID classId,
                                                                    /* [in] */ HRESULT hrStatus);

    virtual HRESULT STDMETHODCALLTYPE FunctionUnloadStarted(        /* [in] */ FunctionID functionId);

    virtual HRESULT STDMETHODCALLTYPE JITCompilationStarted(        /* [in] */ FunctionID functionId,
                                                                    /* [in] */ BOOL fIsSafeToBlock);

    virtual HRESULT STDMETHODCALLTYPE JITCompilationFinished(       /* [in] */ FunctionID functionId,
                                                                    /* [in] */ HRESULT hrStatus,
                                                                    /* [in] */ BOOL fIsSafeToBlock);

    virtual HRESULT STDMETHODCALLTYPE JITCachedFunctionSearchStarted(/*[in] */ FunctionID functionId,
                                                                   /* [out] */ BOOL *pbUseCachedFunction);

    virtual HRESULT STDMETHODCALLTYPE JITCachedFunctionSearchFinished(/*[in]*/ FunctionID functionId,
                                                                    /* [in] */ COR_PRF_JIT_CACHE result);

    virtual HRESULT STDMETHODCALLTYPE JITFunctionPitched(           /* [in] */ FunctionID functionId);

    virtual HRESULT STDMETHODCALLTYPE JITInlining(                  /* [in] */ FunctionID callerId,
                                                                    /* [in] */ FunctionID calleeId,
                                                                    /* [out] */ BOOL *pfShouldInline);

    virtual HRESULT STDMETHODCALLTYPE ThreadCreated(                /* [in] */ ThreadID threadID );

    virtual HRESULT STDMETHODCALLTYPE ThreadDestroyed(              /* [in] */ ThreadID threadID );

    virtual HRESULT STDMETHODCALLTYPE ThreadAssignedToOSThread(     /* [in] */  ThreadID managedThreadId,
                                                                    /* [in] */ DWORD osThreadId);

    virtual HRESULT STDMETHODCALLTYPE ThreadNameChanged(            /* [in] */  ThreadID managedThreadId,
                                                                    /* [in] */ ULONG cchName,
                                                                    /* [in] */ PROFILER_WCHAR name[]);

    virtual HRESULT STDMETHODCALLTYPE RemotingClientInvocationStarted( );

    virtual HRESULT STDMETHODCALLTYPE RemotingClientSendingMessage( /* [in] */ GUID *pCookie,
                                                                    /* [in] */ BOOL fIsAsync);

    virtual HRESULT STDMETHODCALLTYPE RemotingClientReceivingReply( /* [in] */ GUID *pCookie,
                                                                    /* [in] */ BOOL fIsAsync);

    virtual HRESULT STDMETHODCALLTYPE RemotingClientInvocationFinished( );

    virtual HRESULT STDMETHODCALLTYPE RemotingServerReceivingMessage(/*[in] */ GUID *pCookie,
                                                                    /* [in] */ BOOL fIsAsync);

    virtual HRESULT STDMETHODCALLTYPE RemotingServerInvocationStarted( );

    virtual HRESULT STDMETHODCALLTYPE RemotingServerInvocationReturned( );

    virtual HRESULT STDMETHODCALLTYPE RemotingServerSendingReply(   /* [in] */ GUID *pCookie,
                                                                    /* [in] */ BOOL fIsAsync);

    virtual HRESULT STDMETHODCALLTYPE UnmanagedToManagedTransition( /* [in] */ FunctionID functionId,
                                                                    /* [in] */ COR_PRF_TRANSITION_REASON reason);

    virtual HRESULT STDMETHODCALLTYPE ManagedToUnmanagedTransition( /* [in] */ FunctionID functionId,
                                                                    /* [in] */ COR_PRF_TRANSITION_REASON reason);

    virtual HRESULT STDMETHODCALLTYPE RuntimeSuspendStarted(        /* [in] */ COR_PRF_SUSPEND_REASON suspendReason);

    virtual HRESULT STDMETHODCALLTYPE RuntimeSuspendFinished(  );

    virtual HRESULT STDMETHODCALLTYPE RuntimeSuspendAborted(  );

    virtual HRESULT STDMETHODCALLTYPE RuntimeResumeStarted(  );

    virtual HRESULT STDMETHODCALLTYPE RuntimeResumeFinished(  );

    virtual HRESULT STDMETHODCALLTYPE RuntimeThreadSuspended(       /* [in] */ ThreadID threadID );

    virtual HRESULT STDMETHODCALLTYPE RuntimeThreadResumed(         /* [in] */ ThreadID threadID );

    virtual HRESULT STDMETHODCALLTYPE MovedReferences(              /* [in] */ ULONG cMovedObjectIDRanges,
                                                                    /* [in] */ ObjectID oldObjectIDRangeStart[  ],
                                                                    /* [in] */ ObjectID newObjectIDRangeStart[  ],
                                                                    /* [in] */ ULONG cObjectIDRangeLength[  ]);

    virtual HRESULT STDMETHODCALLTYPE ObjectAllocated(              /* [in] */ ObjectID objectId,
                                                                    /* [in] */ ClassID classId);

    virtual HRESULT STDMETHODCALLTYPE ObjectsAllocatedByClass(      /* [in] */ ULONG cClassCount,
                                                                    /* [in] */ ClassID classIds[  ],
                                                                    /* [in] */ ULONG cObjects[  ]);

    virtual HRESULT STDMETHODCALLTYPE ObjectReferences(             /* [in] */ ObjectID objectId,
                                                                    /* [in] */ ClassID classId,
                                                                    /* [in] */ ULONG cObjectRefs,
                                                                    /* [in] */ ObjectID objectRefIds[  ]);

    virtual HRESULT STDMETHODCALLTYPE RootReferences(               /* [in] */ ULONG cRootRefs,
                                                                    /* [in] */ ObjectID rootRefIds[  ]);

    virtual HRESULT STDMETHODCALLTYPE ExceptionThrown(              /* [in] */ ObjectID thrownObjectId);
	virtual UINT_PTR STDMETHODCALLTYPE FunctionIDMapper(FunctionID functionId, BOOL *pbHookFunction );
	virtual UINT_PTR STDMETHODCALLTYPE FunctionIDMapper2(FunctionID functionId, void * clientData, BOOL *pbHookFunction);

    virtual HRESULT STDMETHODCALLTYPE ExceptionSearchFunctionEnter( /* [in] */ FunctionID functionId);

    virtual HRESULT STDMETHODCALLTYPE ExceptionSearchFunctionLeave(  );

    virtual HRESULT STDMETHODCALLTYPE ExceptionSearchFilterEnter(   /* [in] */ FunctionID functionId);

    virtual HRESULT STDMETHODCALLTYPE ExceptionSearchFilterLeave(  );

    virtual HRESULT STDMETHODCALLTYPE ExceptionSearchCatcherFound(  /* [in] */ FunctionID functionId);

    virtual HRESULT STDMETHODCALLTYPE ExceptionOSHandlerEnter(      /* [in] */ UINT_PTR __unused);

    virtual HRESULT STDMETHODCALLTYPE ExceptionOSHandlerLeave(      /* [in] */ UINT_PTR __unused);

    virtual HRESULT STDMETHODCALLTYPE ExceptionUnwindFunctionEnter( /* [in] */ FunctionID functionId);

    virtual HRESULT STDMETHODCALLTYPE ExceptionUnwindFunctionLeave(  );

    virtual HRESULT STDMETHODCALLTYPE ExceptionUnwindFinallyEnter(  /* [in] */ FunctionID functionId);

    virtual HRESULT STDMETHODCALLTYPE ExceptionUnwindFinallyLeave( );

    virtual HRESULT STDMETHODCALLTYPE ExceptionCatcherEnter(        /* [in] */ FunctionID functionId,
                                                                    /* [in] */ ObjectID objectId);

    virtual HRESULT STDMETHODCALLTYPE ExceptionCatcherLeave( );

    virtual HRESULT STDMETHODCALLTYPE ExceptionCLRCatcherFound( );

    virtual HRESULT STDMETHODCALLTYPE ExceptionCLRCatcherExecute( );

    virtual HRESULT STDMETHODCALLTYPE COMClassicVTableCreated(      /* [in] */ ClassID wrappedClassID,
                                                                    /* [in] */ REFGUID implementedIID,
                                                                    /* [in] */ VOID *pVTable,
                                                                    /* [in] */ ULONG cSlots );

    virtual HRESULT STDMETHODCALLTYPE COMClassicVTableDestroyed(    /* [in] */ ClassID wrappedClassID,
                                                                    /* [in] */ REFGUID implementedIID,
                                                                    /* [in] */ VOID *pVTable );

    virtual HRESULT STDMETHODCALLTYPE GarbageCollectionStarted(     /* [in] */ INT cGenerations,
                                                                    /* [in] */ BOOL generationCollected[],
                                                                    /* [in] */ COR_PRF_GC_REASON reason);

    virtual HRESULT STDMETHODCALLTYPE GarbageCollectionFinished();

    virtual HRESULT STDMETHODCALLTYPE FinalizeableObjectQueued(     /* [in] */ DWORD finalizerFlags,
                                                                    /* [in] */ ObjectID objectID);

    virtual HRESULT STDMETHODCALLTYPE RootReferences2(              /* [in] */ ULONG    cRootRefs,
                                                                    /* [in] */ ObjectID rootRefIds[],
                                                                    /* [in] */ COR_PRF_GC_ROOT_KIND rootKinds[],
                                                                    /* [in] */ COR_PRF_GC_ROOT_FLAGS rootFlags[],
                                                                    /* [in] */ UINT_PTR rootIds[]);

    virtual HRESULT STDMETHODCALLTYPE HandleCreated(                /* [in] */ GCHandleID handleId,
                                                                    /* [in] */ ObjectID initialObjectId);

    virtual HRESULT STDMETHODCALLTYPE HandleDestroyed(              /* [in] */ GCHandleID handleId);

    virtual HRESULT STDMETHODCALLTYPE SurvivingReferences(          /* [in] */ ULONG    cSurvivingObjectIDRanges,
                                                                    /* [in] */ ObjectID objectIDRangeStart[],
                                                                    /* [in] */ ULONG    cObjectIDRangeLength[]);

    VOID FunctionEnterCallBack(                                     /* [in] */ FunctionID funcID);
                
    VOID FunctionLeaveCallBack(                                     /* [in] */ FunctionID funcID);
                
    VOID FunctionTailcallCallBack(                                  /* [in] */ FunctionID funcID);

    VOID FunctionEnter2CallBack(                                    /* [in] */ FunctionID funcID,
                                                                    /* [in] */ UINT_PTR mappedFuncID,
                                                                    /* [in] */ COR_PRF_FRAME_INFO frame,
                                                                    /* [in] */ COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo);

    VOID FunctionLeave2CallBack(                                    /* [in] */ FunctionID funcID,
                                                                    /* [in] */ UINT_PTR mappedFuncID,
                                                                    /* [in] */ COR_PRF_FRAME_INFO frame,
                                                                    /* [in] */ COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange);

	VOID FunctionTailcall2CallBack(                                 /* [in] */ FunctionID funcID,
                                                                    /* [in] */ UINT_PTR mappedFuncID,
                                                                    /* [in] */ COR_PRF_FRAME_INFO frame);
	
	VOID FunctionEnter3WithInfoCallBack(							/* [in] */ FunctionIDOrClientID functionIDOrClientID,
																	/* [in] */ COR_PRF_ELT_INFO eltInfo);

	VOID FunctionLeave3WithInfoCallBack(							/* [in] */ FunctionIDOrClientID functionIDOrClientID,
																	/* [in] */ COR_PRF_ELT_INFO eltInfo);

	VOID FunctionTailCall3WithInfoCallBack(							/* [in] */ FunctionIDOrClientID functionIDOrClientID,
																	/* [in] */ COR_PRF_ELT_INFO eltInfo);

	VOID FunctionEnter3CallBack(									/* [in] */ FunctionIDOrClientID functionIDOrClientID);

	VOID FunctionLeave3CallBack(									/* [in] */ FunctionIDOrClientID functionIDOrClientID);

	VOID FunctionTailCall3CallBack(									/* [in] */ FunctionIDOrClientID functionIDOrClientID);

    virtual HRESULT STDMETHODCALLTYPE ReJITCompilationStarted(FunctionID functionId, ReJITID rejitId, BOOL fIsSafeToBlock);
    virtual HRESULT STDMETHODCALLTYPE GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl);
    virtual HRESULT STDMETHODCALLTYPE ReJITCompilationFinished(FunctionID functionId, ReJITID rejitId, HRESULT hrStatus, BOOL fIsSafeToBlock);
    virtual HRESULT STDMETHODCALLTYPE ReJITError(ModuleID moduleId, mdMethodDef methodId, FunctionID functionId, HRESULT hrStatus);

    virtual HRESULT STDMETHODCALLTYPE MovedReferences2(             /* [in] */ ULONG cMovedObjectIDRanges,
                                                                    /* [in] */ ObjectID oldObjectIDRangeStart[  ],
                                                                    /* [in] */ ObjectID newObjectIDRangeStart[  ],
                                                                    /* [in] */ SIZE_T cObjectIDRangeLength[  ]);
    virtual HRESULT STDMETHODCALLTYPE SurvivingReferences2(         /* [in] */ ULONG    cSurvivingObjectIDRanges,
                                                                    /* [in] */ ObjectID objectIDRangeStart[],
                                                                    /* [in] */ SIZE_T cObjectIDRangeLength[]);

    //Adding IUnknown interface implementations
    //  The are necessary now since CoComClass used to implement them for us
    virtual HRESULT __stdcall QueryInterface(
        const IID & iid, 
        void** ppv
        );

    virtual ULONG __stdcall AddRef(
        );

    virtual ULONG __stdcall Release(
        );

    
public:
    
    //
    // Helpers
    //
    static HRESULT STDMETHODCALLTYPE CreateObject( REFIID riid, VOID **ppInterface );
    static TestProfiler * Instance()
    {
        return This;
    }

	BOOL STDMETHODCALLTYPE IsShutdown()
	{
		return ( m_methodTable.VERIFY == NULL || (PPRFCOM==NULL) || (PPRFCOM->m_Shutdown!=0));
	}
    
private:

    HRESULT VerificationRoutine();
    
    BOOL LoadSatelliteModule(wstring pwszSatelliteModule, wstring strTestName);

    HRESULT CommonInit(IUnknown * pProfilerInfoUnk);

    HRESULT SetELTHooks();

    static TestProfiler * This;

    IPrfCom * m_pPrfCom;
    ICorProfilerInfo3* m_pProfilerInfo;
    
    HINSTANCE         m_profilerSatellite;
    MODULEMETHODTABLE m_methodTable;

	AppDomainID m_TestAppdomainId;

    long m_cRef; // reference count 


}; // ProfilerCallback

class AsyncGuard
{
private:
    IPrfCom *m_ptr;

public:
    AsyncGuard(IPrfCom *pPrfCom) :
        m_ptr(pPrfCom)
    {
        // If Sampling is enabled, first check to see if Signaled to wait for a Hard Suspend
        // This is required since this event is delivered with COR_PRF_MONITOR_THREADS and the logging
        // below takes a lock in ProfilerCommon.  If the logging were removed, this synchronization could
        // be removed as well.

        if (m_ptr->m_fEnableAsynch == TRUE)
        {
            lock_guard<recursive_mutex> guard(m_ptr->m_asynchronousCriticalSection);
            m_ptr->m_asynchEvent->Wait();
            ++(m_ptr->m_ActiveCallbacks);
        }
    }

    ~AsyncGuard()
    {
        // Decrement callback count for Sampling
        if (m_ptr->m_fEnableAsynch == TRUE)
        {
            --(m_ptr->m_ActiveCallbacks);
        }
    }
};

// End of File
