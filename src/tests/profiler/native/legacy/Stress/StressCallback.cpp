// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// StressCallback.h
//
// Defines the profiler test framework implementation of ICorProfilerCallback2.
// 
// ======================================================================================

// Windows includes
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif

#include <comdef.h>

#define _ATL_NO_AUTOMATIC_NAMESPACE
#define _ATL_FREE_THREADED
#define _ATL_ALL_WARNINGS
#define _ATL_STATIC_REGISTRY

#include <atlbase.h>
#include <atlcom.h>
using namespace ATL;
#include "StressCommon.h"

_COM_SMARTPTR_TYPEDEF(ICorProfilerInfo3, __uuidof(ICorProfilerInfo3));
_COM_SMARTPTR_TYPEDEF(IUnknown         , __uuidof(IUnknown));

extern const GUID __declspec( selectany ) CLSID_STRESS_CALLBACK = //{22AC9464-07A2-4567-B8DD-72F94DEA6FA9}
{ 0x22ac9464, 0x7a2, 0x4567, { 0xb8, 0xdd, 0x72, 0xf9, 0x4d, 0xea, 0x6f, 0xa9 } };


/***************************************************************************************
 ********************                                               ********************
 ********************      StressCallbackDeclaration           ********************
 ********************                                               ********************
 ***************************************************************************************/

class StressCallback
  : public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<StressCallback, &CLSID_STRESS_CALLBACK>,
    public ICorProfilerCallback3
{
public:
    DECLARE_REGISTRY_RESOURCE(StressCallback);

    DECLARE_CLASSFACTORY()
    BEGIN_COM_MAP(StressCallback)
        COM_INTERFACE_ENTRY_IID(IID_ICorProfilerCallback, ICorProfilerCallback)
        COM_INTERFACE_ENTRY_IID(IID_ICorProfilerCallback2,ICorProfilerCallback2)
        COM_INTERFACE_ENTRY_IID(IID_ICorProfilerCallback3,ICorProfilerCallback3)
    END_COM_MAP()

	StressCallback(){m_pInfo=NULL;}
	~StressCallback(){}

    HRESULT STDMETHODCALLTYPE Initialize(IUnknown *pProfilerInfoUnk)
	{
		HRESULT hr = S_OK;
		hr = pProfilerInfoUnk->QueryInterface(IID_ICorProfilerInfo3, (void **) & m_pInfo);
		if (FAILED(hr))
			FAILURE(L"QI for ICorProfilerInfo3 Failed: %x", hr);
    
		m_pInfo->AddRef();
		hr= m_pInfo->SetEventMask(COR_PRF_MONITOR_ALL & ~COR_PRF_ENABLE_REJIT);
		if (FAILED(hr))
			FAILURE(L"SetEventMask COR_PRF_MONITOR_ALL & ~COR_PRF_ENABLE_REJIT Failed: %x", hr);

		DISPLAY(L"Stress Callback Profiler Initialized.");

		return hr;
	}

	//The following interface is just implemented as return S_OK

    HRESULT STDMETHODCALLTYPE InitializeForAttach(IUnknown * pCorProfilerInfoUnk,
                                                          void * pvClientData,
                                                          UINT cbClientData){return S_OK;}
    
    HRESULT STDMETHODCALLTYPE ProfilerAttachComplete(){return S_OK;}

    HRESULT STDMETHODCALLTYPE ProfilerDetachSucceeded(){return S_OK;}

    HRESULT STDMETHODCALLTYPE Shutdown(){return S_OK;}

    HRESULT STDMETHODCALLTYPE AppDomainCreationStarted(     /* [in] */ AppDomainID appDomainId){return S_OK;}

    HRESULT STDMETHODCALLTYPE AppDomainCreationFinished(    /* [in] */ AppDomainID appDomainId,
                                                                    /* [in] */ HRESULT hrStatus){return S_OK;}

    HRESULT STDMETHODCALLTYPE AppDomainShutdownStarted(     /* [in] */ AppDomainID appDomainId){return S_OK;}

    HRESULT STDMETHODCALLTYPE AppDomainShutdownFinished(    /* [in] */ AppDomainID appDomainId,
                                                                    /* [in] */ HRESULT hrStatus){return S_OK;}

    HRESULT STDMETHODCALLTYPE AssemblyLoadStarted(          /* [in] */ AssemblyID assemblyId){return S_OK;}

    HRESULT STDMETHODCALLTYPE AssemblyLoadFinished(         /* [in] */ AssemblyID assemblyId,
                                                                    /* [in] */ HRESULT hrStatus){return S_OK;}

    HRESULT STDMETHODCALLTYPE AssemblyUnloadStarted(        /* [in] */ AssemblyID assemblyId){return S_OK;}

    HRESULT STDMETHODCALLTYPE AssemblyUnloadFinished(       /* [in] */ AssemblyID assemblyId,
                                                                    /* [in] */ HRESULT hrStatus){return S_OK;}

    HRESULT STDMETHODCALLTYPE ModuleLoadStarted(            /* [in] */ ModuleID moduleId){return S_OK;}

    HRESULT STDMETHODCALLTYPE ModuleLoadFinished(           /* [in] */ ModuleID moduleId,
                                                                    /* [in] */ HRESULT hrStatus){return S_OK;}

    HRESULT STDMETHODCALLTYPE ModuleUnloadStarted(          /* [in] */ ModuleID moduleId){return S_OK;}

    HRESULT STDMETHODCALLTYPE ModuleUnloadFinished(         /* [in] */ ModuleID moduleId,
                                                                    /* [in] */ HRESULT hrStatus){return S_OK;}

    HRESULT STDMETHODCALLTYPE ModuleAttachedToAssembly(     /* [in] */ ModuleID moduleId,
                                                                    /* [in] */ AssemblyID AssemblyId){return S_OK;}

    HRESULT STDMETHODCALLTYPE ClassLoadStarted(             /* [in] */ ClassID classId){return S_OK;}

    HRESULT STDMETHODCALLTYPE ClassLoadFinished(            /* [in] */ ClassID classId,
                                                                    /* [in] */ HRESULT hrStatus){return S_OK;}

    HRESULT STDMETHODCALLTYPE ClassUnloadStarted(           /* [in] */ ClassID classId){return S_OK;}

    HRESULT STDMETHODCALLTYPE ClassUnloadFinished(          /* [in] */ ClassID classId,
                                                                    /* [in] */ HRESULT hrStatus){return S_OK;}

    HRESULT STDMETHODCALLTYPE FunctionUnloadStarted(        /* [in] */ FunctionID functionId){return S_OK;}

    HRESULT STDMETHODCALLTYPE JITCompilationStarted(        /* [in] */ FunctionID functionId,
                                                                    /* [in] */ BOOL fIsSafeToBlock){return S_OK;}

    HRESULT STDMETHODCALLTYPE JITCompilationFinished(       /* [in] */ FunctionID functionId,
                                                                    /* [in] */ HRESULT hrStatus,
                                                                    /* [in] */ BOOL fIsSafeToBlock){return S_OK;}

    HRESULT STDMETHODCALLTYPE JITCachedFunctionSearchStarted(/*[in] */ FunctionID functionId,
                                                                   /* [out] */ BOOL *pbUseCachedFunction){return S_OK;}

    HRESULT STDMETHODCALLTYPE JITCachedFunctionSearchFinished(/*[in]*/ FunctionID functionId,
                                                                    /* [in] */ COR_PRF_JIT_CACHE result){return S_OK;}

    HRESULT STDMETHODCALLTYPE JITFunctionPitched(           /* [in] */ FunctionID functionId){return S_OK;}

    HRESULT STDMETHODCALLTYPE JITInlining(                  /* [in] */ FunctionID callerId,
                                                                    /* [in] */ FunctionID calleeId,
                                                                    /* [out] */ BOOL *pfShouldInline){return S_OK;}

    HRESULT STDMETHODCALLTYPE ThreadCreated(                /* [in] */ ThreadID threadID ){return S_OK;}

    HRESULT STDMETHODCALLTYPE ThreadDestroyed(              /* [in] */ ThreadID threadID ){return S_OK;}

    HRESULT STDMETHODCALLTYPE ThreadAssignedToOSThread(     /* [in] */  ThreadID managedThreadId,
                                                                    /* [in] */ DWORD osThreadId){return S_OK;}

    HRESULT STDMETHODCALLTYPE ThreadNameChanged(            /* [in] */  ThreadID managedThreadId,
                                                                    /* [in] */ ULONG cchName,
                                                                    /* [in] */ WCHAR name[]){return S_OK;}

    HRESULT STDMETHODCALLTYPE RemotingClientInvocationStarted( ){return S_OK;}

    HRESULT STDMETHODCALLTYPE RemotingClientSendingMessage( /* [in] */ GUID *pCookie,
                                                                    /* [in] */ BOOL fIsAsync){return S_OK;}

    HRESULT STDMETHODCALLTYPE RemotingClientReceivingReply( /* [in] */ GUID *pCookie,
                                                                    /* [in] */ BOOL fIsAsync){return S_OK;}

    HRESULT STDMETHODCALLTYPE RemotingClientInvocationFinished( ){return S_OK;}

    HRESULT STDMETHODCALLTYPE RemotingServerReceivingMessage(/*[in] */ GUID *pCookie,
                                                                    /* [in] */ BOOL fIsAsync){return S_OK;}

    HRESULT STDMETHODCALLTYPE RemotingServerInvocationStarted( ){return S_OK;}

    HRESULT STDMETHODCALLTYPE RemotingServerInvocationReturned( ){return S_OK;}

    HRESULT STDMETHODCALLTYPE RemotingServerSendingReply(   /* [in] */ GUID *pCookie,
                                                                    /* [in] */ BOOL fIsAsync){return S_OK;}

    HRESULT STDMETHODCALLTYPE UnmanagedToManagedTransition( /* [in] */ FunctionID functionId,
                                                                    /* [in] */ COR_PRF_TRANSITION_REASON reason){return S_OK;}

    HRESULT STDMETHODCALLTYPE ManagedToUnmanagedTransition( /* [in] */ FunctionID functionId,
                                                                    /* [in] */ COR_PRF_TRANSITION_REASON reason){return S_OK;}

    HRESULT STDMETHODCALLTYPE RuntimeSuspendStarted(        /* [in] */ COR_PRF_SUSPEND_REASON suspendReason){return S_OK;}

    HRESULT STDMETHODCALLTYPE RuntimeSuspendFinished(  ){return S_OK;}

    HRESULT STDMETHODCALLTYPE RuntimeSuspendAborted(  ){return S_OK;}

    HRESULT STDMETHODCALLTYPE RuntimeResumeStarted(  ){return S_OK;}

    HRESULT STDMETHODCALLTYPE RuntimeResumeFinished(  ){return S_OK;}

    HRESULT STDMETHODCALLTYPE RuntimeThreadSuspended(       /* [in] */ ThreadID threadID ){return S_OK;}

    HRESULT STDMETHODCALLTYPE RuntimeThreadResumed(         /* [in] */ ThreadID threadID ){return S_OK;}

    HRESULT STDMETHODCALLTYPE MovedReferences(              /* [in] */ ULONG cMovedObjectIDRanges,
                                                                    /* [in] */ ObjectID oldObjectIDRangeStart[  ],
                                                                    /* [in] */ ObjectID newObjectIDRangeStart[  ],
                                                                    /* [in] */ ULONG cObjectIDRangeLength[  ]){return S_OK;}

    HRESULT STDMETHODCALLTYPE ObjectAllocated(              /* [in] */ ObjectID objectId,
                                                                    /* [in] */ ClassID classId){return S_OK;}

    HRESULT STDMETHODCALLTYPE ObjectsAllocatedByClass(      /* [in] */ ULONG cClassCount,
                                                                    /* [in] */ ClassID classIds[  ],
                                                                    /* [in] */ ULONG cObjects[  ]){return S_OK;}

    HRESULT STDMETHODCALLTYPE ObjectReferences(             /* [in] */ ObjectID objectId,
                                                                    /* [in] */ ClassID classId,
                                                                    /* [in] */ ULONG cObjectRefs,
                                                                    /* [in] */ ObjectID objectRefIds[  ]){return S_OK;}

    HRESULT STDMETHODCALLTYPE RootReferences(               /* [in] */ ULONG cRootRefs,
                                                                    /* [in] */ ObjectID rootRefIds[  ]){return S_OK;}

    HRESULT STDMETHODCALLTYPE ExceptionThrown(              /* [in] */ ObjectID thrownObjectId){return S_OK;}

    HRESULT STDMETHODCALLTYPE ExceptionSearchFunctionEnter( /* [in] */ FunctionID functionId){return S_OK;}

    HRESULT STDMETHODCALLTYPE ExceptionSearchFunctionLeave(  ){return S_OK;}

    HRESULT STDMETHODCALLTYPE ExceptionSearchFilterEnter(   /* [in] */ FunctionID functionId){return S_OK;}

    HRESULT STDMETHODCALLTYPE ExceptionSearchFilterLeave(  ){return S_OK;}

    HRESULT STDMETHODCALLTYPE ExceptionSearchCatcherFound(  /* [in] */ FunctionID functionId){return S_OK;}

    HRESULT STDMETHODCALLTYPE ExceptionOSHandlerEnter(      /* [in] */ UINT_PTR __unused){return S_OK;}

    HRESULT STDMETHODCALLTYPE ExceptionOSHandlerLeave(      /* [in] */ UINT_PTR __unused){return S_OK;}

    HRESULT STDMETHODCALLTYPE ExceptionUnwindFunctionEnter( /* [in] */ FunctionID functionId){return S_OK;}

    HRESULT STDMETHODCALLTYPE ExceptionUnwindFunctionLeave(  ){return S_OK;}

    HRESULT STDMETHODCALLTYPE ExceptionUnwindFinallyEnter(  /* [in] */ FunctionID functionId){return S_OK;}

    HRESULT STDMETHODCALLTYPE ExceptionUnwindFinallyLeave( ){return S_OK;}

    HRESULT STDMETHODCALLTYPE ExceptionCatcherEnter(        /* [in] */ FunctionID functionId,
                                                                    /* [in] */ ObjectID objectId){return S_OK;}

    HRESULT STDMETHODCALLTYPE ExceptionCatcherLeave( ){return S_OK;}

    HRESULT STDMETHODCALLTYPE ExceptionCLRCatcherFound( ){return S_OK;}

    HRESULT STDMETHODCALLTYPE ExceptionCLRCatcherExecute( ){return S_OK;}

    HRESULT STDMETHODCALLTYPE COMClassicVTableCreated(      /* [in] */ ClassID wrappedClassID,
                                                                    /* [in] */ REFGUID implementedIID,
                                                                    /* [in] */ VOID *pVTable,
                                                                    /* [in] */ ULONG cSlots ){return S_OK;}

    HRESULT STDMETHODCALLTYPE COMClassicVTableDestroyed(    /* [in] */ ClassID wrappedClassID,
                                                                    /* [in] */ REFGUID implementedIID,
                                                                    /* [in] */ VOID *pVTable ){return S_OK;}

    HRESULT STDMETHODCALLTYPE GarbageCollectionStarted(     /* [in] */ INT cGenerations,
                                                                    /* [in] */ BOOL generationCollected[],
                                                                    /* [in] */ COR_PRF_GC_REASON reason){return S_OK;}

    HRESULT STDMETHODCALLTYPE GarbageCollectionFinished(){return S_OK;}

    HRESULT STDMETHODCALLTYPE FinalizeableObjectQueued(     /* [in] */ DWORD finalizerFlags,
                                                                    /* [in] */ ObjectID objectID){return S_OK;}

    HRESULT STDMETHODCALLTYPE RootReferences2(              /* [in] */ ULONG    cRootRefs,
                                                                    /* [in] */ ObjectID rootRefIds[],
                                                                    /* [in] */ COR_PRF_GC_ROOT_KIND rootKinds[],
                                                                    /* [in] */ COR_PRF_GC_ROOT_FLAGS rootFlags[],
                                                                    /* [in] */ UINT_PTR rootIds[]){return S_OK;}

    HRESULT STDMETHODCALLTYPE HandleCreated(                /* [in] */ GCHandleID handleId,
                                                                    /* [in] */ ObjectID initialObjectId){return S_OK;}

    HRESULT STDMETHODCALLTYPE HandleDestroyed(              /* [in] */ GCHandleID handleId){return S_OK;}

    HRESULT STDMETHODCALLTYPE SurvivingReferences(          /* [in] */ ULONG    cSurvivingObjectIDRanges,
                                                                    /* [in] */ ObjectID objectIDRangeStart[],
                                                                    /* [in] */ ULONG    cObjectIDRangeLength[]){return S_OK;}

	// The Info pointer (valid as long as the runtime is loaded).
	ICorProfilerInfo3 * m_pInfo;  

}; // ProfilerCallback

OBJECT_ENTRY_AUTO(CLSID_STRESS_CALLBACK, StressCallback);

class CBPDModule : public CAtlDllModuleT< CBPDModule >
{
public :
} _AtlModule;

// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow()
{
	return _AtlModule.DllCanUnloadNow();
}

// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer()
{
	return _AtlModule.DllRegisterServer(FALSE);
}


STDAPI DllUnregisterServer()
{
	return _AtlModule.DllUnregisterServer(FALSE);
}


