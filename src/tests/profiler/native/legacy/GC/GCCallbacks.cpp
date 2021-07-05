#include "GCCommon.h"

// Legacy support
#include "../LegacyCompat.h"
// Legacy support


// filler for empty generations: x86 - 0xc; ia64 - 0x18
#define GEN_FILLER 0x18


// counter for total number of GCs, incremented at GarbageCollectionStarted
LONG gc_totalGCOperations;

// counter for objects that survived a GC
// reset at GarbageCollectionStarted
// incremented at ObjectReferences
// verified at GarbageCollectionFinished against size of object map
LONG gc_survivedObjects;

// a bit mask that indicates what generations are collected
// e.g. 0xf: full GC; 0x1: only collects generation 0.
// set at GarbageCollectionStarted
// passed to VerifyObjectReferences
DWORD gc_generationsCollected;
DWORD gc_maxGeneration = 4;
DWORD gc_gen0Flag = 0x1;
DWORD gc_gen1Flag = 0x2;
DWORD gc_gen2Flag = 0x4;
DWORD gc_lohFlag = 0x8;
DWORD gc_pinFlag = 0x10;
DWORD gc_fullFlag = gc_gen0Flag | gc_gen1Flag | gc_gen2Flag | gc_lohFlag | gc_pinFlag;


COR_PRF_GC_GENERATION_RANGE *gc_generationInternal = NULL;


ObjectID gc_generationTwoInitial;


//
// verify initial GC heap, store frozon object segment
//
HRESULT VerifyFirstGC(IPrfCom * pPrfCom)
{
    HRESULT hr = S_OK;
    DERIVED_COMMON_POINTER(IGCCom);

    ULONG nObjectRanges;
    COR_PRF_GC_GENERATION_RANGE objectRanges[SHORT_LENGTH];
	int n = 0;

    hr = PINFO->GetGenerationBounds(SHORT_LENGTH, &nObjectRanges, objectRanges);
    if (FAILED(hr))
    {
		FAILURE(L"VerifyFirstGC: GetGenerationBounds returned " << HEX(hr) << L"\n");
        return hr;
    }

	// Assuming an initial gc heap like this.
	//
	// Generation: 3	Start: 1ee1000	Length: 2030	Reserved: fff000	// big objects
    // Generation: 2	Start: 7a8d0bbc Length: 1fd1c	Reserved: 1fd1c		// frozen objects (per module)
	// Generation: 2	Start: 3120004	Length: c		Reserved: c			// debug build only, internal
	// Generation: 2	Start: 5ba29b7c	Length: 465f0	Reserved: 465f0		// frozen objects (per module)
	// Generation: 2	Start: ee1000	Length: c		Reserved: c			// empty, with filler
	// Generation: 1	Start: ee100c	Length: c		Reserved: c			// empty, with filler
	// Generation: 0	Start: ee1018	Length: 15b8	Reserved: ffefe8	// new objects

	// loop through generations from 0 to 3
	for (int i = nObjectRanges - 1; i >= 0; i--)
	{
		switch (objectRanges[i].generation)
		{
			case 0:
			case 3:
            case 4:
				// no useful verification
				break;
			case 1:
				if (objectRanges[i].rangeLength > GEN_FILLER || objectRanges[i].rangeLengthReserved > GEN_FILLER)
					FAILURE(L"VerifyFirstGC: initial generation 1 not empty");
				break;
			case 2:
				// frozen objects heap segment are one per module
				if (objectRanges[i].rangeLength > GEN_FILLER)
				{
					PGCCOM->AddFrozenSegment(objectRanges[i]);
				}
				else if (objectRanges[i].rangeLength <= GEN_FILLER)
				{
					// assuming the first empty generation 2 heap segment is for live objects
					if (n == 0)
					{
						gc_generationTwoInitial = objectRanges[i].rangeStart;
					}
					// assuming the second empty generation 2 heap segment encountered (if any) is internal
					else if (n == 1)
					{
						gc_generationInternal = new COR_PRF_GC_GENERATION_RANGE;
						*gc_generationInternal = objectRanges[i];
					}
                                    n++;
    				}
				else
					FAILURE(L"VerifyFirstGC: unexpected generation 2 heap segment size");
				break;
			default:
				FAILURE(L"VerifyFirstGC: invalid generation: " << objectRanges[i].generation);
		}
	}

	return hr;
}


//
// verify no generation boundary overlap
//
HRESULT VerifyGenerations(IPrfCom * pPrfCom)
{
    HRESULT hr = S_OK;

    ULONG nObjectRanges = NULL;
	COR_PRF_GC_GENERATION_RANGE objectRanges[SHORT_LENGTH * 10];

    hr = PINFO->GetGenerationBounds(SHORT_LENGTH * 10, &nObjectRanges, objectRanges);
    if (FAILED(hr))
    {
		FAILURE(L"VerifyGenerations: GetGenerationBounds returned " << HEX(hr) << L"\n");
        return hr;
    }

    if (SHORT_LENGTH * 10 < nObjectRanges)
    {
        return hr;
    }

	for (ULONG i = 0; i < nObjectRanges; i++)
	{
		for (ULONG j = i + 1; j < nObjectRanges; j++)
		{
			if (objectRanges[i].rangeStart < objectRanges[j].rangeStart)
			{
				if (objectRanges[i].rangeStart + objectRanges[i].rangeLengthReserved <= objectRanges[j].rangeStart)
					continue;
			}
			else
			{
				if (objectRanges[j].rangeStart + objectRanges[j].rangeLengthReserved <= objectRanges[i].rangeStart)
					continue;
			}

			FAILURE(L"VerifyGenerations: generation boundary overlap\n\t"
				<< i << L": Start = " << HEX(objectRanges[i].rangeStart) << L", Reserved = " << HEX(objectRanges[i].rangeLengthReserved) << L"\n\t"
				<< j << L": Start = " << HEX(objectRanges[j].rangeStart) << L", Reserved = " << HEX(objectRanges[j].rangeLengthReserved));
		}

		DISPLAY(L"\tGeneration: " << HEX(objectRanges[i].generation) << L"\tStart: " << objectRanges[i].rangeStart << L"\tLength: "
			<< objectRanges[i].rangeLength << "\tReserved: \n" << objectRanges[i].rangeLengthReserved);
	}

	return hr;
}


//
// add newly loaded frozen object segments to table
//
HRESULT StoreNewlyLoadedFrozenObjectSegment(IPrfCom * pPrfCom)
{
    HRESULT hr = S_OK;
    DERIVED_COMMON_POINTER(IGCCom);

	ULONG nObjectRanges;
	COR_PRF_GC_GENERATION_RANGE objectRanges[SHORT_LENGTH];
	BOOL bInitialStartReached = FALSE;

    hr = PINFO->GetGenerationBounds(SHORT_LENGTH, &nObjectRanges, objectRanges);
    if (FAILED(hr))
    {
		FAILURE(L"VerifyFirstGC: GetGenerationBounds returned " << HEX(hr));
        return hr;
    }

	//
	// heuristics for determining frozen object segments (WARNING: SUBJECT TO CHANGE!!!):
	//   generation == 2
	//   appear in an index lower than the initial live generation 2 segment in the generation array (WARNING: BIG ASSUMPTION!!!)
	//   length > filler
	//
	// A typical GC heap:
	//
	//   [0] Generation: 3	Start: 1ec1000	Length: 4260	Reserved: fff000
	//   [1] Generation: 2	Start: 7a86e2b4	Length: 216dc	Reserved: 216dc		// new frozen objects
	//   [2] Generation: 2	Start: 3110004	Length: c	Reserved: c
	//   [3] Generation: 2	Start: 5ba36ba0	Length: 49c84	Reserved: 49c84		// initial frozen objects
	//   [4] Generation: 2	Start: ec1000	Length: ffead8	Reserved: ffead8	// initial live objects
	//   [5] Generation: 2	Start: 4061000	Length: 30435c	Reserved: 30435c	// new live objects
	//   [6] Generation: 1	Start: 436535c	Length: c34048	Reserved: c34048
	//   [7] Generation: 0	Start: 4f993a4	Length: c	Reserved: c6c5c
	//

	for (int i = nObjectRanges - 1; i >= 0; i--)
	{
		if (objectRanges[i].generation == 2)
		{
			if (objectRanges[i].rangeStart == gc_generationTwoInitial)
			{
				bInitialStartReached = TRUE;
			}
			else if (bInitialStartReached && objectRanges[i].rangeLength > GEN_FILLER)
			{
				PGCCOM->AddFrozenSegment(objectRanges[i]);
			}
		}
	}

	return hr;
}


//
// initialization
// mark each object in non-GC generations as survived
//
HRESULT gc_GarbageCollectionStarted(IPrfCom * pPrfCom, int cGenerations, BOOL generationCollected[], COR_PRF_GC_REASON reason)
{

    HRESULT hr = S_OK;
    DERIVED_COMMON_POINTER(IGCCom);

    if ( PPRFCOM->m_GarbageCollectionStarted != 1 )
    {
		FAILURE(L"gc_GarbageCollectionStarted: unexpected event sequence: GarbageCollectionStarted = " << PPRFCOM->m_GarbageCollectionStarted);
    }

    // An attaching profiler must bail until we see the first FULL (all generations) GC.
    if ((gc_totalGCOperations == 0) &&
        PPRFCOM->m_bAttachMode &&
        ((cGenerations != gc_maxGeneration) || !generationCollected[0] || !generationCollected[1] || !generationCollected[2] || !generationCollected[3] || !generationCollected[4]))
    {
        return S_OK;
    }

	++(gc_totalGCOperations);

	if (gc_totalGCOperations == 1 && !pPrfCom->m_bAttachMode)
	{
		VerifyFirstGC(pPrfCom);
	}
	else
	{
		StoreNewlyLoadedFrozenObjectSegment(pPrfCom);
	}

	// reset object counter
	gc_survivedObjects = 0;

	// get collected generations
	gc_generationsCollected = 0;
	for (int g = 0; g < cGenerations; g++)
		if (generationCollected[g])
			gc_generationsCollected |= 1 << g;

	DISPLAY(L"gc_GarbageCollectionStarted [" << gc_totalGCOperations << L"]\n\tcGenerations: " << cGenerations
		<< L"\n\tgenerationCollected: " << HEX(gc_generationsCollected) << "\n\treason: " << reason << L"\n");

	VerifyGenerations(pPrfCom);

	// mark objects in generations not collected as survived
	ObjectMap::iterator iom;
	COR_PRF_GC_GENERATION_RANGE range;

	{
		lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

		for (iom = PGCCOM->m_ObjectMap.begin(); iom != PGCCOM->m_ObjectMap.end(); iom++)
		{
			hr = PINFO->GetObjectGeneration(iom->second->m_objectID, &range);
			if (FAILED(hr))
			{
				FAILURE(L"gc_GarbageCollectionStarted: GetObjectGeneration returned " << HEX(hr));
				return hr;
			}

			if (!(gc_generationsCollected & (1 << range.generation)))
				iom->second->m_isSurvived = TRUE;
			else
				iom->second->m_isSurvived = FALSE;
		}
	}

	return hr;
}


//
// remove collected objects from the map
// verify number of objects in the map against counter
// verify all objects are reachable
// reset counters
//
HRESULT gc_GarbageCollectionFinished(IPrfCom * pPrfCom)
{
    HRESULT hr = S_OK;
    DERIVED_COMMON_POINTER(IGCCom);

	ObjectMap::iterator iom, iom2;

    if ( PPRFCOM->m_GarbageCollectionStarted != PPRFCOM->m_GarbageCollectionFinished )
    {
		FAILURE(L"gc_GarbageCollectionFinished: unexpected event sequence: GarbageCollectionStarted = " << PPRFCOM->m_GarbageCollectionStarted
			<< L", GarbageCollectionFinished = " << PPRFCOM->m_GarbageCollectionFinished << "\n");
    }

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

    if (gc_totalGCOperations > 0)
    {
        if ((gc_totalGCOperations > 1) || !PPRFCOM->m_bAttachMode)
        {
	        // remove collected objects
            for (iom = PGCCOM->m_ObjectMap.begin(); iom != PGCCOM->m_ObjectMap.end();)
            {
		        iom2 = iom;
		        iom++;

		        if (!iom2->second->m_isSurvived)
			        PGCCOM->RemoveObject(iom2->second->m_objectID);
            }
        }

		DISPLAY(L"gc_GarbageCollectionFinished [" << gc_totalGCOperations << L"]\n");

	    VerifyGenerations(pPrfCom);

        if ((gc_totalGCOperations > 1) || !PPRFCOM->m_bAttachMode)
        {
    	    PGCCOM->VerifyObjectReferences(gc_generationsCollected);
        }

	    PGCCOM->ResetObjectTable();
    }

	// reset event counters
	PPRFCOM->m_GarbageCollectionStarted = 0;
	PPRFCOM->m_GarbageCollectionFinished = 0;
	PPRFCOM->m_SurvivingReferences = 0;
	PPRFCOM->m_MovedReferences = 0;
	PPRFCOM->m_RootReferences2 = 0;

    return hr;
}


//
// verify object exists
// mark it as queued for finalization
//
HRESULT gc_FinalizeableObjectQueued(IPrfCom * pPrfCom, DWORD /*finalizerFlags*/, ObjectID objectID)
{
    HRESULT hr = S_OK;
    DERIVED_COMMON_POINTER(IGCCom);

    if ((gc_totalGCOperations <= 1) && PPRFCOM->m_bAttachMode)
    {
        return S_OK;
    }

	ObjectMap::iterator iom;

    if ( PPRFCOM->m_GarbageCollectionStarted != 1 )
    {
		FAILURE(L"gc_FinalizeableObjectQueued: unexpected event sequence: GarbageCollectionStarted = " << PPRFCOM->m_GarbageCollectionStarted << L"\n");
    }

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

	iom = PGCCOM->m_ObjectMap.find(objectID);
    if (iom == PGCCOM->m_ObjectMap.end())
    {
		FAILURE(L"gc_FinalizeableObjectQueued: ObjectID not found " << HEX(objectID) << "\n");
    }

	iom->second->m_isQueuedForFinalize = TRUE;

    return hr;
}


//
// verify each surviving object exists in the map
// mark it as survived
//
HRESULT gc_SurvivingReferences(IPrfCom * pPrfCom, ULONG cSurvivingObjectIDRanges, ObjectID objectIDRangeStart[], ULONG cObjectIDRangeLength[])
{
    HRESULT hr = S_OK;
    DERIVED_COMMON_POINTER(IGCCom);

    if ((gc_totalGCOperations <= 1) && PPRFCOM->m_bAttachMode)
    {
        return S_OK;
    }

	ObjectMap::iterator iom;
	ULONG i;

	// if SurvivingReferences and MovedReferences both occur, it must be a full gc, including generation 3
	if ( PPRFCOM->m_GarbageCollectionStarted != 1 || (PPRFCOM->m_MovedReferences != 0 && gc_generationsCollected != gc_fullFlag) )
	{
		FAILURE("gc_SurvivingReferences: unexpected event sequence: GarbageCollectionStarted = " << PPRFCOM->m_GarbageCollectionStarted
			<< L", MovedReferences = " << PPRFCOM->m_MovedReferences << L"\n");
	}

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

	for (i = 0; i < cSurvivingObjectIDRanges; i++)
	{
		// locate the starting object, it must exist
    	iom = PGCCOM->m_ObjectMap.find(objectIDRangeStart[i]);
	    if (iom == PGCCOM->m_ObjectMap.end())
	    {
			FAILURE(L"gc_SurvivingReferences: unexpected surviving object range " << HEX(objectIDRangeStart[i]));
	    }

		// mark all objects within the range as survived
		while (iom != PGCCOM->m_ObjectMap.end() && iom->second->m_objectID < objectIDRangeStart[i] + cObjectIDRangeLength[i])
		{
			if (iom->second->m_isSurvived)
			{
				FAILURE(L"gc_SurvivingReferences: object marked more than once " << iom->second->m_objectID << L"\n");
			}

			iom->second->m_isSurvived = TRUE;
			iom++;
		}
	}

    return hr;
}


//
// verify each moved object exists in the map
// if necessary, add new object to new location, and mark it as survived
// leave old object unmarked, which will be removed at GarbageCollectionFinished
//
HRESULT gc_MovedReferences(IPrfCom * pPrfCom, ULONG cmovedObjectIDRanges, ObjectID oldObjectIDRangeStart[], ObjectID newObjectIDRangeStart[], ULONG cObjectIDRangeLength[])
{
    HRESULT hr = S_OK;
    DERIVED_COMMON_POINTER(IGCCom);

    if ((gc_totalGCOperations <= 1) && PPRFCOM->m_bAttachMode)
    {
        return S_OK;
    }

	ObjectMap::iterator iom, iom2;
	ObjectID newID;
	ULONG i;

	// if SurvivingReferences and MovedReferences both occur, it must be a full gc, including generation 3
	if ( PPRFCOM->m_GarbageCollectionStarted != 1 || (PPRFCOM->m_SurvivingReferences != 0 && gc_generationsCollected != gc_fullFlag) )
	{
		FAILURE(L"gc_MovedReferences: unexpected event sequence: GarbageCollectionStarted = " <<
			PPRFCOM->m_GarbageCollectionStarted << ", SurvivingReferences = " << PPRFCOM->m_SurvivingReferences <<
            ", gc_generationsCollected = " << gc_generationsCollected << "\n");
	}

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

	for (i = 0; i < cmovedObjectIDRanges; i++)
	{
		// locate beginning object to be moved, it must exist
    	iom = PGCCOM->m_ObjectMap.find(oldObjectIDRangeStart[i]);
	    if (iom == PGCCOM->m_ObjectMap.end())
	    {
			FAILURE(L"gc_MovedReferences: unexpected moved ObjectID range " << HEX(oldObjectIDRangeStart[i]) << L"\n");
	    }

		// insert new objects in new location, old objects will be remove after GC
		while (iom != PGCCOM->m_ObjectMap.end() && iom->second->m_objectID < oldObjectIDRangeStart[i] + cObjectIDRangeLength[i])
		{
			if (iom->second->m_isSurvived)
			{
				FAILURE(L"gc_MovedReferences: object marked more than once " << HEX(iom->second->m_objectID) << L"\n");
			}

			// objects not really moved, needed for keeping track of survived objects
			if (oldObjectIDRangeStart[i] == newObjectIDRangeStart[i])
			{
				iom->second->m_isSurvived = TRUE;
			}
			else
			{
				// calculate new object location
				newID = iom->second->m_objectID - oldObjectIDRangeStart[i] + newObjectIDRangeStart[i];

				// if an object at that location exists, remove it
		    	iom2 = PGCCOM->m_ObjectMap.find(newID);
			    if (iom2 != PGCCOM->m_ObjectMap.end())
			    {
					if (iom2->second->m_isSurvived)
					{
						FAILURE(L"gc_MovedReferences: survived object being overwritten " << HEX(iom->second->m_objectID) << L"\n");
					}
					else
					{
						hr = PGCCOM->RemoveObject(iom2->second->m_objectID);
						if (FAILED(hr))
							FAILURE(L"gc_MovedReferences: RemoveObject failed " << HEX(iom2->second->m_objectID));
					}
			    }

				// add object at new location
				hr = PGCCOM->AddObject(newID, iom->second->m_classID, (ULONG)iom->second->m_size);
			    if (FAILED(hr))
			    {
					FAILURE(L"gc_MovedReferences: AddObject failed " << HEX(iom->second->m_objectID) << L"\n");
			        return hr;
			    }
				iom2 = PGCCOM->m_ObjectMap.find(newID);
				iom2->second->m_isSurvived = TRUE;

				// copy object state from old object to new object
				iom2->second->m_isQueuedForFinalize = iom->second->m_isQueuedForFinalize;
			}
			iom++;
		}
	}

    return hr;
}


//
// verify each root object exists in the map
// mark it as root
// verify according to object type
//
HRESULT gc_RootReferences2(IPrfCom * pPrfCom, ULONG cRootRefs, ObjectID rootRefIds[], COR_PRF_GC_ROOT_KIND rootKinds[], COR_PRF_GC_ROOT_FLAGS rootFlags[], UINT_PTR rootIds[])
{
    HRESULT hr = S_OK;
    DERIVED_COMMON_POINTER(IGCCom);

    if ((gc_totalGCOperations <= 1) && PPRFCOM->m_bAttachMode)
    {
        return S_OK;
    }

	ObjectMap::iterator iom;
	GCHandleMap::iterator ihm;

	if ( PPRFCOM->m_GarbageCollectionStarted != 1 || PPRFCOM->m_RootReferences2 != 1 )
	{
		FAILURE(L"gc_RootReferences2: unexpected event sequence: " <<
			L"GarbageCollectionStarted = " << PPRFCOM->m_GarbageCollectionStarted <<
			", RootReferences2 = " << PPRFCOM->m_RootReferences2 << L"\n");
	}

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

	for (ULONG i = 0; i < cRootRefs; i++)
	{
    	DISPLAY( L"\tObjectID: " << HEX(rootRefIds[i]) << L"\tKind: ");

		if (rootRefIds[i] != 0)
		{
			iom = PGCCOM->m_ObjectMap.find(rootRefIds[i]);
		    if (iom == PGCCOM->m_ObjectMap.end())
		    {
				// root reference could be a frozen object
				BOOL bFound = FALSE;
			    for (FrozenSegmentMap::iterator ifm = PGCCOM->m_FrozenSegmentMap.begin(); ifm != PGCCOM->m_FrozenSegmentMap.end(); ifm++)
			    {
					if (ifm->second->rangeStart <= rootRefIds[i] && rootRefIds[i] < ifm->second->rangeStart + ifm->second->rangeLength)
						bFound = TRUE;
			    }

				if (!bFound)
					FAILURE(L"gc_RootReferences2: unexpected root reference - rootRefId: " << HEX(rootRefIds[i])
						<< L", rootId: " << HEX(rootIds[i]) << L"\n");
		    }
			else
				iom->second->m_isRoot = TRUE;
		}

		switch (rootKinds[i])
		{
			case COR_PRF_GC_ROOT_STACK:
				DISPLAY(L"STACK");

				break;
			case COR_PRF_GC_ROOT_FINALIZER:
	    		DISPLAY(L"FINALIZER");

				if (!iom->second->m_isQueuedForFinalize)
			    {
					FAILURE(L"gc_RootReferences2: object not queued for finalize \n" << HEX(rootRefIds[i]));
			    }

				iom->second->m_isQueuedForFinalize = FALSE;
				break;
			case COR_PRF_GC_ROOT_HANDLE:
				DISPLAY(L"HANDLE");

/*				ihm = pGCCom->m_GCHandleMap.find(rootIds[i]);
			    if (ihm == pGCCom->m_GCHandleMap.end())
			    {
			        _FAILURE_OLD_( (L"GCHandleID not found 0x%x\n", rootIds[i] ) )
			    }*/

				break;
			case COR_PRF_GC_ROOT_OTHER:
				DISPLAY(L"OTHER");
				break;
			default:
        		FAILURE(L"gc_RootReferences2: unknown root type\n" );
		}
    	DISPLAY(L"\tFlags: " << HEX(rootFlags[i]));
		DISPLAY(L"\tRootID: " << HEX(rootIds[i]) << L"\n");
	}

    return hr;
}


//
// add handle to map
// if initialObjectId is not null, verify object exists TODO
//
HRESULT gc_HandleCreated(IPrfCom * pPrfCom, GCHandleID handleId, ObjectID initialObjectId)
{
    HRESULT hr = S_OK;
    DERIVED_COMMON_POINTER(IGCCom);

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

    hr = PGCCOM->AddGCHandle(handleId, initialObjectId);
    if (FAILED(hr))
    {
		FAILURE(L"gc_HandleCreated: AddGCHandle failed for GCHandleID " << HEX(handleId) << L" for ObjectID " << HEX(initialObjectId) << L"\n");
    }

    return hr;
}


//
// remove handle from map
//
HRESULT gc_HandleDestroyed(IPrfCom * pPrfCom, GCHandleID handleId)
{
    HRESULT hr = S_OK;
    DERIVED_COMMON_POINTER(IGCCom);

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

    hr = PGCCOM->RemoveGCHandle(handleId);

    if (PPRFCOM->m_bAttachMode)
        hr = S_OK;

    if (FAILED(hr))
    {
		FAILURE(L"gc_HandleDestroyed: RemoveGCHandle failed " << HEX(handleId) << L"\n");
    }

    return hr;
}


//
// add object to the map
//
HRESULT gc_ObjectAllocated(IPrfCom * pPrfCom, ObjectID objectID, ClassID classID)
{
    HRESULT hr = S_OK;
    DERIVED_COMMON_POINTER(IGCCom);

    if ((gc_totalGCOperations == 0) && PPRFCOM->m_bAttachMode)
    {
        return S_OK;
    }

// 	_DISPLAY_OLD_( ( L"ObjectAllocated: 0x%x", objectID) )
    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

    hr = PGCCOM->AddObject(objectID, classID);
    if (FAILED(hr))
    {
		FAILURE(L"gc_ObjectAllocated: AddObject failed for ObjectID " << HEX(objectID) << " for ClassID " << HEX(classID) << L"\n");
    }

    return hr;
}

//
// verify object exists
// store object references
// increment counter
//
HRESULT gc_ObjectReferences(IPrfCom * pPrfCom, ObjectID objectID, ClassID classID, ULONG objectRefs, ObjectID objectRefIDs[])
{
    HRESULT hr = S_OK;
	ObjectMap::iterator iom;
    DERIVED_COMMON_POINTER(IGCCom);

    if ((gc_totalGCOperations < 1) && PPRFCOM->m_bAttachMode)
    {
        return S_OK;
    }

    if ((gc_totalGCOperations == 1) && PPRFCOM->m_bAttachMode)
    {
        gc_ObjectAllocated(pPrfCom, objectID, classID);
        return S_OK;
    }


// 	_DISPLAY_OLD_( ( L"ObjectReferences: 0x%x", objectID) )

	if ( PPRFCOM->m_GarbageCollectionStarted != 1 )
	{
		FAILURE(L"gc_ObjectReferences: unexpected event sequence: GarbageCollectionStarted = " << PPRFCOM->m_GarbageCollectionStarted << L"\n");
	}

	// ignore frozen objects
    for (FrozenSegmentMap::iterator ifm = PGCCOM->m_FrozenSegmentMap.begin(); ifm != PGCCOM->m_FrozenSegmentMap.end(); ifm++)
    {
		if (ifm->second->rangeStart <= objectID && objectID < ifm->second->rangeStart + ifm->second->rangeLength)
			return hr;
    }

	// ignore dummy segment
	if (gc_generationInternal && objectID >= gc_generationInternal->rangeStart && objectID < gc_generationInternal->rangeStart + gc_generationInternal->rangeLength)
		return hr;

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

    PGCCOM->AddObjectReferences(objectID, objectRefs, objectRefIDs);

    ++gc_survivedObjects;

    return hr;
}


HRESULT gc_ProfilerAttachComplete(IPrfCom * pPrfCom)
{
    HRESULT hr = S_OK;

    // NOTE: If this test is running in attach mode, we need to catch-up on objectIDs before we start testing.
    // We will initiate a Forced GC collection (Info::ForceGC) from inside the ProfilerAttachComplete
    // callback (the only callback where it is safe to trigger a GC synchronously). Once we catch up with
    // the object states, we will then actually start the test.
    hr = PINFO->ForceGC();
    if (FAILED(hr))
    {
		FAILURE(L"ProfilerAttachComplete(: ForceGC returned " << HEX(hr) << L"\n");
    }

   return hr;
}


//
// verify and clean up
//
HRESULT gc_Verify(IPrfCom * pPrfCom)
{
    DISPLAY( L"Verify GC Callbacks ...\n");
    HRESULT hr = S_OK;

    if (gc_totalGCOperations == 0)
    {
		FAILURE(L"gc_Verify: Failure - no GC events\n");
        return E_FAIL;
    }

	if (pPrfCom->m_HandleCreated == 0)
	{
		FAILURE(L"gc_Verify: Failure - no GC HandleCreated events\n");
	}

	if (pPrfCom->m_HandleDestroyed == 0)
	{
		FAILURE(L"gc_Verify: Failure - no GC HandleDestroyed events\n");
	}

	if (pPrfCom->m_ObjectAllocated == 0)
	{
		FAILURE(L"gc_Verify: Failure - no GC ObjectAllocated events\n");
	}

    // Disabling debug logging
    /*
    ObjectMap::iterator iom;

    _DISPLAY_OLD_( ( L"gc_Verify: Object Map 0x%x entries\n", pGCCom->m_ObjectMap.size() ) )
    for (iom = pGCCom->m_ObjectMap.begin(); iom != pGCCom->m_ObjectMap.end(); iom++)
    {
    	_DISPLAY_OLD_( ( L"\t%x\t%x\n", iom->second->m_objectID, iom->second->m_size ) )
    }

	GCHandleMap::iterator ihm;

    _DISPLAY_OLD_( ( L"gc_Verify: GCHandle Map 0x%x entries\n", pGCCom->m_GCHandleMap.size() ) )
    for (ihm = pGCCom->m_GCHandleMap.begin(); ihm != pGCCom->m_GCHandleMap.end(); ihm++)
    {
    	_DISPLAY_OLD_( ( L"\t%x\t%x\n", ihm->second->m_handleID, ihm->second->m_objectID ) )
    }
    */


	if (gc_generationInternal)
		delete gc_generationInternal;

	DISPLAY(L"SUCCESS: Test Passed.\n");
    return hr;
}


void GC_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
	DISPLAY(L"Initialize GCCallbacks extension\n");

    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_GC
    							| COR_PRF_MONITOR_OBJECT_ALLOCATED
    							| COR_PRF_ENABLE_OBJECT_ALLOCATED;

    REGISTER_CALLBACK(VERIFY, gc_Verify);
    REGISTER_CALLBACK(GARBAGECOLLECTIONSTARTED, gc_GarbageCollectionStarted);
    REGISTER_CALLBACK(GARBAGECOLLECTIONFINISHED, gc_GarbageCollectionFinished);
    REGISTER_CALLBACK(SURVIVINGREFERENCES, gc_SurvivingReferences);
    REGISTER_CALLBACK(FINALIZEABLEOBJECTQUEUED, gc_FinalizeableObjectQueued);
    REGISTER_CALLBACK(ROOTREFERENCES2, gc_RootReferences2);
    REGISTER_CALLBACK(HANDLECREATED, gc_HandleCreated);
    REGISTER_CALLBACK(HANDLEDESTROYED, gc_HandleDestroyed);
    REGISTER_CALLBACK(OBJECTALLOCATED, gc_ObjectAllocated);
    REGISTER_CALLBACK(MOVEDREFERENCES, gc_MovedReferences);
    REGISTER_CALLBACK(OBJECTREFERENCES, gc_ObjectReferences);
    REGISTER_CALLBACK(PROFILERATTACHCOMPLETE, gc_ProfilerAttachComplete);

    gc_totalGCOperations = 0;

    return;
}
