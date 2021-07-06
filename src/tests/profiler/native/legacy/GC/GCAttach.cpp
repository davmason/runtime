// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
//
// GCAttach.cpp
//
// Attach and test GC Callbacks
//
// ======================================================================================

#ifdef __clang__
#pragma clang diagnostic ignored "-Wunused-function"
#endif // __clang__

#include "GCCommon.h"
#include "../LegacyCompat.h"

#define GCATTACH_FAILURE(message)       \
    m_ulFailure++;                      \
    FAILURE(message)

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

#define GCATTACH_MUST_FAIL(call, hr)                                            \
    {                                                                           \
        HRESULT __hr = (call);                                                  \
        if( SUCCEEDED(hr) || __hr != (hr) )                                     \
        {                                                                       \
            FAILURE(L"\nCall '" << #call << L"' with unexpected HR=" << HEX(__hr)   \
                    << L", expecting " << HEX(hr)                               \
                    << L"\nIn " << __FILE__ << L" at line " << __LINE__);       \
            m_ulFailure++;                                                      \
            return __hr;                                                        \
        }                                                                       \
    }

#define MAX_OBJECT_RANGES 0x100

#pragma region GC Structures

    struct GenerationInfo
    {
        ULONG nObjectRanges;
        COR_PRF_GC_GENERATION_RANGE objectRanges[MAX_OBJECT_RANGES];
    };

    struct SurviveRefInfo
    {
        ULONG      cSurvivingObjectIDRanges;
        ObjectID * objectIDRangeStart;
        SIZE_T    * cObjectIDRangeLength;
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
        SIZE_T   * cObjectIDRangeLength;
    };
    typedef vector<MovedObjectInfo *> MovedObjectInfoVector;

    typedef vector<ObjectID> ObjectIDVector;

#pragma endregion // GC Structures

class GCAttach
{
    public:

        GCAttach(IPrfCom * pPrfCom);
        ~GCAttach();

        #pragma region static_wrapper_methods

        static HRESULT FinalizeableObjectQueuedWrapper(IPrfCom * pPrfCom,
                                                       DWORD finalizerFlags,
                                                       ObjectID objectID)
        {
            return STATIC_CLASS_CALL(GCAttach)->FinalizeableObjectQueued(pPrfCom, finalizerFlags, objectID);
        }

        static HRESULT GarbageCollectionFinishedWrapper(IPrfCom * pPrfCom)
        {
            return STATIC_CLASS_CALL(GCAttach)->GarbageCollectionFinished(pPrfCom);
        }

        static HRESULT GarbageCollectionStartedWrapper(IPrfCom * pPrfCom,
                                                       INT cGenerations,
                                                       BOOL generationCollected[],
                                                       COR_PRF_GC_REASON reason)
        {
            return STATIC_CLASS_CALL(GCAttach)->GarbageCollectionStarted(pPrfCom, cGenerations, generationCollected, reason);
        }

        static HRESULT HandleCreatedWrapper(IPrfCom * pPrfCom,
                                            GCHandleID handleId,
                                            ObjectID initialObjectId)
        {
            return STATIC_CLASS_CALL(GCAttach)->HandleCreated(pPrfCom, handleId, initialObjectId);
        }

        static HRESULT HandleDestroyedWrapper(IPrfCom * pPrfCom,
                                              GCHandleID handleId)
        {
            return STATIC_CLASS_CALL(GCAttach)->HandleDestroyed(pPrfCom, handleId);
        }


        static HRESULT MovedReferencesWrapper(IPrfCom * pPrfCom,
                                              ULONG cMovedObjectIDRanges,
                                              ObjectID oldObjectIDRangeStart[  ],
                                              ObjectID newObjectIDRangeStart[  ],
                                              ULONG cObjectIDRangeLength[  ])
        {
            return STATIC_CLASS_CALL(GCAttach)->MovedReferences(pPrfCom, cMovedObjectIDRanges, oldObjectIDRangeStart, newObjectIDRangeStart, cObjectIDRangeLength);
        }

        static HRESULT MovedReferences2Wrapper(IPrfCom * pPrfCom,
                                              ULONG cMovedObjectIDRanges,
                                              ObjectID oldObjectIDRangeStart[  ],
                                              ObjectID newObjectIDRangeStart[  ],
                                              SIZE_T cObjectIDRangeLength[  ])
        {
            return STATIC_CLASS_CALL(GCAttach)->MovedReferences2(pPrfCom, cMovedObjectIDRanges, oldObjectIDRangeStart, newObjectIDRangeStart, cObjectIDRangeLength);
        }

        static HRESULT ObjectAllocatedWrapper(IPrfCom * pPrfCom,
                                              ObjectID objectId,
                                              ClassID classId)
        {
            return STATIC_CLASS_CALL(GCAttach)->ObjectAllocated(pPrfCom, objectId, classId);
        }

        static HRESULT ObjectReferencesWrapper(IPrfCom * pPrfCom,
                                               ObjectID objectId,
                                               ClassID classId,
                                               ULONG cObjectRefs,
                                               ObjectID objectRefIds[  ])
        {
            return STATIC_CLASS_CALL(GCAttach)->ObjectReferences(pPrfCom, objectId, classId, cObjectRefs, objectRefIds);
        }

        static HRESULT RootReferences2Wrapper(IPrfCom * pPrfCom,
                                              ULONG cRootRefs,
                                              ObjectID rootRefIds[],
                                              COR_PRF_GC_ROOT_KIND rootKinds[],
                                              COR_PRF_GC_ROOT_FLAGS rootFlags[],
                                              UINT_PTR rootIds[])
        {
            return STATIC_CLASS_CALL(GCAttach)->RootReferences2(pPrfCom, cRootRefs, rootRefIds, rootKinds, rootFlags, rootIds);
        }

        static HRESULT SurvivingReferencesWrapper(IPrfCom * pPrfCom,
                                                  ULONG cSurvivingObjectIDRanges,
                                                  ObjectID objectIDRangeStart[],
                                                  ULONG cObjectIDRangeLength[])
        {
            return STATIC_CLASS_CALL(GCAttach)->SurvivingReferences(pPrfCom, cSurvivingObjectIDRanges, objectIDRangeStart, cObjectIDRangeLength);
        }

        static HRESULT SurvivingReferences2Wrapper(IPrfCom * pPrfCom,
                                                  ULONG cSurvivingObjectIDRanges,
                                                  ObjectID objectIDRangeStart[],
                                                  SIZE_T cObjectIDRangeLength[])
        {
            return STATIC_CLASS_CALL(GCAttach)->SurvivingReferences2(pPrfCom, cSurvivingObjectIDRanges, objectIDRangeStart, cObjectIDRangeLength);
        }

        static HRESULT VerifyWrapper(IPrfCom * pPrfCom)
        {
            return STATIC_CLASS_CALL(GCAttach)->Verify(pPrfCom);
        }

        #pragma endregion // static_wrapper_methods

        HRESULT GCAttach::Verify(IPrfCom *  pPrfCom );

    private:

        void GCAttach::AlignObject(IPrfCom * pPrfCom, ObjectID * pObjectID);
        COR_PRF_GC_GENERATION GCAttach::GetGeneration(IPrfCom * pPrfCom, ObjectID objectID);
        HRESULT GCAttach::VerifyMetadata(IPrfCom * pPrfCom, ClassID classID);
        HRESULT GCAttach::VerifyGenerations(IPrfCom * pPrfCom);
        HRESULT GCAttach::VerifyMovedReferences(IPrfCom * pPrfCom, MovedObjectInfo * pMovedRefs);
        HRESULT GCAttach::VerifyObjectReferences(IPrfCom * pPrfCom, ObjectRefInfo * pObjectRefInfo);
        HRESULT GCAttach::VerifyRootReferences(IPrfCom * pPrfCom, RootRefInfo * pRootRefs);
        HRESULT GCAttach::VerifySurvivingReferences(IPrfCom * pPrfCom, SurviveRefInfo * pSurviveRefInfo);

        #pragma region callback_handler_prototypes

        HRESULT GCAttach::FinalizeableObjectQueued(IPrfCom *  pPrfCom ,
                                                   DWORD  finalizerFlags ,
                                                   ObjectID  objectID );

        HRESULT GCAttach::GarbageCollectionFinished(IPrfCom *  pPrfCom );

        HRESULT GCAttach::GarbageCollectionStarted(IPrfCom *  pPrfCom ,
                                                   INT  cGenerations ,
                                                   BOOL  generationCollected[] ,
                                                   COR_PRF_GC_REASON  reason );

        HRESULT GCAttach::HandleCreated(IPrfCom *  pPrfCom ,
                                        GCHandleID  handleId ,
                                        ObjectID  initialObjectId );

        HRESULT GCAttach::HandleDestroyed(IPrfCom *  pPrfCom ,
                                          GCHandleID  handleId );

        HRESULT GCAttach::MovedReferences(IPrfCom *  pPrfCom ,
                                          ULONG  cMovedObjectIDRanges ,
                                          ObjectID  oldObjectIDRangeStart[  ] ,
                                          ObjectID  newObjectIDRangeStart[  ] ,
                                          ULONG  cObjectIDRangeLength[  ] );

        HRESULT GCAttach::MovedReferences2(IPrfCom *  pPrfCom ,
                                          ULONG  cMovedObjectIDRanges ,
                                          ObjectID  oldObjectIDRangeStart[  ] ,
                                          ObjectID  newObjectIDRangeStart[  ] ,
                                          SIZE_T  cObjectIDRangeLength[  ] );

        HRESULT GCAttach::ObjectAllocated(IPrfCom *  pPrfCom ,
                                          ObjectID  objectId ,
                                          ClassID  classId );

        HRESULT GCAttach::ObjectReferences(IPrfCom *  pPrfCom ,
                                           ObjectID  objectId ,
                                           ClassID  classId ,
                                           ULONG  cObjectRefs ,
                                           ObjectID  objectRefIds[  ] );

        HRESULT GCAttach::RootReferences2(IPrfCom *  pPrfCom ,
                                          ULONG  cRootRefs ,
                                          ObjectID  rootRefIds[] ,
                                          COR_PRF_GC_ROOT_KIND  rootKinds[] ,
                                          COR_PRF_GC_ROOT_FLAGS  rootFlags[] ,
                                          UINT_PTR  rootIds[] );

        HRESULT GCAttach::SurvivingReferences(IPrfCom *  pPrfCom ,
                                              ULONG  cSurvivingObjectIDRanges ,
                                              ObjectID  objectIDRangeStart[] ,
                                              ULONG  cObjectIDRangeLength[] );
        HRESULT GCAttach::SurvivingReferences2(IPrfCom *  pPrfCom ,
                                              ULONG  cSurvivingObjectIDRanges ,
                                              ObjectID  objectIDRangeStart[] ,
                                              SIZE_T  cObjectIDRangeLength[] );


        #pragma endregion // callback_handler_prototypes

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
};

/*
 *  Initialize the GCAttach class.
 */
GCAttach::GCAttach(IPrfCom * pPrfCom)
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
    memset(&m_generationInfo, 0, sizeof(m_generationInfo));

    m_pPrfCom = pPrfCom;
}


/*
 *  Clean up
 */
GCAttach::~GCAttach()
{
    //ToDo: Kill Unmanaged Thread
}

HRESULT GCAttach::VerifyGenerations(IPrfCom * pPrfCom)
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

        	GCATTACH_FAILURE(L"VerifyGenerations: generation boundary overlap\n\t" <<
                    i << L" Start = " << HEX(m_generationInfo.objectRanges[i].rangeStart) << L" Reserved = " << HEX(m_generationInfo.objectRanges[i].rangeLengthReserved) << L"\n\t" <<
                    j << L" Start = " << HEX(m_generationInfo.objectRanges[j].rangeStart) << L" Reserved = " << HEX(m_generationInfo.objectRanges[j].rangeLengthReserved));
		}
	}

	return hr;
}

HRESULT GCAttach::FinalizeableObjectQueued(IPrfCom * pPrfCom ,
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
        GCATTACH_FAILURE(L"Invalid finalizerFlags in FinalizeableObjectQueued: " << HEX(finalizerFlags));
    }

    {
        lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
        m_vFinalizeQueued.push_back(objectID);
    }

    SIZE_T size;
    GCATTACH_MUST_PASS(PINFO4->GetObjectSize2(objectID, &size));
    if (size > ULONG_MAX)
    {
        DISPLAY(L">=4GB object seen @ 0x " << HEX(objectID));

        ULONG ulSize;
        GCATTACH_MUST_FAIL(PINFO->GetObjectSize(objectID, &ulSize), COR_E_OVERFLOW);
    }

    ClassID classID;
    GCATTACH_MUST_PASS(PINFO->GetClassFromObject(objectID, &classID));
    VerifyMetadata(pPrfCom, classID);
   
    return S_OK;
}

HRESULT GCAttach::GarbageCollectionFinished(IPrfCom * pPrfCom )
{
    if ((m_cGCStartedCalls == 0) || (m_ulFailure > 0))
    {
        return S_OK;
    }

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
        if ((void*)objectID != NULL)
        {
            PGCCOM->RemoveObject(objectID);
            objectID = NULL;
        }

        if (!iom->second->m_isSurvived)
        {
            objectID = iom->second->m_objectID;
        }
    }

    if ((void*)objectID != NULL)
    {
        PGCCOM->RemoveObject(objectID);
        objectID = NULL;
    }

    return S_OK;
}

HRESULT GCAttach::GarbageCollectionStarted(IPrfCom * pPrfCom,
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

    DISPLAY(L"GarbageCollectionStarted - Generations: " << HEX(cGenerations) << L" GenerationCollected: " << HEX(m_dwGenerationsCollected) << L" Reason: " << HEX(reason));

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

HRESULT GCAttach::HandleCreated(IPrfCom *  /* pPrfCom */,
                                GCHandleID /* handleId */,
                                ObjectID   /* initialObjectId */)
{
    if (m_ulFailure > 0)
    {
        return S_OK;
    }

    //DERIVED_COMMON_POINTER(IGCCom);

    ++m_cHandleCreatedCalls;

    // lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);;
    // GCATTACH_MUST_PASS(PGCCOM->AddGCHandle(handleId, initialObjectId));

    return S_OK;
}

HRESULT GCAttach::HandleDestroyed(IPrfCom *  /* pPrfCom */,
                                  GCHandleID  /* handleId */)
{
    if (m_ulFailure > 0)
    {
        return S_OK;
    }

    //DERIVED_COMMON_POINTER(IGCCom);

    ++m_cHandleDestroyedCalls;

    // lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);;
    // GCATTACH_MUST_PASS(PGCCOM->RemoveGCHandle(handleId));

    return S_OK;
}

HRESULT GCAttach::VerifyMovedReferences(IPrfCom         * pPrfCom,
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
            SIZE_T newObjectSize;
            GCATTACH_MUST_PASS(PINFO4->GetObjectSize2(newObjectIDCur, &newObjectSize));
            if (newObjectSize > ULONG_MAX)
            {
                DISPLAY(L"Seen object 0x" << HEX(newObjectIDCur) << L", Size = 0x" << HEX(newObjectSize) << L">=4GB object in VerifyMovedReferences; Current range length = 0x" << HEX(pMovedRefs->cObjectIDRangeLength[i]));

                ULONG ulSize;
                GCATTACH_MUST_FAIL(PINFO->GetObjectSize(newObjectIDCur, &ulSize), COR_E_OVERFLOW);
            }

            if (newObjectIDCur + newObjectSize > pMovedRefs->newObjectIDRangeStart[i] + pMovedRefs->cObjectIDRangeLength[i])
            {
                GCATTACH_FAILURE(L"VerifyMovedReferences: object 0x" << HEX(newObjectIDCur) << L", size 0x" << HEX(newObjectSize) << L"exceeding range (0x" << HEX(pMovedRefs->newObjectIDRangeStart[i]) << L", 0x" << HEX(pMovedRefs->newObjectIDRangeStart[i] + pMovedRefs->cObjectIDRangeLength[i]) << L")");
            }

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
                        GCATTACH_FAILURE(L"VerifyMovedReferences: survived object being overwritten " << HEX(iom->second->m_objectID));
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
                        GCATTACH_FAILURE(L"Moved object contained unexpected classid << '0x" << HEX(classID) << L".  Expected: '0x" << HEX(iom->second->m_classID) << "'");
                    }
                }

                // Add object at new location
                GCATTACH_MUST_PASS(PGCCOM->AddObject(newObjectIDCur, classID));
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

HRESULT GCAttach::MovedReferences(IPrfCom * pPrfCom ,
                                  ULONG     cMovedObjectIDRanges ,
                                  ObjectID  oldObjectIDRangeStart[],
                                  ObjectID  newObjectIDRangeStart[],
                                  ULONG     cObjectIDRangeLength[])
{
    GCATTACH_FAILURE(L"Should not see ICorProfilerCallback::MovedReferences");

    return E_FAIL;
}

HRESULT GCAttach::MovedReferences2(IPrfCom * pPrfCom ,
                                  ULONG     cMovedObjectIDRanges ,
                                  ObjectID  oldObjectIDRangeStart[],
                                  ObjectID  newObjectIDRangeStart[],
                                  SIZE_T     cObjectIDRangeLength[])
{
    if ((m_cGCStartedCalls == 0) || (m_ulFailure > 0))
    {
        // Skip ICorProfilerCallback::MovedReferences
        return E_FAIL;
    }

    ++m_cMovedReferencesCalls;

    MovedObjectInfo * pMovedObjects = new(MovedObjectInfo);

    pMovedObjects->cMovedObjectIDRanges  = cMovedObjectIDRanges;
    pMovedObjects->oldObjectIDRangeStart = new ObjectID[cMovedObjectIDRanges];
    pMovedObjects->newObjectIDRangeStart = new ObjectID[cMovedObjectIDRanges];
    pMovedObjects->cObjectIDRangeLength  = new SIZE_T[cMovedObjectIDRanges];

    for (ULONG i = 0; i < cMovedObjectIDRanges; i++)
    {
        pMovedObjects->oldObjectIDRangeStart[i] = oldObjectIDRangeStart[i];
        pMovedObjects->newObjectIDRangeStart[i] = newObjectIDRangeStart[i];
        pMovedObjects->cObjectIDRangeLength[i]  = cObjectIDRangeLength[i];
    }

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    m_vMovedReferences.push_back(pMovedObjects);

    // Skip ICorProfilerCallback::MovedReferences
    return E_FAIL;
}

HRESULT GCAttach::ObjectAllocated(IPrfCom *  pPrfCom ,
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

HRESULT GCAttach::VerifyObjectReferences(IPrfCom * pPrfCom, ObjectRefInfo * pObjectRefInfo)
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

HRESULT GCAttach::ObjectReferences(IPrfCom * pPrfCom,
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

HRESULT GCAttach::VerifyRootReferences(IPrfCom * pPrfCom, RootRefInfo * pRootRefs)
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

        if ((void*)newObjectID != NULL)
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

HRESULT GCAttach::RootReferences2(IPrfCom *             pPrfCom,
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

static void AlignObjectTo4Bytes(ObjectID * pObjectID)
{
    if ((*pObjectID) & 0x3) 
    { 
        (*pObjectID) += (0x4 - ((*pObjectID) & 0x3)); 
    }
}

static void AlignObjectTo8Bytes(ObjectID * pObjectID)
{
    if ((*pObjectID) & 0x7)
    {
        (*pObjectID) += (0x8 - ((*pObjectID) & 0x7));
    }
}

COR_PRF_GC_GENERATION GCAttach::GetGeneration(IPrfCom * pPrfCom, ObjectID objectID)
{
	for (ULONG i = 0; i < m_generationInfo.nObjectRanges; i++)
	{
		if ((m_generationInfo.objectRanges[i].rangeStart <= objectID) &&
            (objectID <= m_generationInfo.objectRanges[i].rangeStart + m_generationInfo.objectRanges[i].rangeLengthReserved))
		{
            return m_generationInfo.objectRanges[i].generation;
		}
    }

    GCATTACH_FAILURE(L"Could not find containing generation for object 0x" << objectID);
    return COR_PRF_GC_GEN_0;
}

void GCAttach::AlignObject(IPrfCom * pPrfCom, ObjectID * pObjectID)
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

HRESULT GCAttach::VerifySurvivingReferences(IPrfCom * pPrfCom, SurviveRefInfo * pSurviveRefInfo)
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

        while ((newObjectID < lastObjectID) && (*(LPVOID *)newObjectID != NULL))
        {
            iom = PGCCOM->m_ObjectMap.find(newObjectID);
            if (iom == PGCCOM->m_ObjectMap.end())
            {
                GCATTACH_MUST_PASS(PINFO->GetClassFromObject(newObjectID, &newClassID));
                VerifyMetadata(pPrfCom, newClassID);
		        GCATTACH_MUST_PASS(PGCCOM->AddObject(newObjectID, newClassID));
		        iom = PGCCOM->m_ObjectMap.find(newObjectID);
            }

            if( iom->second->m_size > ULONG_MAX)
                DISPLAY(L"Seen object 0x" << HEX(newObjectID) << L", size = 0x" << HEX(iom->second->m_size) << L">= 4GB in VerifySurvivingReferences, Current Range Length = 0x" << HEX(pSurviveRefInfo->cObjectIDRangeLength[i]));

            if (newObjectID + iom->second->m_size > pSurviveRefInfo->objectIDRangeStart[i] + pSurviveRefInfo->cObjectIDRangeLength[i])
            {
                GCATTACH_FAILURE(L"VerifyMovedReferences: object 0x" << HEX(newObjectID) << L", size 0x" << HEX(iom->second->m_size) << L"exceeding range (0x" << HEX(pSurviveRefInfo->objectIDRangeStart[i]) << L", 0x" << HEX(pSurviveRefInfo->objectIDRangeStart[i] + pSurviveRefInfo->cObjectIDRangeLength[i]) << L")");
            }
            
            iom->second->m_isSurvived = TRUE;
            newObjectID += iom->second->m_size;
            AlignObject(pPrfCom, &newObjectID);
        }
    }
    return S_OK;
}

HRESULT GCAttach::SurvivingReferences(IPrfCom * pPrfCom,
                                      ULONG     cSurvivingObjectIDRanges,
                                      ObjectID  objectIDRangeStart[],
                                      ULONG     cObjectIDRangeLength[])
{
    GCATTACH_FAILURE(L"Should not see ICorProfilerCallback::SurvivingReferences");
    return E_FAIL;
}

HRESULT GCAttach::SurvivingReferences2(IPrfCom * pPrfCom,
                                      ULONG     cSurvivingObjectIDRanges,
                                      ObjectID  objectIDRangeStart[],
                                      SIZE_T     cObjectIDRangeLength[])
{
    if ((m_cGCStartedCalls == 0) || (m_ulFailure > 0))
    {
        // Skip ICorProfilerCallback::SurvivingReferences
        return E_FAIL;
    }

    ++m_cSurvivingReferencesCalls;

    SurviveRefInfo * pSurvivingRefs = new(SurviveRefInfo);

    pSurvivingRefs->cSurvivingObjectIDRanges = cSurvivingObjectIDRanges;
    pSurvivingRefs->objectIDRangeStart       = new ObjectID[cSurvivingObjectIDRanges];
    pSurvivingRefs->cObjectIDRangeLength     = new SIZE_T[cSurvivingObjectIDRanges];

    for (ULONG i = 0; i < cSurvivingObjectIDRanges; i++)
    {
        pSurvivingRefs->objectIDRangeStart[i]   = objectIDRangeStart[i];
        pSurvivingRefs->cObjectIDRangeLength[i] = cObjectIDRangeLength[i];
    }

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);
    m_vSurvivingReferences.push_back(pSurvivingRefs);

    // Skip ICorProfilerCallback::SurvivingReferences
    return E_FAIL;
}

HRESULT GCAttach::VerifyMetadata(IPrfCom * pPrfCom, ClassID classID)
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


HRESULT GCAttach::Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"GC Attach test complete.");
    // Some tests that do create handles, may still leak them
    if (m_cHandleCreatedCalls > 0 && m_cHandleDestroyedCalls < m_cHandleCreatedCalls)
    {
        //GCATTACH_FAILURE(L"Number of HandleCreated calls exceeds number of HandleDestroyed calls");
    }

    // TODO: PRINT ALL STATISTICS


    // gc.exe doesn't call this
    /*
    if (m_cFinalizeableObjectQueuedCalls == 0)
    {
        GCATTACH_FAILURE(L"m_cFinalizeableObjectQueuedCalls == 0");
    }
    */

    if (m_cGCFinishedCalls == 0)
    {
        GCATTACH_FAILURE(L"m_cGCFinishedCalls == 0.  Try only running this profiler with an executable that does GCs");
    }
    if (m_cGCStartedCalls == 0)
    {
        GCATTACH_FAILURE(L"m_cGCStartedCalls == 0.  Try only running this profiler with an executable that does GCs");
    }
    // some tests don't create / destroy handles
    /*
    if (m_cHandleCreatedCalls == 0)
    {
        GCATTACH_FAILURE(L"m_cHandleCreatedCalls == 0");
    }
    if (m_cHandleDestroyedCalls == 0)
    {
        GCATTACH_FAILURE(L"m_cHandleDestroyedCalls == 0");
    }
    */

    // Some tests don't do moved or surviving references
    /*
    if (m_cMovedReferencesCalls == 0)
    {
        GCATTACH_FAILURE(L"m_cMovedReferencesCalls == 0");
    }
    */
    /*
    if (m_cSurvivingReferencesCalls == 0)
    {
        GCATTACH_FAILURE(L"m_cSurvivingReferencesCalls == 0");
    }
    */
    if (m_cMovedReferencesCalls == 0 && m_cSurvivingReferencesCalls == 0)
    {
        GCATTACH_FAILURE(L"Both m_cMovedReferencesCalls and m_cSurvivingReferencesCalls are 0");
    }

    if (m_cObjectReferencesCalls == 0)
    {
        GCATTACH_FAILURE(L"m_cObjectReferencesCalls == 0");
    }
    if (m_cRootReferencesCalls == 0)
    {
        GCATTACH_FAILURE(L"m_cRootReferencesCalls == 0");
    }

    DISPLAY(L"Statistics: m_cHandleCreatedCalls = " << m_cHandleCreatedCalls <<
        L"\nm_cHandleDestroyedCalls = " << m_cHandleDestroyedCalls <<
        L"\nm_cGCStartedCalls = " << m_cGCStartedCalls <<
        L"\nm_cGCFinishedCalls = " << m_cGCFinishedCalls <<
        L"\nm_cMovedReferencesCalls = " << m_cMovedReferencesCalls <<
        L"\nm_cSurvivingReferencesCalls = " << m_cSurvivingReferencesCalls <<
        L"\nm_cObjectReferencesCalls = " << m_cObjectReferencesCalls <<
        L"\nm_cRootReferencesCalls = " << m_cRootReferencesCalls <<
        L"\nm_cFinalizeableObjectQueuedCalls = " << m_cFinalizeableObjectQueuedCalls);
    
    if (m_ulFailure > 0)
    {
        GCATTACH_FAILURE(L"Failing test due to ERRORS above.");
        return E_FAIL;
    }


    return S_OK;
}

/*
 *  Verification routine called by TestProfiler
 */
HRESULT GCAttach_Verify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(GCAttach);
    HRESULT hr = pGCAttach->Verify(pPrfCom);

    // Clean up instance of test class
    delete pGCAttach;
    pPrfCom->m_pTestClassInstance = NULL;

    return hr;
}


/*
 *  Initialization routine called by TestProfiler
 */
void GCAttach_Initialize(IGCCom * pGCCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    IPrfCom * pPrfCom = dynamic_cast<IPrfCom *>(pGCCom);
    DISPLAY(L"Initialize GCAttach.\n")

    // Create and save an instance of test class
    SET_CLASS_POINTER(new GCAttach(pPrfCom));

    // Initialize MethodTable
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_GC;

    REGISTER_CALLBACK(FINALIZEABLEOBJECTQUEUED,  GCAttach::FinalizeableObjectQueuedWrapper);
    REGISTER_CALLBACK(GARBAGECOLLECTIONFINISHED, GCAttach::GarbageCollectionFinishedWrapper);
    REGISTER_CALLBACK(GARBAGECOLLECTIONSTARTED,  GCAttach::GarbageCollectionStartedWrapper);
    REGISTER_CALLBACK(HANDLECREATED,             GCAttach::HandleCreatedWrapper);
    REGISTER_CALLBACK(HANDLEDESTROYED,           GCAttach::HandleDestroyedWrapper);
    REGISTER_CALLBACK(MOVEDREFERENCES,           GCAttach::MovedReferencesWrapper);
    REGISTER_CALLBACK(MOVEDREFERENCES2,          GCAttach::MovedReferences2Wrapper);
    REGISTER_CALLBACK(OBJECTALLOCATED,           GCAttach::ObjectAllocatedWrapper);
    REGISTER_CALLBACK(OBJECTREFERENCES,          GCAttach::ObjectReferencesWrapper);
    REGISTER_CALLBACK(ROOTREFERENCES2,           GCAttach::RootReferences2Wrapper);
    REGISTER_CALLBACK(SURVIVINGREFERENCES,       GCAttach::SurvivingReferencesWrapper);
    REGISTER_CALLBACK(SURVIVINGREFERENCES2,      GCAttach::SurvivingReferences2Wrapper);
    REGISTER_CALLBACK(VERIFY,                    GCAttach::VerifyWrapper);

    return;
}

