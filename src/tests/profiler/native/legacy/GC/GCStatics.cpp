// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
//
// GCStatics.cpp
//
// Large part of this test case is "shamelssly" stolen from GCAttach.cpp.
// The important change is that when GC finsihed, GetStaticAddress will be called on every surived objects (moved objects)
// GetStaticAddress will check every field of the object, if it is a static field, then call the corespoding API depend on its type:
// which would be one of thread static, context static, appdomaon static or RVA static
// ======================================================================================
#include "GCCommon.h"
#include "../LegacyCompat.h"

typedef vector<ThreadID> ThreadPool;
typedef vector<AppDomainID> AppDoaminPool;
typedef vector<ClassID> ClassPool;

#define MAX_OBJECT_RANGES 0x100
#define THREADSTATIC L"myThreadStaticTestVar"
#define APPDOMAINSTATIC L"myAppdomainStaticTestVar"
#define CONTEXTSTATIC L"myContextStaticTestVar"
#define RVASTATIC L"myRVAStaticTestVar"

#pragma region GC Structures

#define GCSTATICS_FAILURE(message)       \
    {                                    \
        m_ulFailure++;                   \
        FAILURE(message)                 \
    }                                       

#define GCATTACH_MUST_PASS(call)                                                \
    {                                                                           \
        HRESULT __hr = (call);                                                  \
        if( FAILED(__hr) )                                                      \
        {                                                                       \
            FAILURE(L"\nCall '" << #call << L"' failed with HR=" << HEX(__hr)   \
                    << L"\nIn " << __FILE__ << L" at line " << __LINE__);       \
            m_ulFailure++;                                                      \
            return __hr;                                                        \
        }                                                                       \
    }

   struct GenerationInfo
    {
        ULONG nObjectRanges;
        COR_PRF_GC_GENERATION_RANGE objectRanges[MAX_OBJECT_RANGES];
    };

    struct SurviveRefInfo
    {
        ULONG      cSurvivingObjectIDRanges;
        ObjectID * objectIDRangeStart;
        ULONG    * cObjectIDRangeLength;
    };
    typedef vector<SurviveRefInfo *> SurviveRefInfoVector;

    struct RootRefInfo
    {
        ULONG                   cRootRefs;
        ObjectID              * rootRefIds;
        COR_PRF_GC_ROOT_KIND  * rootKinds;
        COR_PRF_GC_ROOT_FLAGS * rootFlags;
        UINT_PTR              * rootIds;
    };
    typedef vector<RootRefInfo *> RootRefInfoVector;

    struct ObjectRefInfo
    {
        ObjectID   objectId;
        ClassID    classId;
        ULONG      cObjectRefs;
        ObjectID * objectRefIds;
    };
    typedef vector<ObjectRefInfo *> ObjectRefInfoVector;

    struct MovedObjectInfo
    {
        ULONG      cMovedObjectIDRanges;
        ObjectID * oldObjectIDRangeStart;
        ObjectID * newObjectIDRangeStart;
        ULONG    * cObjectIDRangeLength;
    };
    typedef vector<MovedObjectInfo *> MovedObjectInfoVector;

    typedef vector<ObjectID> ObjectIDVector;

    enum STATIC_TYPE{THREAD_STATIC_1, THREAD_STATIC_2, APPDOMAIN_STATIC};

#pragma endregion // GC Structures

class GCStatics
{
    public:

        GCStatics(IPrfCom * pPrfCom);
        ~GCStatics();

        #pragma region static_wrapper_methods

        static HRESULT FinalizeableObjectQueuedWrapper(IPrfCom * pPrfCom,
                                                       DWORD finalizerFlags,
                                                       ObjectID objectID)
        {
            return STATIC_CLASS_CALL(GCStatics)->FinalizeableObjectQueued(pPrfCom, finalizerFlags, objectID);
        }

        static HRESULT GarbageCollectionFinishedWrapper(IPrfCom * pPrfCom)
        {
            return STATIC_CLASS_CALL(GCStatics)->GarbageCollectionFinished(pPrfCom);
        }

        static HRESULT GarbageCollectionStartedWrapper(IPrfCom * pPrfCom,
                                                       INT cGenerations,
                                                       BOOL generationCollected[],
                                                       COR_PRF_GC_REASON reason)
        {
            return STATIC_CLASS_CALL(GCStatics)->GarbageCollectionStarted(pPrfCom, cGenerations, generationCollected, reason);
        }

        static HRESULT HandleCreatedWrapper(IPrfCom * pPrfCom,
                                            GCHandleID handleId,
                                            ObjectID initialObjectId)
        {
            return STATIC_CLASS_CALL(GCStatics)->HandleCreated(pPrfCom, handleId, initialObjectId);
        }

        static HRESULT HandleDestroyedWrapper(IPrfCom * pPrfCom,
                                              GCHandleID handleId)
        {
            return STATIC_CLASS_CALL(GCStatics)->HandleDestroyed(pPrfCom, handleId);
        }

        static HRESULT MovedReferencesWrapper(IPrfCom * pPrfCom,
                                              ULONG cMovedObjectIDRanges,
                                              ObjectID oldObjectIDRangeStart[  ],
                                              ObjectID newObjectIDRangeStart[  ],
                                              ULONG cObjectIDRangeLength[  ])
        {
            return STATIC_CLASS_CALL(GCStatics)->MovedReferences(pPrfCom, cMovedObjectIDRanges, oldObjectIDRangeStart, newObjectIDRangeStart, cObjectIDRangeLength);
        }

        static HRESULT ObjectAllocatedWrapper(IPrfCom * pPrfCom,
                                              ObjectID objectId,
                                              ClassID classId)
        {
            return STATIC_CLASS_CALL(GCStatics)->ObjectAllocated(pPrfCom, objectId, classId);
        }

        static HRESULT ObjectReferencesWrapper(IPrfCom * pPrfCom,
                                               ObjectID objectId,
                                               ClassID classId,
                                               ULONG cObjectRefs,
                                               ObjectID objectRefIds[  ])
        {
            return STATIC_CLASS_CALL(GCStatics)->ObjectReferences(pPrfCom, objectId, classId, cObjectRefs, objectRefIds);
        }

        static HRESULT RootReferences2Wrapper(IPrfCom * pPrfCom,
                                              ULONG cRootRefs,
                                              ObjectID rootRefIds[],
                                              COR_PRF_GC_ROOT_KIND rootKinds[],
                                              COR_PRF_GC_ROOT_FLAGS rootFlags[],
                                              UINT_PTR rootIds[])
        {
            return STATIC_CLASS_CALL(GCStatics)->RootReferences2(pPrfCom, cRootRefs, rootRefIds, rootKinds, rootFlags, rootIds);
        }

        static HRESULT SurvivingReferencesWrapper(IPrfCom * pPrfCom,
                                                  ULONG cSurvivingObjectIDRanges,
                                                  ObjectID objectIDRangeStart[],
                                                  ULONG cObjectIDRangeLength[])
        {
            return STATIC_CLASS_CALL(GCStatics)->SurvivingReferences(pPrfCom, cSurvivingObjectIDRanges, objectIDRangeStart, cObjectIDRangeLength);
        }

        static HRESULT ThreadCreatedWrapper(IPrfCom * pPrfCom,
                    ThreadID managedThreadId)
        {
            return STATIC_CLASS_CALL(GCStatics)->ThreadCreated(pPrfCom, managedThreadId);
        }   

        static HRESULT AppDomainCreationFinishedWrapper(IPrfCom * pPrfCom,
                    AppDomainID appDomainId,
                    HRESULT     hrStatus)
        {
            return STATIC_CLASS_CALL(GCStatics)->AppDomainCreationFinished(pPrfCom, appDomainId, hrStatus);    
        }

        static HRESULT AppDomainShutdownStartedWrapper(IPrfCom * pPrfCom,
                AppDomainID appDomainId)
        {
            return STATIC_CLASS_CALL(GCStatics)->AppDomainShutdownStarted(pPrfCom, appDomainId);  
        }

        static HRESULT ThreadDestroyedWrapper(IPrfCom * pPrfCom,
                        ThreadID managedThreadId)
        {
            return STATIC_CLASS_CALL(GCStatics)->ThreadDestroyed(pPrfCom, managedThreadId);
        }
         static HRESULT ThreadNameChangedWrapper(IPrfCom * pPrfCom,
                ThreadID threadId,
                ULONG cchName,
                PROFILER_WCHAR name[])
         {
            return STATIC_CLASS_CALL(GCStatics)->ThreadNameChanged(pPrfCom, threadId, cchName, name);
         }

        static HRESULT ClassLoadFinishedWrapper(IPrfCom * pPrfCom, ClassID classID,HRESULT hrStatus)
        {
            return STATIC_CLASS_CALL(GCStatics)->ClassLoadFinished(pPrfCom, classID, hrStatus);
        }

        static HRESULT ClassUnloadStartedWrapper(IPrfCom * pPrfCom, ClassID classId)
        {
            return STATIC_CLASS_CALL(GCStatics)->ClassUnloadStarted(pPrfCom, classId);
        }
        static HRESULT VerifyWrapper(IPrfCom * pPrfCom)
        {
            return STATIC_CLASS_CALL(GCStatics)->Verify(pPrfCom);
        }


        #pragma endregion // static_wrapper_methods

        HRESULT GCStatics::Verify(IPrfCom *  pPrfCom );

    private:

        void GCStatics::AlignObject(IPrfCom * pPrfCom, ObjectID * pObjectID);
        COR_PRF_GC_GENERATION GCStatics::GetGeneration(IPrfCom * pPrfCom, ObjectID objectID);
        HRESULT GCStatics::VerifyMetadata(IPrfCom * pPrfCom, ClassID classID);
        HRESULT GCStatics::VerifyGenerations(IPrfCom * pPrfCom);
        HRESULT GCStatics::VerifyMovedReferences(IPrfCom * pPrfCom, MovedObjectInfo * pMovedRefs);
        HRESULT GCStatics::VerifyObjectReferences(IPrfCom * pPrfCom, ObjectRefInfo * pObjectRefInfo);
        HRESULT GCStatics::VerifyRootReferences(IPrfCom * pPrfCom, RootRefInfo * pRootRefs);
        HRESULT GCStatics::VerifySurvivingReferences(IPrfCom * pPrfCom, SurviveRefInfo * pSurviveRefInfo);

        #pragma region callback_handler_prototypes

        HRESULT GCStatics::FinalizeableObjectQueued(IPrfCom *  pPrfCom ,
                                                   DWORD  finalizerFlags ,
                                                   ObjectID  objectID );

        HRESULT GCStatics::GarbageCollectionFinished(IPrfCom *  pPrfCom );

        HRESULT GCStatics::GarbageCollectionStarted(IPrfCom *  pPrfCom ,
                                                   INT  cGenerations ,
                                                   BOOL  generationCollected[] ,
                                                   COR_PRF_GC_REASON  reason );

        HRESULT GCStatics::HandleCreated(IPrfCom *  pPrfCom ,
                                        GCHandleID  handleId ,
                                        ObjectID  initialObjectId );

        HRESULT GCStatics::HandleDestroyed(IPrfCom *  pPrfCom ,
                                          GCHandleID  handleId );

        HRESULT GCStatics::MovedReferences(IPrfCom *  pPrfCom ,
                                          ULONG  cMovedObjectIDRanges ,
                                          ObjectID  oldObjectIDRangeStart[  ] ,
                                          ObjectID  newObjectIDRangeStart[  ] ,
                                          ULONG  cObjectIDRangeLength[  ] );

        HRESULT GCStatics::ObjectAllocated(IPrfCom *  pPrfCom ,
                                          ObjectID  objectId ,
                                          ClassID  classId );

        HRESULT GCStatics::ObjectReferences(IPrfCom *  pPrfCom ,
                                           ObjectID  objectId ,
                                           ClassID  classId ,
                                           ULONG  cObjectRefs ,
                                           ObjectID  objectRefIds[  ] );

        HRESULT GCStatics::RootReferences2(IPrfCom *  pPrfCom ,
                                          ULONG  cRootRefs ,
                                          ObjectID  rootRefIds[] ,
                                          COR_PRF_GC_ROOT_KIND  rootKinds[] ,
                                          COR_PRF_GC_ROOT_FLAGS  rootFlags[] ,
                                          UINT_PTR  rootIds[] );

        HRESULT GCStatics::SurvivingReferences(IPrfCom *  pPrfCom ,
                                              ULONG  cSurvivingObjectIDRanges ,
                                              ObjectID  objectIDRangeStart[] ,
                                              ULONG  cObjectIDRangeLength[] );

        

        HRESULT ThreadCreated(IPrfCom * pPrfCom, ThreadID managedThreadId);
        
        HRESULT ThreadDestroyed(IPrfCom * pPrfCom, ThreadID managedThreadId);

        HRESULT AppDomainCreationFinished(IPrfCom * pPrfCom, AppDomainID appDomainId,HRESULT     hrStatus);
        HRESULT AppDomainShutdownStarted(IPrfCom * pPrfCom, AppDomainID appDomainId);  
        HRESULT ThreadNameChanged(IPrfCom * pPrfCom, ThreadID threadId, ULONG cchName, PROFILER_WCHAR name[]);
        HRESULT ClassLoadFinished(IPrfCom * pPrfCom, ClassID classID,HRESULT hrStatus);
        HRESULT ClassUnloadStarted(IPrfCom * pPrfCom, ClassID classId);

        #pragma endregion // callback_handler_prototypes


        #pragma region GetStaticAddressFunctions
        HRESULT GCStatics::GetContextStaticAddress(IPrfCom * pPrfCom, ClassID classId, mdFieldDef fieldToken, ThreadID threadId, PLATFORM_WCHAR tokenName[]);
        HRESULT GCStatics::GetThreadStaticAddress(IPrfCom * pPrfCom, ClassID classId, mdFieldDef fieldToken, AppDomainID AppdomainId, PLATFORM_WCHAR tokenName[]);
        HRESULT GCStatics::GetThreadStaticAddress2(IPrfCom * pPrfCom, ClassID classId, mdFieldDef fieldToken, AppDomainID AppdomainId, ThreadID threadId, PLATFORM_WCHAR tokenName[]);
        HRESULT GCStatics::GetAppdomainStaticAddress(IPrfCom * pPrfCom, ClassID classId, mdFieldDef fieldToken, AppDomainID AppdomainId, PLATFORM_WCHAR tokenName[]);
        HRESULT GCStatics::GetRVAStaticAddress(IPrfCom * pPrfCom, ClassID classId, mdFieldDef fieldToken, PLATFORM_WCHAR tokenName[]);
        HRESULT GCStatics::GetStaticAddress(IPrfCom * pPrfCom, ClassID classId, STATIC_TYPE type);
        #pragma region

        atomic<LONG> m_ulTotalGCOperations;

        // a bit mask that indicates what generations are collected
        // e.g. 0xf: full GC; 0x1: only collects generation 0.
        DWORD m_dwGenerationsCollected;

        /*  Success/Failure counters */
        ULONG m_ulSuccess;
        ULONG m_ulFailure;

        IPrfCom * m_pPrfCom;

        ObjectIDVector        m_vFinalizeQueued;
        MovedObjectInfoVector m_vMovedReferences;
        ObjectRefInfoVector   m_vObjectReferences;
        RootRefInfoVector     m_vRootReferences;
        SurviveRefInfoVector  m_vSurvivingReferences;
        GenerationInfo        m_generationInfo;

        LONG m_cSurvivingReferencesCalls;
        LONG m_cFinalizeableObjectQueuedCalls;
        LONG m_cGCFinishedCalls;
        LONG m_cGCStartedCalls;
        LONG m_cHandleCreatedCalls;
        LONG m_cHandleDestroyedCalls;
        LONG m_cMovedReferencesCalls;
        LONG m_cObjectReferencesCalls;
        LONG m_cRootReferencesCalls;

        ThreadPool tPool;
        AppDoaminPool adPool;

        ULONG m_ulLiterals;
        ULONG m_ulErrorArgument;
        ULONG m_ulGetAppdomainStaticAddress;
        ULONG m_ulGetThreadStaticAddress2;
        ULONG m_ulGetThreadStaticAddress;
        ULONG m_ulDataIncomplete;
        ULONG m_ulGetContextStaticAddress;
        ULONG m_ulGetRVAStaticAddress;
        ULONG m_ulNewThreadInit;
        ULONG m_ulSOKWithZeroValue;
        ThreadID m_ThreadStaticTestThreadId;
        ClassID m_staticClassId;
};

//insert every managed threadid into threadpool
HRESULT GCStatics::ThreadCreated(IPrfCom * pPrfCom, ThreadID managedThreadId)
{
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    DISPLAY(L"managedThread "<<HEX(managedThreadId)<<L" Created.");
    for(ULONG i=0;i<tPool.size();++i)
    {
        if (tPool.at(i) == managedThreadId)
        {
            return S_OK;
        }
    }
    tPool.push_back(managedThreadId);
    HRESULT hr = S_OK;
    if (m_staticClassId != 0) hr = GetStaticAddress(pPrfCom, m_staticClassId, THREAD_STATIC_1);
    return hr;  
}

HRESULT GCStatics::ThreadDestroyed(IPrfCom * pPrfCom, ThreadID managedThreadId)
{
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    DISPLAY(L"managedThread "<<HEX(managedThreadId)<<L" Destroyed.");
    for(ULONG i=0;i<tPool.size();++i)
    {
        if (tPool.at(i) == managedThreadId)
        {
            tPool[i] = 0;
            return S_OK;
        }
    }
    return S_OK;
}

//insert every appdomain into appdomainpool
HRESULT GCStatics::AppDomainCreationFinished(IPrfCom * pPrfCom, AppDomainID appDomainId,HRESULT     hrStatus)
{
    if (FAILED(hrStatus))return S_OK;
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    DISPLAY(L"AppDomain "<<HEX(appDomainId)<<L" Created.");
    for(ULONG i=0;i<adPool.size();++i)
    {
        if (adPool.at(i) == appDomainId)
        {
            return S_OK;
        }
    }
    adPool.push_back(appDomainId);
    HRESULT hr = S_OK;
    if (m_staticClassId != 0) hr = GetStaticAddress(pPrfCom, m_staticClassId, APPDOMAIN_STATIC);
    return hr;    
    
}
HRESULT GCStatics::AppDomainShutdownStarted(IPrfCom * pPrfCom, AppDomainID appDomainId)
{
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    DISPLAY(L"AppDomain "<<HEX(appDomainId)<<L" Shutdown.");
    for(ULONG i=0;i<adPool.size();++i)
    {
        if (adPool.at(i) == appDomainId)
        {
            adPool[i] = 0;
            return S_OK;
        }
    }
    return S_OK;
}

HRESULT GCStatics::ThreadNameChanged(IPrfCom * pPrfCom, ThreadID threadId, ULONG cchName, PROFILER_WCHAR name[])
{
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

	PLATFORM_WCHAR nameTemp[STRING_LENGTH];

	ConvertProfilerWCharToPlatformWChar(nameTemp, STRING_LENGTH, name, STRING_LENGTH);

    DISPLAY(L"ThreadNameChanged "<< HEX(threadId) << L" " <<nameTemp);
    PLATFORM_WCHAR threadName[]= L"myThreadStaticTestThread";
	if (wcsncmp(nameTemp, threadName, cchName) == 0) 
    {
        assert(cchName == wcslen(threadName));
        m_ThreadStaticTestThreadId = threadId;
    }
    return S_OK;
}

HRESULT GCStatics::ClassLoadFinished(IPrfCom * pPrfCom,
                                    ClassID classId,
                                    HRESULT hrStatus)
{
    if (FAILED(hrStatus)) return S_OK;
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    mdTypeDef token = NULL;
    ModuleID classModuleId = NULL;
    COMPtrHolder<IMetaDataImport> pIMDImport;
    HRESULT hr = S_OK;
    MUST_PASS(PINFO->GetClassIDInfo2(classId, &classModuleId, &token, NULL, NULL, NULL, NULL));
    MUST_PASS(PINFO->GetModuleMetaData(classModuleId, ofRead, IID_IMetaDataImport, (IUnknown **)&pIMDImport));
    
    PROFILER_WCHAR tokenNameTemp[STRING_LENGTH];
    PLATFORM_WCHAR tokenName[STRING_LENGTH];
    PLATFORM_WCHAR className[]= L"MyStaticClass";

    MUST_PASS(pIMDImport->GetTypeDefProps(token, tokenNameTemp, 256, NULL, NULL, NULL));
	ConvertProfilerWCharToPlatformWChar(tokenName, STRING_LENGTH, tokenNameTemp, STRING_LENGTH);
    DISPLAY(L"Class "<< tokenName << L" LoadFinished");
	if (wcsncmp(className, tokenName, wcslen(className)) == 0)
    {
        m_staticClassId = classId;
        hr = GetStaticAddress(pPrfCom, classId, THREAD_STATIC_1);
    }
    return S_OK;
} 
HRESULT GCStatics::ClassUnloadStarted(IPrfCom * pPrfCom, ClassID classId)
{
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    mdTypeDef token = NULL;
    ModuleID classModuleId = NULL;
    COMPtrHolder<IMetaDataImport> pIMDImport;
    HRESULT hr = S_OK;
    MUST_PASS(PINFO->GetClassIDInfo2(classId, &classModuleId, &token, NULL, NULL, NULL, NULL));
    MUST_PASS(PINFO->GetModuleMetaData(classModuleId, ofRead, IID_IMetaDataImport, (IUnknown **)&pIMDImport));
    
    WCHAR tokenName[256];
    MUST_PASS(pIMDImport->GetTypeDefProps(token, tokenName, 256, NULL, NULL, NULL));
    DISPLAY(L"Class "<< tokenName << " UnLoad Started");
   
    return hr;
}

GCStatics::GCStatics(IPrfCom * pPrfCom)
{
    // Initialize result counters
    m_ulSuccess = 0;
    m_ulFailure = 0;
    m_cSurvivingReferencesCalls = 0;
    m_cFinalizeableObjectQueuedCalls = 0;
    m_cGCFinishedCalls = 0;
    m_cGCStartedCalls = 0;
    m_cHandleCreatedCalls = 0;
    m_cHandleDestroyedCalls = 0;
    m_cMovedReferencesCalls = 0;
    m_cObjectReferencesCalls = 0;
    m_cRootReferencesCalls = 0;
    m_ulLiterals = 0;
    m_ulErrorArgument = 0;
    m_staticClassId = 0;
    m_ulGetAppdomainStaticAddress=0;
    m_ulGetThreadStaticAddress2=0;
    m_ulGetThreadStaticAddress=0;
    m_ulGetContextStaticAddress = 0;
    m_ulGetRVAStaticAddress=0;
    m_ulDataIncomplete=0;
    m_ulNewThreadInit=0;
    m_ulSOKWithZeroValue=0;
    memset(&m_generationInfo, 0, sizeof(m_generationInfo));

    m_pPrfCom = pPrfCom;
}


/*
 *  Clean up
 */
GCStatics::~GCStatics()
{
    //ToDo: Kill Unmanaged Thread
}


HRESULT GCStatics::GetContextStaticAddress(IPrfCom * pPrfCom, ClassID classId, mdFieldDef fieldToken, ThreadID threadId, PLATFORM_WCHAR tokenName[])
{
    m_ulGetContextStaticAddress++;
    PVOID staticAddress=0;
    HRESULT hr = S_OK;
    ContextID contextId;
    hr = PINFO->GetThreadContext(threadId, &contextId);
    if (FAILED(hr)|| (void*)contextId == NULL)
        DISPLAY(L"Failed get context for thread "<< HEX(threadId));
    hr = PINFO->GetContextStaticAddress(classId, fieldToken, contextId, &staticAddress);
    if (FAILED(hr)) 
    {
        if (hr==COR_E_ARGUMENT || hr==CORPROF_E_NOT_MANAGED_THREAD)m_ulErrorArgument++;
        else if (hr==CORPROF_E_DATAINCOMPLETE)m_ulDataIncomplete++;
        else GCSTATICS_FAILURE(L"FAILED GetContextStaticAddress "<<HEX(hr));
    }
    else if (wcsncmp(tokenName, CONTEXTSTATIC, wcslen(CONTEXTSTATIC))==0)
    {
        if (staticAddress==0) 
        {
            m_ulSOKWithZeroValue++;
            return S_OK;
        }
        PULONG ptr =  static_cast<PULONG>(staticAddress);  
        if (*ptr != 0xBCD)
        {
             GCSTATICS_FAILURE(L"Get Context Static HR: "<< HEX(hr) <<L" Thread "<< HEX(threadId)<< L" address: "<<HEX(staticAddress)<< L" value " << HEX(*ptr));    
        }
        else
        {
            m_ulSuccess++;
            DISPLAY(L"Get Context Static HR: "<< HEX(hr) <<L" Thread "<< HEX(threadId)<< L" address: "<<HEX(staticAddress)<< L" value " << HEX(*ptr));
        }
    }
    return S_OK;
}

HRESULT GCStatics::GetThreadStaticAddress(IPrfCom * pPrfCom, ClassID classId, mdFieldDef fieldToken, ThreadID threadId, PLATFORM_WCHAR tokenName[])
{
    m_ulGetThreadStaticAddress++;
    PVOID staticAddress=0;
    HRESULT hr =S_OK;
    hr = PINFO->GetThreadStaticAddress(classId, fieldToken, threadId, &staticAddress);

    if (FAILED(hr)) 
    {
        if (hr==COR_E_ARGUMENT || hr==CORPROF_E_NOT_MANAGED_THREAD)m_ulErrorArgument++;
        else if (hr==CORPROF_E_DATAINCOMPLETE)m_ulDataIncomplete++;
        else GCSTATICS_FAILURE(L"FAILED GetThreadStaticAddress "<<HEX(hr));
    }
    else if (wcsncmp(tokenName, THREADSTATIC, wcslen(THREADSTATIC))==0)
    {
        if (staticAddress==0)
        {
            m_ulSOKWithZeroValue++;
            return S_OK;
        }
        PULONG ptr =  static_cast<PULONG>(staticAddress);  

        if (*ptr == 0x0)
        {
            *ptr = 0xABC;
            m_ulNewThreadInit++;
        }
        else if (*ptr != 0xABC)
        {
             GCSTATICS_FAILURE("FAILED GetThreadStaticAddress returned error evalue "<<HEX(*ptr));    
        }
        else
        {
            m_ulSuccess++;
            DISPLAY(L"Get Thread Static HR: "<< HEX(hr) <<L" Thread "<< HEX(threadId)<< L" address: "<<HEX(staticAddress)<< L" value " << HEX(*ptr));
        }
    }
    return S_OK;
}

HRESULT GCStatics::GetThreadStaticAddress2(IPrfCom * pPrfCom, ClassID classId, mdFieldDef fieldToken, AppDomainID AppdomainId, ThreadID threadId, PLATFORM_WCHAR tokenName[])
{
    m_ulGetThreadStaticAddress2++;
    PVOID staticAddress=0;
    HRESULT hr=S_OK;
    hr = PINFO->GetThreadStaticAddress2(classId, fieldToken, AppdomainId, threadId, &staticAddress);
    if (FAILED(hr)) 
    {
        if (hr==COR_E_ARGUMENT || hr==CORPROF_E_NOT_MANAGED_THREAD)m_ulErrorArgument++;
        else if (hr==CORPROF_E_DATAINCOMPLETE)m_ulDataIncomplete++;
        else GCSTATICS_FAILURE(L"FAILED GetThreadStaticAddress2 "<<HEX(hr));
        
    }
    
    else if (wcsncmp(tokenName, THREADSTATIC, wcslen(THREADSTATIC))==0)
    {
        if (staticAddress==0)
        {
            m_ulSOKWithZeroValue++;
            return S_OK;
        }
        PULONG ptr =  static_cast<PULONG>(staticAddress);  
        if (*ptr == 0) 
        {
            *ptr = 0xABC;
            m_ulNewThreadInit++;
        }
        else if (*ptr != 0xABC)
        {
            GCSTATICS_FAILURE(L"Get Thread Static 2 HR: "<< HEX(hr) <<L" Thread "<< HEX(threadId)<< L" AddDomain " << HEX(AppdomainId) << L" address: "<<HEX(staticAddress)<< L" value " << HEX(*ptr));    
        }
        else
        {
            m_ulSuccess++;
            DISPLAY(L"Get Thread Static 2 HR: "<< HEX(hr) <<L" Thread "<< HEX(threadId)<< L" AddDomain " << HEX(AppdomainId) << L" address: "<<HEX(staticAddress)<< L" value " << HEX(*ptr));
        }
    }
    return S_OK;
}

HRESULT GCStatics::GetAppdomainStaticAddress(IPrfCom * pPrfCom, ClassID classId, mdFieldDef fieldToken, AppDomainID AppdomainId, PLATFORM_WCHAR tokenName[])
{

    m_ulGetAppdomainStaticAddress++;
    PVOID staticAddress = 0;
    HRESULT hr = PINFO->GetAppDomainStaticAddress(classId, fieldToken, AppdomainId, &staticAddress);
    if (FAILED(hr)) 
    {
        if (hr==COR_E_ARGUMENT || hr==CORPROF_E_LITERALS_HAVE_NO_ADDRESS)m_ulErrorArgument++;
        else if (hr==CORPROF_E_DATAINCOMPLETE)m_ulDataIncomplete++;
        else GCSTATICS_FAILURE(L"FAILED GetAppdomainStaticAddress "<<HEX(hr));
    }
    else if (wcsncmp(tokenName, APPDOMAINSTATIC, wcslen(APPDOMAINSTATIC))==0)
    {
        if (staticAddress==0)
        {
            m_ulSOKWithZeroValue++;
            return S_OK;
        }
        PULONG ptr =  static_cast<PULONG>(staticAddress);  
        if (*ptr != 0xDEF)
        {
             GCSTATICS_FAILURE(L"Get Appdomain Static HR: "<< HEX(hr) << L" AddDomain " << HEX(AppdomainId) << L" address: "<<HEX(staticAddress)<<L" value "<<HEX(*ptr));    
        }
        else
        {
            m_ulSuccess++;
            DISPLAY(L"Get Appdomain Static HR: "<< HEX(hr) << L" AddDomain " << HEX(AppdomainId) << L" address: "<<HEX(staticAddress)<<L" value "<<HEX(*ptr));
        }
    }
    return S_OK;
   
}


HRESULT GCStatics::GetRVAStaticAddress(IPrfCom * pPrfCom,
                                      ClassID classId,
                                      mdFieldDef fieldToken,
                                      PLATFORM_WCHAR tokenName[])
{
    m_ulGetRVAStaticAddress++;
    PVOID staticAddress = 0;
    HRESULT hr;
    hr = PINFO->GetRVAStaticAddress(classId, fieldToken, &staticAddress);
    if (FAILED(hr)) 
    {
        if (hr==COR_E_ARGUMENT || hr==CORPROF_E_NOT_MANAGED_THREAD)m_ulErrorArgument++;
        else if (hr==CORPROF_E_DATAINCOMPLETE)m_ulDataIncomplete++;
        else GCSTATICS_FAILURE(L"FAILED GetRVAStaticAddress "<<HEX(hr));
    }
    else if (wcsncmp(tokenName, RVASTATIC, wcslen(RVASTATIC))==0)
    {
        PULONG ptr =  static_cast<PULONG>(staticAddress);  
        if (*ptr != 0xCDE)
        {
             GCSTATICS_FAILURE(L"Get RVA Static HR: "<< HEX(hr) << L" address: "<<HEX(staticAddress)<<L" value "<<HEX(*ptr));    
        }
        else
        {
            m_ulSuccess++;
            DISPLAY(L"Get RVA Static HR: "<< HEX(hr) << L" address: "<<HEX(staticAddress)<<L" value "<<HEX(*ptr));
        }
    }
    return S_OK;
}

HRESULT GCStatics::GetStaticAddress(IPrfCom * pPrfCom, ClassID classId, STATIC_TYPE type)
{
    HRESULT hr=S_OK;
    ThreadID    threadId    = NULL;
    AppDomainID AppdomainId = NULL;
    mdFieldDef fieldTokens[STRING_LENGTH];
    HCORENUM hEnum  = NULL;
    mdTypeDef token = NULL;
    ULONG cTokens   = NULL;

    ModuleID classModuleId = NULL;
    COMPtrHolder<IMetaDataImport> pIMDImport;

    ULONG nameLength = NULL;
    DWORD corElementType = NULL;
    PCCOR_SIGNATURE pvSig = NULL;
    ULONG cbSig = NULL;
    DWORD fieldAttributes = NULL;

    PROFILER_WCHAR tokenNameTemp[STRING_LENGTH];
    PLATFORM_WCHAR tokenName[STRING_LENGTH];
        
    hr = PINFO->GetClassIDInfo2(classId, &classModuleId, &token, NULL, NULL, NULL, NULL);
    if (FAILED(hr))
    {
        return hr;
    }
    hr = PINFO->GetModuleMetaData(classModuleId, ofRead, IID_IMetaDataImport, (IUnknown **)&pIMDImport);
    if (FAILED(hr))
    {
        return hr;
    }

    MUST_PASS(pIMDImport->GetTypeDefProps(token, tokenNameTemp, 256, NULL, NULL, NULL));
    MUST_PASS(pIMDImport->EnumFields(&hEnum, token, fieldTokens, STRING_LENGTH, &cTokens));

    for (ULONG i = 0; i < cTokens; i++)
    {
        ClassID fieldClassId = NULL;
        mdTypeDef fieldClassToken;
        MUST_PASS(pIMDImport->GetFieldProps(fieldTokens[i],    // The field for which to get props.
                                            &fieldClassToken,  // Put field's class here.
                                            tokenNameTemp,         // Put field's name here.
                                            256,               // Size of szField buffer in wide chars.
                                            &nameLength,       // Put actual size here
                                            &fieldAttributes,  // Put flags here.
                                            &pvSig,            // [OUT] point to the blob value of meta data
                                            &cbSig,            // [OUT] actual size of signature blob
                                            &corElementType,   // [OUT] flag for value type. selected ELEMENT_TYPE_*
                                            NULL,              // [OUT] constant value
                                            NULL));            // [OUT] size of constant value, string only, wide chars

		ConvertProfilerWCharToPlatformWChar(tokenName, STRING_LENGTH, tokenNameTemp, STRING_LENGTH);

        if(IsFdStatic(fieldAttributes))
        {
            if (S_OK == pIMDImport->GetCustomAttributeByName(fieldTokens[i], PROFILER_STRING("System.ThreadStaticAttribute"), NULL, NULL))
            {
                for(ULONG t =0;t<tPool.size();++t)
                {
                    threadId = tPool.at(t);
                    if (threadId == 0) continue;
                    //GetThreadStaticAddress only for current thread ID
                    hr = GetThreadStaticAddress(pPrfCom, classId, fieldTokens[i], threadId, tokenName);
                    
                    for (ULONG ad = 0;ad<adPool.size();++ad)
                    {
                        AppdomainId = adPool.at(ad);
                        if (AppdomainId == 0) continue;
            
                        // GetThreadStaticAddress2 per thread per appdomain
                        hr = GetThreadStaticAddress2(pPrfCom, classId, fieldTokens[i], AppdomainId, threadId, tokenName);
                    }
                }
            }
            else if (S_OK == pIMDImport->GetCustomAttributeByName(fieldTokens[i], PROFILER_STRING("System.ContextStaticAttribute"), NULL, NULL))
            {
                for(ULONG t =0;t<tPool.size();++t)
                {
                    threadId = tPool.at(t);
                    if (threadId == 0) continue;
                    // GetContextStaticAddress per thread
                    hr = GetContextStaticAddress(pPrfCom, classId, fieldTokens[i], threadId, tokenName);
                }
            }
            else if (IsFdHasFieldRVA(fieldAttributes))
            {
                for(ULONG t =0;t<tPool.size();++t)
                {
                    threadId = tPool.at(t);
                    if (threadId == 0) continue;
                    // GetRVAStatic once per static field
                    hr = GetRVAStaticAddress(pPrfCom, classId, fieldTokens[i], tokenName);
                }
            }
            else
            {  
                for (ULONG ad = 0;ad<adPool.size();++ad)
                {
                    AppdomainId = adPool.at(ad);
                    if (AppdomainId == 0) continue;
                    // GetAppdomainStaticAddress per appdomain
                    hr = GetAppdomainStaticAddress(pPrfCom, classId, fieldTokens[i], AppdomainId, tokenName);
                }
            }
        }//if static field
    }//for every token
    return hr;
}

#pragma region GC_PROFILER_CALLBACKS
//Following GC profiler callbacks capture every class ID in GC, 
//Then try to get the statics address of every static field of every class when GC finished.
HRESULT GCStatics::VerifyGenerations(IPrfCom * pPrfCom)
{
    if ((m_cGCStartedCalls == 0) || (m_ulFailure > 0))
    {
        return S_OK;
    }

    HRESULT hr = S_OK;

    memset(&m_generationInfo, 0, sizeof(m_generationInfo));

    GCATTACH_MUST_PASS(PINFO->GetGenerationBounds(MAX_OBJECT_RANGES, &m_generationInfo.nObjectRanges, m_generationInfo.objectRanges));

    if (MAX_OBJECT_RANGES < m_generationInfo.nObjectRanges)
    {
        DISPLAY(L"Too many ranges for VerifyGenerations: " << m_generationInfo.nObjectRanges);
        return hr;
    }

	for (ULONG i = 0; i < m_generationInfo.nObjectRanges; i++)
	{
		for (ULONG j = i + 1; j < m_generationInfo.nObjectRanges; j++)
		{
			if (m_generationInfo.objectRanges[i].rangeStart < m_generationInfo.objectRanges[j].rangeStart)
			{
				if (m_generationInfo.objectRanges[i].rangeStart + m_generationInfo.objectRanges[i].rangeLengthReserved <= m_generationInfo.objectRanges[j].rangeStart)
					continue;
			}
			else
			{
				if (m_generationInfo.objectRanges[j].rangeStart + m_generationInfo.objectRanges[j].rangeLengthReserved <= m_generationInfo.objectRanges[i].rangeStart)
					continue;
			}

        	GCSTATICS_FAILURE(L"VerifyGenerations: generation boundary overlap\n\t" <<
                    i << L" Start = 0x" << HEX(m_generationInfo.objectRanges[i].rangeStart) << L" Reserved = 0x" << HEX(m_generationInfo.objectRanges[i].rangeLengthReserved) << L"\n\t" <<
                    j << L" Start = 0x" << HEX(m_generationInfo.objectRanges[j].rangeStart) << L" Reserved = 0x" << HEX(m_generationInfo.objectRanges[j].rangeLengthReserved));
		}
	}

	return hr;
}

HRESULT GCStatics::FinalizeableObjectQueued(IPrfCom * pPrfCom ,
                                           DWORD finalizerFlags,
                                           ObjectID objectID)
{
    if (m_ulFailure > 0)
    {
        return S_OK;
    }

    ++m_cFinalizeableObjectQueuedCalls;
    if (finalizerFlags < 0x00 || finalizerFlags > 0x01)
    {
        // Make sure COR_PRF_FINALIZER_FLAGS has not changed before opening a new bug
        GCSTATICS_FAILURE(L"Invalid finalizerFlags in FinalizeableObjectQueued: 0x" << HEX(finalizerFlags));
    }

    {
        lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
        m_vFinalizeQueued.push_back(objectID);
    }

    ULONG size;
    GCATTACH_MUST_PASS(PINFO->GetObjectSize(objectID, &size));
    ClassID classID;
    GCATTACH_MUST_PASS(PINFO->GetClassFromObject(objectID, &classID));
    VerifyMetadata(pPrfCom, classID);
   
    return S_OK;
}

HRESULT GCStatics::GarbageCollectionFinished(IPrfCom * pPrfCom )
{
    if ((m_cGCStartedCalls == 0) || (m_ulFailure > 0))
    {
        return S_OK;
    }

    DISPLAY("\nGarbageCollectionFinished\n");

    DERIVED_COMMON_POINTER(IGCCom);

    ++m_cGCFinishedCalls;

    VerifyGenerations(pPrfCom);

    for (UINT i = 0; i < m_vMovedReferences.size(); i++)
    {
        MovedObjectInfo * pMovedObjs = m_vMovedReferences[i];

        GCATTACH_MUST_PASS(VerifyMovedReferences(pPrfCom, pMovedObjs));

        delete[] pMovedObjs->cObjectIDRangeLength;
        delete[] pMovedObjs->newObjectIDRangeStart;
        delete[] pMovedObjs->oldObjectIDRangeStart;
        delete (pMovedObjs);
    }
    m_vMovedReferences.clear();

    for (UINT i = 0; i < m_vObjectReferences.size(); i++)
    {
        ObjectRefInfo * pObjectRefInfo = m_vObjectReferences[i];

        GCATTACH_MUST_PASS(VerifyObjectReferences(pPrfCom, pObjectRefInfo));

        delete[] pObjectRefInfo->objectRefIds;
        delete (pObjectRefInfo);
    }
    m_vObjectReferences.clear();

    for (UINT i = 0; i < m_vObjectReferences.size(); i++)
    {
        RootRefInfo * pRootRefs = m_vRootReferences[i];

        GCATTACH_MUST_PASS(VerifyRootReferences(pPrfCom, pRootRefs));

        delete[] pRootRefs->rootFlags;
        delete[] pRootRefs->rootIds;
        delete[] pRootRefs->rootKinds;
        delete[] pRootRefs->rootRefIds;
        delete (pRootRefs);
    }
    m_vRootReferences.clear();

    for (UINT i = 0; i < m_vSurvivingReferences.size(); i++)
    {
        SurviveRefInfo * pSurviveRefInfo = m_vSurvivingReferences[i];

        GCATTACH_MUST_PASS(VerifySurvivingReferences(pPrfCom, pSurviveRefInfo));

        delete[] pSurviveRefInfo->objectIDRangeStart;
        delete[] pSurviveRefInfo->cObjectIDRangeLength;
        delete (pSurviveRefInfo);
    }
    m_vSurvivingReferences.clear();

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    ObjectID objectID = NULL;
    for (ObjectMap::iterator iom = PGCCOM->m_ObjectMap.begin(); 
         iom != PGCCOM->m_ObjectMap.end();
         iom++)
    {
        if (objectID != 0)
        {
            PGCCOM->RemoveObject(objectID);
            objectID = NULL;
        }

        if (!iom->second->m_isSurvived)
        {
            objectID = iom->second->m_objectID;
        }
    }

    if (objectID != 0)
    {
        PGCCOM->RemoveObject(objectID);
        objectID = NULL;
    }

    return S_OK;
}

HRESULT GCStatics::GarbageCollectionStarted(IPrfCom * pPrfCom,
                                           INT cGenerations,
                                           BOOL generationCollected[],
                                           COR_PRF_GC_REASON reason )
{
    if (m_ulFailure > 0)
    {
        return S_OK;
    }

    DERIVED_COMMON_POINTER(IGCCom);

    ++m_cGCStartedCalls;

	// get collected generations
	m_dwGenerationsCollected = 0;

	for (int i = 0; i < cGenerations; i++)
    {
		if (generationCollected[i])
        {
			m_dwGenerationsCollected |= 1 << i;
        }
    }

    DISPLAY(L"GarbageCollectionStarted - Generations: 0x" << HEX(cGenerations) << L" GenerationCollected: 0x" << HEX(m_dwGenerationsCollected) << L" Reason: 0x" << HEX(reason));

    VerifyGenerations(pPrfCom);

	// mark objects in generations not collected as survived
	ObjectMap::iterator iom;
	COR_PRF_GC_GENERATION_RANGE range;

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

    for (iom = PGCCOM->m_ObjectMap.begin(); iom != PGCCOM->m_ObjectMap.end(); iom++)
    {
		GCATTACH_MUST_PASS(PINFO->GetObjectGeneration(iom->second->m_objectID, &range));

		if ( !(m_dwGenerationsCollected & (1 << range.generation)) )
			iom->second->m_isSurvived = TRUE;
		else
			iom->second->m_isSurvived = FALSE;
    }

    return S_OK;
}

HRESULT GCStatics::HandleCreated(IPrfCom *  /* pPrfCom */,
                                GCHandleID /* handleId */,
                                ObjectID   /* initialObjectId */)
{
    if (m_ulFailure > 0)
    {
        return S_OK;
    }

    //DERIVED_COMMON_POINTER(IGCCom);

    ++m_cHandleCreatedCalls;

    //lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    //GCATTACH_MUST_PASS(PGCCOM->AddGCHandle(handleId, initialObjectId));

    return S_OK;
}

HRESULT GCStatics::HandleDestroyed(IPrfCom *  /* pPrfCom */,
                                  GCHandleID  /* handleId */)
{
    if (m_ulFailure > 0)
    {
        return S_OK;
    }

    //DERIVED_COMMON_POINTER(IGCCom);

    ++m_cHandleDestroyedCalls;

    //lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    //GCATTACH_MUST_PASS(PGCCOM->RemoveGCHandle(handleId));

    return S_OK;
}

HRESULT GCStatics::VerifyMovedReferences(IPrfCom         * pPrfCom,
                                        MovedObjectInfo * pMovedRefs)
{
    if ((m_cGCStartedCalls == 0) || (m_ulFailure > 0))
    {
        return S_OK;
    }

    DERIVED_COMMON_POINTER(IGCCom);

    ObjectMap::iterator iom, iom2;

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    for (ULONG i = 0; i < pMovedRefs->cMovedObjectIDRanges; i++)
    {
        // insert new objects in new location, old objects will be remove after GC
        ObjectID newObjectIDCur = pMovedRefs->newObjectIDRangeStart[i];
        while (newObjectIDCur < pMovedRefs->newObjectIDRangeStart[i] + pMovedRefs->cObjectIDRangeLength[i])
        {
            ULONG newObjectSize;
            GCATTACH_MUST_PASS(PINFO->GetObjectSize(newObjectIDCur, &newObjectSize));
            ClassID classID;
            GCATTACH_MUST_PASS(PINFO->GetClassFromObject(newObjectIDCur, &classID));
            VerifyMetadata(pPrfCom, classID);

            // Does object exist at old location?
            ObjectID oldObjectIDCur = newObjectIDCur - pMovedRefs->newObjectIDRangeStart[i] + pMovedRefs->oldObjectIDRangeStart[i];
            iom = PGCCOM->m_ObjectMap.find(oldObjectIDCur);

            if ((iom != PGCCOM->m_ObjectMap.end()) && (pMovedRefs->oldObjectIDRangeStart[i] == pMovedRefs->newObjectIDRangeStart[i]))
            {
                // object existed at old location and it's not really moved,
                // needed for keeping track of survived objects
                iom->second->m_isSurvived = TRUE;
            }
            else
            {
                // if an object at the new location exists, remove it
                iom2 = PGCCOM->m_ObjectMap.find(newObjectIDCur);
                if (iom2 != PGCCOM->m_ObjectMap.end())
                {
                    // Object does exist at new location
                    if (iom2->second->m_isSurvived)
                    {
                        GCSTATICS_FAILURE(L"VerifyMovedReferences: survived object being overwritten 0x" << HEX(iom->second->m_objectID));
                    }
                    else
                    {
                        GCATTACH_MUST_PASS(PGCCOM->RemoveObject(iom2->second->m_objectID));
                    }
                }

                // If object existed at old location, classIDs should match
                if (iom != PGCCOM->m_ObjectMap.end())
                {
                    if (classID != iom->second->m_classID)
                    {
                        GCSTATICS_FAILURE(L"Moved object contained unexpected classid << '0x" << HEX(classID) << L".  Expected: '0x" << HEX(iom->second->m_classID) << "'");
                    }
                }

                // Add object at new location
                GCATTACH_MUST_PASS(PGCCOM->AddObject(newObjectIDCur, classID));

                GetStaticAddress(pPrfCom, classID, THREAD_STATIC_2);

                iom2 = PGCCOM->m_ObjectMap.find(newObjectIDCur);
                iom2->second->m_isSurvived = TRUE;
            }

            // Skip to next new object
            newObjectIDCur += newObjectSize;
            AlignObject(pPrfCom, &newObjectIDCur);
        }
    }

    return S_OK;
}


HRESULT GCStatics::MovedReferences(IPrfCom * pPrfCom ,
                                  ULONG     cMovedObjectIDRanges ,
                                  ObjectID  oldObjectIDRangeStart[],
                                  ObjectID  newObjectIDRangeStart[],
                                  ULONG     cObjectIDRangeLength[])
{
    if ((m_cGCStartedCalls == 0) || (m_ulFailure > 0))
    {
        return S_OK;
    }

    ++m_cMovedReferencesCalls;

    MovedObjectInfo * pMovedObjects = new(MovedObjectInfo);

    pMovedObjects->cMovedObjectIDRanges  = cMovedObjectIDRanges;
    pMovedObjects->oldObjectIDRangeStart = new ObjectID[sizeof(ObjectID) * cMovedObjectIDRanges];
    pMovedObjects->newObjectIDRangeStart = new ObjectID[sizeof(ObjectID) * cMovedObjectIDRanges];
    pMovedObjects->cObjectIDRangeLength  = new ULONG[sizeof(ULONG) * cMovedObjectIDRanges];

    for (ULONG i = 0; i < cMovedObjectIDRanges; i++)
    {
        pMovedObjects->oldObjectIDRangeStart[i] = oldObjectIDRangeStart[i];
        pMovedObjects->newObjectIDRangeStart[i] = newObjectIDRangeStart[i];
        pMovedObjects->cObjectIDRangeLength[i]  = cObjectIDRangeLength[i];
    }

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    m_vMovedReferences.push_back(pMovedObjects);
    return S_OK;
}

HRESULT GCStatics::ObjectAllocated(IPrfCom *  pPrfCom ,
                                  ObjectID  objectId ,
                                  ClassID  classId )
{
    if (m_ulFailure > 0)
    {
        return S_OK;
    }

    DERIVED_COMMON_POINTER(IGCCom);

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    GCATTACH_MUST_PASS(PGCCOM->AddObject(objectId, classId));

    return S_OK;
}

HRESULT GCStatics::VerifyObjectReferences(IPrfCom * pPrfCom, ObjectRefInfo * pObjectRefInfo)
{
    if ((m_cGCStartedCalls == 0) || (m_ulFailure > 0))
    {
        return S_OK;
    }

    DERIVED_COMMON_POINTER(IGCCom);

    ObjectMap::iterator iom;

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    iom = PGCCOM->m_ObjectMap.find(pObjectRefInfo->objectId);

    if (iom == PGCCOM->m_ObjectMap.end())
    {
        // First time for this ObjectID
        GCATTACH_MUST_PASS(PGCCOM->AddObject(pObjectRefInfo->objectId, pObjectRefInfo->classId));
        iom = PGCCOM->m_ObjectMap.find(pObjectRefInfo->objectId);
        iom->second->m_isSurvived = TRUE;
    }

    for (ULONG i = 0; i < pObjectRefInfo->cObjectRefs; i++)
    {
        ObjectID newObjectID = pObjectRefInfo->objectRefIds[i];
        iom = PGCCOM->m_ObjectMap.find(newObjectID);

        if (iom == PGCCOM->m_ObjectMap.end())
        {
            // First time for this ObjectID
            ClassID newClassID = NULL;
            GCATTACH_MUST_PASS(PINFO->GetClassFromObject(newObjectID, &newClassID));
            VerifyMetadata(pPrfCom, newClassID);
            GCATTACH_MUST_PASS(PGCCOM->AddObject(newObjectID, newClassID));
            iom = PGCCOM->m_ObjectMap.find(newObjectID);
            iom->second->m_isSurvived = TRUE;
        }
    }

    return S_OK;
}

HRESULT GCStatics::ObjectReferences(IPrfCom * pPrfCom,
                                   ObjectID  objectId,
                                   ClassID   classId,
                                   ULONG     cObjectRefs,
                                   ObjectID  objectRefIds[])
{
    if ((m_cGCStartedCalls == 0) || (m_ulFailure > 0))
    {
        return S_OK;
    }

    ++m_cObjectReferencesCalls;

    ObjectRefInfo * pObjectRefs = new(ObjectRefInfo);

    pObjectRefs->objectId = objectId;
    pObjectRefs->classId  = classId;
    pObjectRefs->cObjectRefs = cObjectRefs;
    pObjectRefs->objectRefIds = new ObjectID[sizeof(ObjectID) * cObjectRefs];

    for (ULONG i = 0; i < cObjectRefs; i++)
    {
        pObjectRefs->objectRefIds[i] = objectRefIds[i];
    }

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    m_vObjectReferences.push_back(pObjectRefs);

    return S_OK;
}

HRESULT GCStatics::VerifyRootReferences(IPrfCom * pPrfCom, RootRefInfo * pRootRefs)
{
    if ((m_cGCStartedCalls == 0) || (m_ulFailure > 0))
    {
        return S_OK;
    }

    DERIVED_COMMON_POINTER(IGCCom);

    ObjectMap::iterator iom;

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    for (ULONG i = 0; i < pRootRefs->cRootRefs; i++)
    {
        ObjectID newObjectID = pRootRefs->rootRefIds[i];

        if (newObjectID != 0)
        {
            iom = PGCCOM->m_ObjectMap.find(newObjectID);

            if (iom == PGCCOM->m_ObjectMap.end())
            {
                // First time for this ObjectID
                ClassID newClassID = NULL;
                GCATTACH_MUST_PASS(PINFO->GetClassFromObject(newObjectID, &newClassID));
                VerifyMetadata(pPrfCom, newClassID);
                GCATTACH_MUST_PASS(PGCCOM->AddObject(newObjectID, newClassID));
                iom = PGCCOM->m_ObjectMap.find(newObjectID);
                iom->second->m_isSurvived = TRUE;
            }
        }
    }

    return S_OK;
}

HRESULT GCStatics::RootReferences2(IPrfCom *             pPrfCom,
                                  ULONG                 cRootRefs,
                                  ObjectID              rootRefIds[],
                                  COR_PRF_GC_ROOT_KIND  rootKinds[],
                                  COR_PRF_GC_ROOT_FLAGS rootFlags[],
                                  UINT_PTR              rootIds[])
{
    if ((m_cGCStartedCalls == 0) || (m_ulFailure > 0))
    {
        return S_OK;
    }

    ++m_cRootReferencesCalls;

    RootRefInfo * pRootRefs = new(RootRefInfo);

    pRootRefs->cRootRefs  = cRootRefs;
    pRootRefs->rootRefIds = new ObjectID[sizeof(ObjectID) * cRootRefs];
    pRootRefs->rootKinds  = new COR_PRF_GC_ROOT_KIND[sizeof(COR_PRF_GC_ROOT_KIND)  * cRootRefs];
    pRootRefs->rootFlags  = new COR_PRF_GC_ROOT_FLAGS[sizeof(COR_PRF_GC_ROOT_FLAGS) * cRootRefs];
    pRootRefs->rootIds    = new UINT_PTR[sizeof(UINT_PTR) * cRootRefs];

    for (ULONG i = 0; i < cRootRefs; i++)
    {
        pRootRefs->rootRefIds[i] = rootRefIds[i];
        pRootRefs->rootKinds[i]  = rootKinds[i];
        pRootRefs->rootFlags[i]  = rootFlags[i];
        pRootRefs->rootIds[i]    = rootIds[i];
    }

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    m_vRootReferences.push_back(pRootRefs);

    return S_OK;
}

#if defined(_X86_) || defined(_ARM_)
static void AlignObjectTo4Bytes(ObjectID * pObjectID)
{
    if ((*pObjectID) & 0x3)
    {
        (*pObjectID) += (0x4 - ((*pObjectID) & 0x3));
    }
}
#endif

static void AlignObjectTo8Bytes(ObjectID * pObjectID)
{
    if ((*pObjectID) & 0x7)
    {
        (*pObjectID) += (0x8 - ((*pObjectID) & 0x7));
    }
}

COR_PRF_GC_GENERATION GCStatics::GetGeneration(IPrfCom * pPrfCom, ObjectID objectID)
{
	for (ULONG i = 0; i < m_generationInfo.nObjectRanges; i++)
	{
		if ((m_generationInfo.objectRanges[i].rangeStart <= objectID) &&
            (objectID <= m_generationInfo.objectRanges[i].rangeStart + m_generationInfo.objectRanges[i].rangeLengthReserved))
		{
            return m_generationInfo.objectRanges[i].generation;
		}
    }

    GCSTATICS_FAILURE(L"Could not find containing generation for object 0x" << objectID);
    return COR_PRF_GC_GEN_0;
}

void GCStatics::AlignObject(IPrfCom * pPrfCom, ObjectID * pObjectID)
{
#if defined(_X86_) || defined(_ARM_)
    if (GetGeneration(pPrfCom, *pObjectID) == COR_PRF_GC_LARGE_OBJECT_HEAP)
    {
        // LOH is 8 byte aligned, even on x86
        AlignObjectTo8Bytes(pObjectID);
    }
    else
    {
        AlignObjectTo4Bytes(pObjectID);
    }
#else
    AlignObjectTo8Bytes(pObjectID);
#endif
}

HRESULT GCStatics::VerifySurvivingReferences(IPrfCom * pPrfCom, SurviveRefInfo * pSurviveRefInfo)
{
    if ((m_cGCStartedCalls == 0) || (m_ulFailure > 0))
    {
        return S_OK;
    }
    DERIVED_COMMON_POINTER(IGCCom);

	ObjectMap::iterator iom, iom2;

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    for (ULONG i = 0; i < pSurviveRefInfo->cSurvivingObjectIDRanges; i++)
	{
        ClassID    newClassID = NULL;
        ObjectID  newObjectID = pSurviveRefInfo->objectIDRangeStart[i];
        ObjectID lastObjectID = newObjectID + static_cast<ObjectID>(pSurviveRefInfo->cObjectIDRangeLength[i]);
        while ((newObjectID < lastObjectID) && (*(PULONG)newObjectID != 0))
        {
            iom = PGCCOM->m_ObjectMap.find(newObjectID);
            GCATTACH_MUST_PASS(PINFO->GetClassFromObject(newObjectID, &newClassID));
              
            GetStaticAddress(pPrfCom, newClassID, THREAD_STATIC_2);

            if (iom == PGCCOM->m_ObjectMap.end())
            {
                
                VerifyMetadata(pPrfCom, newClassID);
		        GCATTACH_MUST_PASS(PGCCOM->AddObject(newObjectID, newClassID));
                
                iom = PGCCOM->m_ObjectMap.find(newObjectID);
            }

		    iom->second->m_isSurvived = TRUE;
            newObjectID += iom->second->m_size;
            AlignObject(pPrfCom, &newObjectID);
        }
    }
    return S_OK;
}

HRESULT GCStatics::SurvivingReferences(IPrfCom * pPrfCom,
                                      ULONG     cSurvivingObjectIDRanges,
                                      ObjectID  objectIDRangeStart[],
                                      ULONG     cObjectIDRangeLength[])
{


    if ((m_cGCStartedCalls == 0) || (m_ulFailure > 0))
    {
        return S_OK;
    }

    ++m_cSurvivingReferencesCalls;

    SurviveRefInfo * pSurvivingRefs = new(SurviveRefInfo);

    pSurvivingRefs->cSurvivingObjectIDRanges = cSurvivingObjectIDRanges;
    pSurvivingRefs->objectIDRangeStart       = new ObjectID[sizeof(ObjectID) * cSurvivingObjectIDRanges];
    pSurvivingRefs->cObjectIDRangeLength     = new ULONG[sizeof(ULONG) * cSurvivingObjectIDRanges];

    for (ULONG i = 0; i < cSurvivingObjectIDRanges; i++)
    {
        pSurvivingRefs->objectIDRangeStart[i]   = objectIDRangeStart[i];
        pSurvivingRefs->cObjectIDRangeLength[i] = cObjectIDRangeLength[i];
    }

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    m_vSurvivingReferences.push_back(pSurvivingRefs);

    return S_OK;
}

HRESULT GCStatics::VerifyMetadata(IPrfCom * pPrfCom, ClassID classID)
{
    if ((m_cGCStartedCalls == 0) || (m_ulFailure > 0))
    {
        return S_OK;
    }

    ModuleID moduleID;
    mdTypeDef typeDef;
    ClassID classIDParent;
    ULONG32 cTypeArgs;
    CorElementType baseElemType;
    ULONG cRank;

    HRESULT hr = PINFO->IsArrayClass(classID, &baseElemType, &classIDParent, &cRank);
    if (hr == S_FALSE)
    {
        GCATTACH_MUST_PASS(PINFO->GetClassIDInfo2(classID, &moduleID, &typeDef, &classIDParent, 0, &cTypeArgs, NULL));
    }
    else
    {
        GCATTACH_MUST_PASS(hr);
    }

    return S_OK;
}

#pragma region //GC_PROFILER_CALLBACKS

HRESULT GCStatics::Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"GC Attach test complete.");
    // Some tests that do create handles, may still leak them
    if (m_cHandleCreatedCalls > 0 && m_cHandleDestroyedCalls < m_cHandleCreatedCalls)
    {
        //GCSTATICS_FAILURE(L"Number of HandleCreated calls exceeds number of HandleDestroyed calls");
    }

    // TODO: PRINT ALL STATISTICS


    // gc.exe doesn't call this
   
    if (m_cGCFinishedCalls == 0)
    {
        GCSTATICS_FAILURE(L"m_cGCFinishedCalls == 0.  Try only running this profiler with an executable that does GCs");
    }
    if (m_cGCStartedCalls == 0)
    {
        GCSTATICS_FAILURE(L"m_cGCStartedCalls == 0.  Try only running this profiler with an executable that does GCs");
    }
   
    if (m_cMovedReferencesCalls == 0 && m_cSurvivingReferencesCalls == 0)
    {
        GCSTATICS_FAILURE(L"Both m_cMovedReferencesCalls and m_cSurvivingReferencesCalls are 0");
    }

    if (m_cObjectReferencesCalls == 0)
    {
        GCSTATICS_FAILURE(L"m_cObjectReferencesCalls == 0");
    }
    if (m_cRootReferencesCalls == 0)
    {
        GCSTATICS_FAILURE(L"m_cRootReferencesCalls == 0");
    }

    DISPLAY(L"Statistics: m_cHandleCreatedCalls = " << m_cHandleCreatedCalls <<
        L"\nm_cHandleDestroyedCalls = " << m_cHandleDestroyedCalls <<
        L"\nm_cGCStartedCalls = " << m_cGCStartedCalls <<
        L"\nm_cGCFinishedCalls = " << m_cGCFinishedCalls <<
        L"\nm_cMovedReferencesCalls = " << m_cMovedReferencesCalls <<
        L"\nm_cSurvivingReferencesCalls = " << m_cSurvivingReferencesCalls <<
        L"\nm_cObjectReferencesCalls = " << m_cObjectReferencesCalls <<
        L"\nm_cRootReferencesCalls = " << m_cRootReferencesCalls <<
        L"\nm_cFinalizeableObjectQueuedCalls = " << m_cFinalizeableObjectQueuedCalls<<
        L"\nm_ulLiterals = " << m_ulLiterals <<
        L"\nm_ulErrorArgument = "<<m_ulErrorArgument <<
        L"\nm_ulDataIncomplete = "<<m_ulDataIncomplete <<
        L"\nm_ulGetAppdomainStaticAddress = "<<m_ulGetAppdomainStaticAddress <<
        L"\nm_ulGetThreadStaticAddress = "<<m_ulGetThreadStaticAddress <<
        L"\nm_ulGetThreadStaticAddress2 = "<<m_ulGetThreadStaticAddress2 <<
        L"\nm_ulGetContextStaticAddress = "<<m_ulGetContextStaticAddress <<
        L"\nm_ulGetRVAStaticAddress = "<<m_ulGetRVAStaticAddress <<
        L"\nm_ulNewThreadInit = "<<m_ulNewThreadInit <<
        L"\nm_ulSOKWithZeroValue = "<<m_ulSOKWithZeroValue <<
        L"\nm_ulFailure = "<<m_ulFailure <<
        L"\nm_ulSuccess = "<<m_ulSuccess 
        );
    
    if (m_ulFailure > 0 || m_ulSuccess <=0)
    {
        GCSTATICS_FAILURE(L"Failing test due to ERRORS above.");
        return E_FAIL;
    }

    if ((m_ulGetThreadStaticAddress+m_ulGetThreadStaticAddress2+m_ulGetThreadStaticAddress+m_ulGetContextStaticAddress+m_ulGetRVAStaticAddress+m_ulGetAppdomainStaticAddress)==0)
    {
        GCSTATICS_FAILURE(L"No Static field is found.");
        return E_FAIL;
    }

    return S_OK;
}

/*
 *  Verification routine called by TestProfiler
 */
HRESULT GCStatics_Verify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(GCStatics);
    HRESULT hr = pGCStatics->Verify(pPrfCom);

    // Clean up instance of test class
    delete pGCStatics;
    pPrfCom->m_pTestClassInstance = NULL;

    return hr;
}


/*
 *  Initialization routine called by TestProfiler
 */
void GCStatics_Initialize(IGCCom * pGCCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    IPrfCom * pPrfCom = dynamic_cast<IPrfCom *>(pGCCom);
    DISPLAY(L"Initialize GCStatics.\n")

    // Create and save an instance of test class
    SET_CLASS_POINTER(new GCStatics(pPrfCom));

    // Initialize MethodTable
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_GC|
                                COR_PRF_MONITOR_THREADS|
                                COR_PRF_MONITOR_APPDOMAIN_LOADS|
                                COR_PRF_MONITOR_CLASS_LOADS;

    REGISTER_CALLBACK(FINALIZEABLEOBJECTQUEUED,  GCStatics::FinalizeableObjectQueuedWrapper);
    REGISTER_CALLBACK(GARBAGECOLLECTIONFINISHED, GCStatics::GarbageCollectionFinishedWrapper);
    REGISTER_CALLBACK(GARBAGECOLLECTIONSTARTED,  GCStatics::GarbageCollectionStartedWrapper);
    REGISTER_CALLBACK(HANDLECREATED,             GCStatics::HandleCreatedWrapper);
    REGISTER_CALLBACK(HANDLEDESTROYED,           GCStatics::HandleDestroyedWrapper);
    REGISTER_CALLBACK(MOVEDREFERENCES,           GCStatics::MovedReferencesWrapper);
    REGISTER_CALLBACK(OBJECTALLOCATED,           GCStatics::ObjectAllocatedWrapper);
    REGISTER_CALLBACK(OBJECTREFERENCES,          GCStatics::ObjectReferencesWrapper);
    REGISTER_CALLBACK(ROOTREFERENCES2,           GCStatics::RootReferences2Wrapper);
    REGISTER_CALLBACK(SURVIVINGREFERENCES,       GCStatics::SurvivingReferencesWrapper);
    REGISTER_CALLBACK(THREADCREATED,             GCStatics::ThreadCreatedWrapper);
    REGISTER_CALLBACK(THREADDESTROYED,           GCStatics::ThreadDestroyedWrapper);
    REGISTER_CALLBACK(THREADNAMECHANGED,         GCStatics::ThreadNameChangedWrapper);
    REGISTER_CALLBACK(APPDOMAINCREATIONFINISHED, GCStatics::AppDomainCreationFinishedWrapper);
    REGISTER_CALLBACK(APPDOMAINSHUTDOWNSTARTED,  GCStatics::AppDomainShutdownStartedWrapper);
    REGISTER_CALLBACK(CLASSLOADFINISHED,         GCStatics::ClassLoadFinishedWrapper);
    REGISTER_CALLBACK(CLASSUNLOADSTARTED,        GCStatics::ClassUnloadStartedWrapper);
    REGISTER_CALLBACK(VERIFY,                    GCStatics::VerifyWrapper);

    return;
}

