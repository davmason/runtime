// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// StressMemory.h
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

#define MAX_TEMPLATE_ARGS 10
#define LONG_LENGTH       1024

_COM_SMARTPTR_TYPEDEF(ICorProfilerInfo3, __uuidof(ICorProfilerInfo3));
_COM_SMARTPTR_TYPEDEF(IUnknown         , __uuidof(IUnknown));

// {C5F4E5DB-29D8-4a64-B161-9A59056232B5}
extern const GUID __declspec( selectany ) CLSID_STRESS_MEMORY = //{C5F4E5DB-29D8-4a64-B161-9A59056232B5}
{ 0xc5f4e5db, 0x29d8, 0x4a64, { 0xb1, 0x61, 0x9a, 0x59, 0x5, 0x62, 0x32, 0xb5 } };



/***************************************************************************************
 ********************                                               ********************
 ********************      StressMemoryDeclaration           ********************
 ********************                                               ********************
 ***************************************************************************************/

class StressMemory
  : public CComObjectRootEx<CComMultiThreadModel>,
    public CComCoClass<StressMemory, &CLSID_STRESS_MEMORY>,
    public ICorProfilerCallback3
{
public:
    DECLARE_REGISTRY_RESOURCE(StressMemory);

    DECLARE_CLASSFACTORY()
    BEGIN_COM_MAP(StressMemory)
        COM_INTERFACE_ENTRY_IID(IID_ICorProfilerCallback, ICorProfilerCallback)
        COM_INTERFACE_ENTRY_IID(IID_ICorProfilerCallback2,ICorProfilerCallback2)
        COM_INTERFACE_ENTRY_IID(IID_ICorProfilerCallback3,ICorProfilerCallback3)
    END_COM_MAP()

	StressMemory()
	{
		m_pInfo = NULL;
		m_oldObjectId = NULL;
		m_newObjectId = NULL;
		m_length = NULL;
		m_rootId = NULL;
		m_rootKind = COR_PRF_GC_ROOT_OTHER;
		m_rootFlag = COR_PRF_GC_ROOT_PINNING;

	}
	~StressMemory(){}

    HRESULT STDMETHODCALLTYPE Initialize(IUnknown *pProfilerInfoUnk)
	{
		HRESULT hr = S_OK;
		hr = pProfilerInfoUnk->QueryInterface(IID_ICorProfilerInfo3, (void **) & m_pInfo);
		if (FAILED(hr))	FAILURE(L"QI for ICorProfilerInfo3 Failed: %x", hr);
    
		m_pInfo->AddRef();
		hr= m_pInfo->SetEventMask(COR_PRF_MONITOR_GC
								  | COR_PRF_MONITOR_OBJECT_ALLOCATED
    							  | COR_PRF_ENABLE_OBJECT_ALLOCATED);
		if (FAILED(hr))	FAILURE(L"SetEventMask COR_PRF_MONITOR_GC Falied: %x", hr);

		DISPLAY(L"Stress Memory Profiler Initialized.");

		return hr;
	}

	VOID DoTest(ObjectID objectId, ClassID inClassId)
	{
		if (objectId == NULL) return;

		ClassID classId;
		ULONG cSize;
		COR_PRF_GC_GENERATION_RANGE range;
		HRESULT hr = S_OK;

		hr = m_pInfo->GetObjectSize(objectId, &cSize);
		if (FAILED(hr)) FAILURE(L"GetObjectSize returned %x", hr);

		hr = m_pInfo->GetObjectGeneration(objectId, &range);
		//GetObjectGeneration shoule NOT be called inside the callbacks.
		//Here it is just used for internal test.
		//The returned value is ignored.

		hr = m_pInfo->GetClassFromObject(objectId, &classId);
		//GetClassFromObject may fail
		
		if (SUCCEEDED(hr))
		{
			if (inClassId !=NULL && inClassId != classId) 
				FAILURE(L"GetClassFromObject returns different value from GC callbacks returned value.");
			
			DoTestClassID(classId);
		}
	}


	VOID DoTestClassID(ClassID classId)
	{
		if (classId == NULL) return;

		HRESULT hr = S_OK;

		CorElementType	BaseElemType;
		ClassID			BaseClassId = NULL;
		ULONG			cRank = NULL;
		hr = m_pInfo->IsArrayClass(classId, &BaseElemType, &BaseClassId, &cRank);
		//IsArrayClass would return S_FALSE, which is a success code (not a failure code).  
		//IsArrayClass will fail if you give it a NULL ClassID.
		
		COR_FIELD_OFFSET rFieldOffset[LONG_LENGTH];
		ULONG cFieldOffset  = NULL;
		ULONG ulClassSize   = NULL;
		hr = m_pInfo->GetClassLayout(classId, rFieldOffset, LONG_LENGTH, &cFieldOffset, &ulClassSize);
		//GetClassLayout may fail
		
		ModuleID ModuleId      = NULL;
		mdTypeDef TypeDefToken = NULL;
		ClassID ParentClassId  = NULL;
		ULONG32 cNumTypeArgs   = NULL;
        ClassID typeArgs[MAX_TEMPLATE_ARGS];
		m_pInfo->GetClassIDInfo2(classId, &ModuleId, &TypeDefToken, &ParentClassId, MAX_TEMPLATE_ARGS, &cNumTypeArgs, typeArgs);
		//GetClassIDInfo2 may fail
		
		ULONG32 BufferOffset = NULL;
		m_pInfo->GetBoxClassLayout(classId, &BufferOffset);
		//GetBoxClassLayout may fail is the class is NOT boxed
	}

    HRESULT STDMETHODCALLTYPE ObjectAllocated(              /* [in] */ ObjectID objectId,
                                                            /* [in] */ ClassID classId)
	{
		DoTest(objectId, classId);
		return S_OK;
	}

    HRESULT STDMETHODCALLTYPE ObjectsAllocatedByClass(      /* [in] */ ULONG cClassCount,
                                                            /* [in] */ ClassID classIds[  ],
                                                            /* [in] */ ULONG cObjects[  ])
	{
		for ( ULONG i = 0; i < cClassCount; ++i )
		{
			DoTestClassID(classIds[i]);
			m_length = cObjects[i];
		}
		return S_OK;
	
	}

	//NOTE:
	//Even though corprof.idl says that "None of the objectIDs returned by RootReferences2/ObjectReferences are valid during the callback",
	//it is supposed safe to be used here for internal tests
	HRESULT STDMETHODCALLTYPE RootReferences2(              /* [in] */ ULONG    cRootRefs,
                                                            /* [in] */ ObjectID rootRefIds[],
                                                            /* [in] */ COR_PRF_GC_ROOT_KIND rootKinds[],
                                                            /* [in] */ COR_PRF_GC_ROOT_FLAGS rootFlags[],
                                                            /* [in] */ UINT_PTR rootIds[])
	{
		for ( ULONG i = 0; i < cRootRefs; ++i )
		{
			m_rootKind = rootKinds[i];
			m_rootFlag = rootFlags[i];
			m_rootId = rootIds[i];
			//rootRefId might be zero...
			if (rootRefIds[i] != NULL)
				DoTest(rootRefIds[i], NULL);
		}
		return S_OK;
	}


	HRESULT STDMETHODCALLTYPE ObjectReferences(             /* [in] */ ObjectID objectId,
															/* [in] */ ClassID classId,
															/* [in] */ ULONG cObjectRefs,
															/* [in] */ ObjectID objectRefIds[  ])
	{
		DoTest(objectId, classId);
		for ( ULONG i = 0; i < cObjectRefs; ++i )
		{
			DoTest(objectRefIds[i], NULL);
		}
		return S_OK;
	}

	//NOTE:
	//"None of the objectIDs returned by MovedReferences/SurvivingReferences are valid during the callback",
	//It is REALLY not valid. 
	//So just try to read the returned value, guarantee the value is readable.
	HRESULT STDMETHODCALLTYPE MovedReferences(              /* [in] */ ULONG cMovedObjectIDRanges,
                                                            /* [in] */ ObjectID oldObjectIDRangeStart[  ],
                                                            /* [in] */ ObjectID newObjectIDRangeStart[  ],
                                                            /* [in] */ ULONG cObjectIDRangeLength[  ])
	{
		for ( ULONG i = 0; i < cMovedObjectIDRanges; ++i )
		{
			m_oldObjectId = oldObjectIDRangeStart[i];
			m_newObjectId = newObjectIDRangeStart[i];
			m_length = cObjectIDRangeLength[i];
		}
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE SurvivingReferences(          /* [in] */ ULONG    cSurvivingObjectIDRanges,
                                                            /* [in] */ ObjectID objectIDRangeStart[],
                                                            /* [in] */ ULONG    cObjectIDRangeLength[])
	{
		for ( ULONG i = 0; i < cSurvivingObjectIDRanges; ++i )
		{
			m_newObjectId = objectIDRangeStart[i];
			m_length = cObjectIDRangeLength[i];
		}
		return S_OK;
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

    HRESULT STDMETHODCALLTYPE HandleCreated(                /* [in] */ GCHandleID handleId,
                                                                    /* [in] */ ObjectID initialObjectId){return S_OK;}

    HRESULT STDMETHODCALLTYPE HandleDestroyed(              /* [in] */ GCHandleID handleId){return S_OK;}

	// The Info pointer (valid as long as the runtime is loaded).
	ICorProfilerInfo3 * m_pInfo;  
	ObjectID m_oldObjectId;
	ObjectID m_newObjectId;
	ULONG m_length;
	COR_PRF_GC_ROOT_KIND m_rootKind;
	COR_PRF_GC_ROOT_FLAGS m_rootFlag;
	UINT_PTR m_rootId;

}; // ProfilerCallback

OBJECT_ENTRY_AUTO(CLSID_STRESS_MEMORY, StressMemory);

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


