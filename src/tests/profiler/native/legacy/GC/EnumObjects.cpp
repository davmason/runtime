// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "GCCommon.h"

// Legacy support
#include "../LegacyCompat.h"
// Legacy support

HRESULT EnumObjectsVerifyGCState(IPrfCom * pPrfCom);

LONG EnumObjectstotalDeletedObjects;
LONG EnumObjectstotalGCOperations;

HRESULT EnumObjectsVerify(IPrfCom * pPrfCom)
{
	DISPLAY(L"Verify EnumObjects Callbacks\n");

        HRESULT hr = S_OK;

    if (EnumObjectstotalGCOperations == 0)
    {
		FAILURE(L"No GC Operation Took place\n");
        hr = E_FAIL;
    }


      return hr;
}


HRESULT EnumObjectsJITCachedFunctionSearchStarted(IPrfCom * /*pPrfCom*/, FunctionID /*functionID*/, BOOL *pbUseCachedFunction)
{
    *pbUseCachedFunction = TRUE;
    return S_OK;
}


HRESULT EnumObjectsClassLoadStarted(IPrfCom * pPrfCom, ClassID classID)
{
    HRESULT hr = S_OK;
    IGCCom * pGCCom = dynamic_cast<IGCCom *>(pPrfCom);

    if (TRUE == pGCCom->m_bForcedObjectCatchupMode)
        return S_OK;

    hr = pGCCom->AddClass(classID);
	if (FAILED(hr))
		FAILURE(L"AddClass in ClassLoadStarted returned hr = " << HEX(hr) << L" for ClassID " << HEX(classID) << L"\n");

    return hr;
}

HRESULT EnumObjectsClassUnloadFinished(IPrfCom * pPrfCom, ClassID classID, HRESULT /*hrStatus*/)
{
    HRESULT hr = S_OK;
    IGCCom * pGCCom = dynamic_cast<IGCCom *>(pPrfCom);

    if (TRUE == pGCCom->m_bForcedObjectCatchupMode)
        return S_OK;

    hr = pGCCom->RemoveClass(classID);
	if (FAILED(hr))
		FAILURE(L"RemoveClass in ClassUnloadFinished returned hr = " << HEX(hr) << L" for ClassID " << HEX(classID) << L"\n");

    return hr;
}

HRESULT EnumObjectsRuntimeSuspendStarted(IPrfCom * pPrfCom, COR_PRF_SUSPEND_REASON suspendReason)
{
    HRESULT hr = S_OK;
    IGCCom * pGCCom = dynamic_cast<IGCCom *>(pPrfCom);

    if (TRUE == pGCCom->m_bForcedObjectCatchupMode)
        return S_OK;

    hr = pGCCom->AddSuspension(suspendReason);
	if (FAILED(hr))
		FAILURE(L"AddSuspension in RuntimeSuspendStarted returned hr = " << HEX(hr) << L"\n");

    return hr;
}

HRESULT EnumObjectsRuntimeSuspendFinished(IPrfCom * pPrfCom)
{
    HRESULT hr = S_OK;
    IGCCom * pGCCom = dynamic_cast<IGCCom *>(pPrfCom);

    lock_guard<recursive_mutex> guard(pPrfCom->m_criticalSection);
    
    if (TRUE == pGCCom->m_bForcedObjectCatchupMode)
        return S_OK;

    if (pPrfCom->m_RuntimeSuspendStarted != (pPrfCom->m_RuntimeSuspendFinished + pPrfCom->m_RuntimeSuspendAborted))
    {
		FAILURE(L"RuntimeSuspendCounter error\n");
    }

    ThreadID threadID = NULL;

    hr = pPrfCom->m_pInfo->GetCurrentThreadID(&threadID);
    if ( !((hr == S_OK) || (hr == CORPROF_E_NOT_MANAGED_THREAD)) )
    {
		FAILURE(L"GetCurrentThreadID returned hr = " << HEX(hr) << L"\n");
    }

    auto iSuspensionMap = pGCCom->m_SuspensionMap.find(threadID);

    if (iSuspensionMap == pGCCom->m_SuspensionMap.end())
    {
		FAILURE(L"ThreadID " << HEX(threadID) << L"0x%x not found in table\n");
    }

    PrfSuspensionInfo *pSuspensionInfo = iSuspensionMap->second;

    if (pSuspensionInfo->m_reason == COR_PRF_SUSPEND_FOR_GC)
    {
        pGCCom->m_PrfGCVariables.m_threadID = threadID;
        pGCCom->m_PrfGCVariables.m_suspendCompleted = TRUE;
        pGCCom->m_PrfGCVariables.m_shutdownMode = FALSE;
        ++EnumObjectstotalGCOperations;
    }
    else if (pSuspensionInfo->m_reason == COR_PRF_SUSPEND_FOR_SHUTDOWN)
    {
        pGCCom->m_PrfGCVariables.m_threadID = threadID;
        pGCCom->m_PrfGCVariables.m_shutdownMode = TRUE;
        pGCCom->m_PrfGCVariables.m_GCInShutdownOccurred = FALSE;
    }

    pPrfCom->m_RuntimeSuspendStarted = 0;
    pPrfCom->m_RuntimeSuspendFinished = 0;
    pPrfCom->m_RuntimeSuspendAborted = 0;

    return hr;
}


HRESULT EnumObjectsRuntimeResumeStarted(IPrfCom * pPrfCom)
{
    IGCCom * pGCCom = dynamic_cast<IGCCom *>(pPrfCom);

    lock_guard<recursive_mutex> guard(pPrfCom->m_criticalSection);
    
    if (TRUE == pGCCom->m_bForcedObjectCatchupMode)
        return S_OK;

    COR_PRF_SUSPEND_REASON suspendReason = COR_PRF_SUSPEND_OTHER;
    pGCCom->CleanSuspensionList( &suspendReason );

    pPrfCom->m_RuntimeResumeStarted = 0;
    pPrfCom->m_RuntimeResumeFinished = 0;

    if ((suspendReason == COR_PRF_SUSPEND_FOR_GC) || (suspendReason == COR_PRF_SUSPEND_FOR_SHUTDOWN))
    {
		if (SUCCEEDED(EnumObjectsVerifyGCState(pPrfCom)))
			pGCCom->m_PrfGCVariables.reset();
		else
			FAILURE(L"GCState Verification FAILED");
    }

    return S_OK;
}

HRESULT EnumObjectsModuleLoadFinished(IPrfCom * pPrfCom, ModuleID moduleID, HRESULT /*hrStatus*/)
{
    HRESULT hr = S_OK;
    IGCCom * pGCCom = dynamic_cast<IGCCom *>(pPrfCom);

    if (TRUE == pGCCom->m_bForcedObjectCatchupMode)
        return S_OK;

    ICorProfilerObjectEnum *pObjectEnum;

    hr = pPrfCom->m_pInfo->EnumModuleFrozenObjects( moduleID, &pObjectEnum);

    if (hr == CORPROF_E_DATAINCOMPLETE)
    {        
		DISPLAY(L"EnumModuleFrozenObjects returned CORPROF_E_DATAINCOMPLETE\n");
        return S_OK;
    }

    if (FAILED(hr))
	{
		FAILURE(L"EnumModuleFrozenObjects returned hr = " << HEX(hr) << L"\n");
		return hr;			
	}

    ULONG frozenCount = 0;
    hr = pObjectEnum->GetCount(&frozenCount);
	
	// Oct 15, 2007: There should be no more frozen objects in modules (after the removal of 
	// hardbinding between NGen images, String Freezing has been disabled
	// and frozen objects will not be created)
    if (FAILED(hr) || (frozenCount != 0))
    {
		FAILURE(L"ICorProfilerObjectEnum::GetCount returned hr = " << HEX(hr) << L", " << frozenCount << L" Frozen Objects Found\n");
        return hr;
    }	
    return S_OK;
}


HRESULT EnumObjectsMovedReferences(IPrfCom * pPrfCom, ULONG cmovedObjectIDRanges,
                            ObjectID oldObjectIDRangeStart[],
                            ObjectID newObjectIDRangeStart[],
                            ULONG cObjectIDRangeLength[])
{
    IGCCom * pGCCom = dynamic_cast<IGCCom *>(pPrfCom);

    if (TRUE == pGCCom->m_bForcedObjectCatchupMode)
        return S_OK;

    {
        lock_guard<recursive_mutex> guard(pPrfCom->m_criticalSection);

		if (!((pGCCom->m_PrfGCVariables.m_suspendCompleted == TRUE) && (pGCCom->m_PrfGCVariables.m_resumeStarted == FALSE)))
			FAILURE(L"Boolean variables of GCStruct do not have the expected value.\n");

        pGCCom->m_PrfGCVariables.m_movedReferences++;

        ObjectMap::iterator iom;
        for (iom = pGCCom->m_ObjectMap.begin(); iom != pGCCom->m_ObjectMap.end(); ++iom)
        {
            for (ULONG i = 0; i < cmovedObjectIDRanges; i++)
            {
                UINT_PTR objectID = iom->second->m_objectID;

                if ((oldObjectIDRangeStart[i] <= objectID) &&
                    ((oldObjectIDRangeStart[i] + cObjectIDRangeLength[i]) > objectID))
                {
                    // We should move the new ID appropiately only if they actually move
                    if (newObjectIDRangeStart[i] != oldObjectIDRangeStart[i])
                    {
                        objectID = objectID + newObjectIDRangeStart[i] - oldObjectIDRangeStart[i];
                        iom->second->m_newID = objectID;

                        // Perform a sanity verification
                        pGCCom->VerifyObject(iom->second);

                        if (i != cmovedObjectIDRanges - 1)
                        {
                            // Check to see if the object has been omved to within the range of another object
                            if ((oldObjectIDRangeStart[i + 1] < oldObjectIDRangeStart[i] + cObjectIDRangeLength[i]) &&
                                (oldObjectIDRangeStart[i + 1] > oldObjectIDRangeStart[i]))
                            {
								FAILURE(L"ERROR: OldObjectReferences Overlapping\n");
                            }
                        }
                    }
                }
            }
        }
    }
    
    return S_OK;
}

HRESULT EnumObjectsObjectAllocated(IPrfCom * pPrfCom, ObjectID objectID, ClassID classID)
{
    HRESULT hr = S_OK;
    IGCCom * pGCCom = dynamic_cast<IGCCom *>(pPrfCom);

    lock_guard<recursive_mutex> guard(pPrfCom->m_criticalSection);

    ClassIDMap::iterator icm;

    if (FALSE == pGCCom->m_bForcedObjectCatchupMode) {

        if ( (pGCCom->m_PrfGCVariables.m_shutdownMode == TRUE) && (pGCCom->m_PrfGCVariables.m_GCInShutdownOccurred == TRUE))
        {
            hr = EnumObjectsVerifyGCState(pPrfCom);
            if (FAILED(hr))
            {
				FAILURE(L"GCState Verification FAILED\n");
                goto ErrReturn;
            }

            pGCCom->m_PrfGCVariables.m_resumeStarted = FALSE;
            pGCCom->m_PrfGCVariables.m_suspendCompleted = FALSE;
            pGCCom->m_PrfGCVariables.m_referencesUpdated = FALSE;
            pGCCom->m_PrfGCVariables.m_rootReferences = 0;
            pGCCom->m_PrfGCVariables.m_movedReferences = 0;
            pGCCom->m_PrfGCVariables.m_objectReferences = 0;
            pGCCom->m_PrfGCVariables.m_objectsAllocated = 0;
            pGCCom->m_PrfGCVariables.m_objectsAllocatedByClass = 0;
            pGCCom->m_PrfGCVariables.m_GCInShutdownOccurred = FALSE;
        }

        if ( !(pGCCom->m_PrfGCVariables.m_suspendCompleted == FALSE) && (pGCCom->m_PrfGCVariables.m_resumeStarted == FALSE))
        {
			FAILURE(L"ERROR: ObjectAllocated() Callback Received While in GC Suspension\n");
            hr = E_FAIL;
            goto ErrReturn;
        }

        if ((void*)classID == NULL)
        {
			FAILURE(L"ERROR: ClassID == NULL\n");
            hr = E_FAIL;
            goto ErrReturn;
        }

        ClassID verifyClassID = NULL;
        hr = pPrfCom->m_pInfo->GetClassFromObject(objectID, &verifyClassID);
        if ((FAILED(hr)) || (classID != verifyClassID))
        {
			FAILURE(L"GetClassFromObject returned hr = " << HEX(hr) << L", classID " << HEX(classID) << L", verifyClassID " << HEX(verifyClassID) << L"\n");
            hr = E_FAIL;
            goto ErrReturn;
        }

    }

    icm = pGCCom->m_ClassIDMap.find(classID);
    // classID corresponds to a table
    if (icm == pGCCom->m_ClassIDMap.end())
    {
        // verify that the classid is an array
        hr = pPrfCom->m_pInfo->IsArrayClass(classID, NULL, NULL, NULL);
        if (hr == S_OK)
        {
            // Add that array in the list so next time you run across it you already have it there
            hr = pGCCom->AddClass(classID);
            if (FAILED(hr))
            {
				FAILURE(L"AddClass for ClassID " << HEX(classID) << L" failed\n");
                goto ErrReturn;
            }
        }
        else if (hr == S_FALSE)
        {
            hr = pGCCom->AddClass(classID, TRUE);
            if (FAILED(hr))
            {
				FAILURE(L"AddClass for ClassID " << HEX(classID) << L" failed\n");
                goto ErrReturn;
            }
        }

         icm = pGCCom->m_ClassIDMap.find(classID);
         if (icm == pGCCom->m_ClassIDMap.end())
         {
			 FAILURE(L"ClassID not found in ClassInfoMap for ClassID " << HEX(classID) << L" failed\n");
             hr = E_FAIL;
             goto ErrReturn;
         }

    }

    //case 1 or 2
    if (SUCCEEDED(hr) && (icm->second != NULL))
    {
        hr = pGCCom->AddObject(objectID, classID);
        if (FAILED(hr))
        {
			FAILURE(L"AddObject failed for ObjectID " << HEX(objectID) << L" for classID " << HEX(classID) << L"\n");
            goto ErrReturn;
        }
    }

ErrReturn:

    return hr;
}


HRESULT EnumObjectsObjectAllocatedByClass(IPrfCom * pPrfCom, ULONG classCount, ClassID classIDs[], ULONG /*objects[]*/)
{
    HRESULT hr = S_OK;
    IGCCom * pGCCom = dynamic_cast<IGCCom *>(pPrfCom);

    if (TRUE == pGCCom->m_bForcedObjectCatchupMode)
        return S_OK;

    lock_guard<recursive_mutex> guard(pPrfCom->m_criticalSection);

    if ((pGCCom->m_PrfGCVariables.m_shutdownMode == TRUE) && (pGCCom->m_PrfGCVariables.m_GCInShutdownOccurred == FALSE))
    {
        // update data members
        pGCCom->m_PrfGCVariables.m_referencesUpdated = FALSE;
        pGCCom->m_PrfGCVariables.m_GCInShutdownOccurred = TRUE;
        pGCCom->m_PrfGCVariables.m_suspendCompleted = TRUE;

        EnumObjectstotalGCOperations++;
    }

	if ((pGCCom->m_PrfGCVariables.m_suspendCompleted == TRUE) && (pGCCom->m_PrfGCVariables.m_resumeStarted == FALSE))
	{
		pGCCom->m_PrfGCVariables.m_objectsAllocatedByClass++;

		if (classCount == 0)
		{
			FAILURE(L"ObjectAllocatedByClass returned a classCount == 0\n");
			hr = E_FAIL;
			goto ErrReturn;
		}


		ClassIDMap::iterator icm;

		for (ULONG i = 0; i < classCount; i++)
		{
			if ((void*)(classIDs[i]) == NULL)
			{
				FAILURE(L"ObjectAllocatedByClass returned an invalid ClassID\n");
				hr = E_FAIL;
				goto ErrReturn;
			}

			icm = pGCCom->m_ClassIDMap.find(classIDs[i]);
			if (icm == pGCCom->m_ClassIDMap.end())
			{
				FAILURE(L"ObjectAllocatedByClass classID was never encountered before\n");
				hr = E_FAIL;
				goto ErrReturn;
			}
		}
	}
	else
		FAILURE(L"Boolean variables of m_PrfGCVariables do not have the expected values\n");

ErrReturn:
    return hr;
}


HRESULT EnumObjectsObjectReferences(IPrfCom * pPrfCom, ObjectID objectID, ClassID classID, ULONG objectRefs, ObjectID objectRefIDs[])
{
    HRESULT hr = S_OK;
    IGCCom * pGCCom = dynamic_cast<IGCCom *>(pPrfCom);

    if (TRUE == pGCCom->m_bForcedObjectCatchupMode) {
        EnumObjectsObjectAllocated(pPrfCom, objectID, classID);        
        return S_OK;
    }

    lock_guard<recursive_mutex> guard(pPrfCom->m_criticalSection);

	if ((pGCCom->m_PrfGCVariables.m_suspendCompleted == TRUE) && (pGCCom->m_PrfGCVariables.m_resumeStarted == FALSE))
	{
		pGCCom->m_PrfGCVariables.m_objectReferences++;
		if (pGCCom->m_PrfGCVariables.m_referencesUpdated == FALSE)
		{
			pGCCom->CleanObjectTable(GC_STARTS);
			pGCCom->m_PrfGCVariables.m_referencesUpdated = TRUE;
		}

		if ((void*)objectID == NULL)
		{
			FAILURE(L"ObjectID == NULL\n");
			hr = E_FAIL;
			goto ErrReturn;
		}

		ClassID classID2 = NULL;

		hr = pPrfCom->m_pInfo->GetClassFromObject(objectID, &classID2);
		if (FAILED(hr))
		{
			FAILURE(L"GetClassFromObject failed for ObjectID " << HEX(objectID) << L"\n");
			hr = E_FAIL;
			goto ErrReturn;
		}

		if (classID != classID2)
		{
			FAILURE(L"ClassID mismatch from ClassID returned from GetClassFromObject\n");
			hr = E_FAIL;
			goto ErrReturn;
		}

		if ((void*)classID != NULL)
		{
			ClassIDMap::iterator icm;
			icm = pGCCom->m_ClassIDMap.find(classID);

			if (icm == pGCCom->m_ClassIDMap.end())
			{
				FAILURE(L"Entry with the given ClassID not found in the class list\n");
				hr = E_FAIL;
				goto ErrReturn;
			}
		}

		hr = pGCCom->UpdateAndVerifyReferences(objectID, objectRefs, objectRefIDs);

	}
	else
		FAILURE(L"Boolean variables of m_PrfGCVariables do not have the expected values\n");

ErrReturn:
    return hr;
}


HRESULT EnumObjectsRootReferences(IPrfCom * pPrfCom, ULONG rootRefs, ObjectID rootRefIDs[])
{
    HRESULT hr = S_OK;
    IGCCom * pGCCom = dynamic_cast<IGCCom *>(pPrfCom);

    if (TRUE == pGCCom->m_bForcedObjectCatchupMode)
        return S_OK;

    lock_guard<recursive_mutex> guard(pPrfCom->m_criticalSection);

    if ((pGCCom->m_PrfGCVariables.m_suspendCompleted == TRUE) && (pGCCom->m_PrfGCVariables.m_resumeStarted == FALSE))
    {
        pGCCom->m_PrfGCVariables.m_rootReferences++;
        if (pGCCom->m_PrfGCVariables.m_referencesUpdated == FALSE)
        {
            pGCCom->CleanObjectTable( GC_STARTS );
            pGCCom->m_PrfGCVariables.m_referencesUpdated = TRUE;
        }

        for (ULONG i = 0; i < rootRefs; i++)
        {
            if ((void*)(rootRefIDs[i]) != NULL)
            {
                ObjectMap::iterator iom;
                iom = pGCCom->m_ObjectMap.find(rootRefIDs[i]);
                if (iom != pGCCom->m_ObjectMap.end())
                {
                    iom->second->m_isRoot = TRUE;
                }
                else
                {
                    FAILURE( L"RootReference ObjectID " << HEX(rootRefIDs[i]) << L" not found in m_ObjectMap following CleanObjectTable \n");
                }
            }
        }
    }
    else
        FAILURE(L"Boolean variables of m_PrfGCVariables do not have the expected values\n");

    return hr;
}


HRESULT EnumObjectsProfilerAttachComplete(IPrfCom * pPrfCom)
{
    HRESULT hr = S_OK;
    IGCCom * pGCCom = dynamic_cast<IGCCom *>(pPrfCom);   

    // We are also including a small test for Dev10: 588009. The code has been tweaked as follows - 
    // 1. Call SetEventMask with COR_PRF_MONITOR_GC flag.
    // 2. Call SetEventMask again with the rest of the mutable flags -
    //  - COR_PRF_MONITOR_SUSPENDS
    //  - COR_PRF_MONITOR_CLASS_LOADS
    //  - COR_PRF_MONITOR_MODULE_LOADS
    // 3. Finally set the immutable flags under the test hook
    //  - COR_PRF_MONITOR_OBJECT_ALLOCATED
    //  - COR_PRF_ENABLE_OBJECT_ALLOCATED

    DWORD dwFlags = COR_PRF_MONITOR_GC;

    // HACK ALERT: We are setting the event-mask ourselves explicitly instead of relying on BPD to do it.
    hr = pPrfCom->m_pInfo->SetEventMask(dwFlags| COR_PRF_MONITOR_APPDOMAIN_LOADS);
	if (S_OK != hr) {
		FAILURE(L"SetEventMask returned hr = " << HEX(hr) << L"\n");
	}
	else
		DISPLAY(L"ProfilerEventMask: " << HEX(dwFlags) << L"\n");

    dwFlags |= COR_PRF_MONITOR_SUSPENDS    |
               COR_PRF_MONITOR_CLASS_LOADS | 
               COR_PRF_MONITOR_MODULE_LOADS;

    hr = pPrfCom->m_pInfo->SetEventMask(dwFlags | COR_PRF_MONITOR_APPDOMAIN_LOADS);
	if (S_OK != hr) {
		FAILURE(L"SetEventMask returned hr = " << HEX(hr) << L"\n");
	}
	else
		DISPLAY(L"ProfilerEventMask:" << HEX(dwFlags) << L"\n");

    dwFlags |= COR_PRF_MONITOR_OBJECT_ALLOCATED |
               COR_PRF_ENABLE_OBJECT_ALLOCATED;
    
    hr = pPrfCom->m_pInfo->SetEventMask(dwFlags | COR_PRF_MONITOR_APPDOMAIN_LOADS);
	if (S_OK != hr) {
		FAILURE(L"SetEventMask returned hr = " << HEX(hr) << L"\n");
	}
	else
		DISPLAY(L"ProfilerEventMask: " << HEX(dwFlags) << "\n");
 

    EnumObjectstotalDeletedObjects = 0;
    EnumObjectstotalGCOperations = 0;

    //        
    // We are now triggering the GC ourselves. 
    //
    pGCCom->m_bForcedObjectCatchupMode = TRUE;
    hr = pPrfCom->m_pInfo->ForceGC();
    if (S_OK != hr)
    {
		FAILURE(L"SetEventMask returned hr = " << HEX(hr) << L"\n");
    }
    pGCCom->m_bForcedObjectCatchupMode = FALSE;

    return hr;
}


void EnumObjectsInitialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
	DISPLAY(L"**** Profiler API Test: Verify GC callbacks ****\n");

    // NOTE: If this test is running in attach mode, we need to catch-up on objectIDs before we start testing.
    // We will initiate a Forced GC collection (Info::ForceGC) from inside the ProfilerAttachComplete 
    // callback (the only callback where it is safe to trigger a GC synchronously). Once we catch up with 
    // the object states, we will then actually start the test.

    if(FALSE == pPrfCom->m_bAttachMode) {
        pModuleMethodTable->FLAGS = COR_PRF_MONITOR_GC
                                    | COR_PRF_MONITOR_SUSPENDS
                                    | COR_PRF_MONITOR_CLASS_LOADS
                                    | COR_PRF_MONITOR_OBJECT_ALLOCATED
                                    | COR_PRF_ENABLE_OBJECT_ALLOCATED
                                    | COR_PRF_MONITOR_MODULE_LOADS;
    }

    pModuleMethodTable->VERIFY = (FC_VERIFY) &EnumObjectsVerify;
    pModuleMethodTable->JITCACHEDFUNCTIONSEARCHSTARTED = (FC_JITCACHEDFUNCTIONSEARCHSTARTED) &EnumObjectsJITCachedFunctionSearchStarted;
    pModuleMethodTable->CLASSLOADSTARTED = (FC_CLASSLOADSTARTED) &EnumObjectsClassLoadStarted;
    pModuleMethodTable->CLASSUNLOADFINISHED = (FC_CLASSUNLOADFINISHED) &EnumObjectsClassUnloadFinished;
    pModuleMethodTable->RUNTIMESUSPENDSTARTED = (FC_RUNTIMESUSPENDSTARTED) &EnumObjectsRuntimeSuspendStarted;
    pModuleMethodTable->RUNTIMESUSPENDFINISHED = (FC_RUNTIMESUSPENDFINISHED) &EnumObjectsRuntimeSuspendFinished;
    pModuleMethodTable->RUNTIMERESUMESTARTED = (FC_RUNTIMERESUMESTARTED) &EnumObjectsRuntimeResumeStarted;
    pModuleMethodTable->MODULELOADFINISHED = (FC_MODULELOADFINISHED) &EnumObjectsModuleLoadFinished;
    pModuleMethodTable->MOVEDREFERENCES = (FC_MOVEDREFERENCES) &EnumObjectsMovedReferences;
    pModuleMethodTable->OBJECTALLOCATED = (FC_OBJECTALLOCATED) &EnumObjectsObjectAllocated;
    pModuleMethodTable->OBJECTSALLOCATEDBYCLASS = (FC_OBJECTSALLOCATEDBYCLASS) &EnumObjectsObjectAllocatedByClass;
    pModuleMethodTable->OBJECTREFERENCES = (FC_OBJECTREFERENCES) &EnumObjectsObjectReferences;
    pModuleMethodTable->ROOTREFERENCES = (FC_ROOTREFERENCES) &EnumObjectsRootReferences;
    pModuleMethodTable->PROFILERATTACHCOMPLETE = (FC_PROFILERATTACHCOMPLETE) & EnumObjectsProfilerAttachComplete;

    EnumObjectstotalDeletedObjects = 0;
    EnumObjectstotalGCOperations = 0;
    return;
}

HRESULT EnumObjectsVerifyGCState(IPrfCom * pPrfCom)
{
    HRESULT hr = S_OK;
    IGCCom * pGCCom = dynamic_cast<IGCCom *>(pPrfCom);

    //
    // case 1: Not in a shutdown
    // case 2: In a shutdown but not any GC's took place
    //
    if (  (pGCCom->m_PrfGCVariables.m_shutdownMode == FALSE)
        ||( (pGCCom->m_PrfGCVariables.m_shutdownMode == TRUE) && (pGCCom->m_PrfGCVariables.m_GCInShutdownOccurred == TRUE) ) )
    {
        if (   (pGCCom->m_PrfGCVariables.m_rootReferences == 0)
            && (pGCCom->m_PrfGCVariables.m_objectsAllocatedByClass == 0)
            && (pGCCom->m_PrfGCVariables.m_objectReferences == 0 ) )
        {
                FAILURE(L"Error: GC Counters Are Zero\n" );
        }
        else
        {
              //
              // we will not verify the rest of the booleans since
              // they are checked in every GC callback
              //
              if ( pGCCom->m_PrfGCVariables.m_rootReferences == 0 )
              {
                hr = E_FAIL;
                  FAILURE(L"Unexpected number of RootReferences callback");
              }

            hr = pGCCom->VerifyObjectList();
			if (FAILED(hr))
				FAILURE(L"Object List Verification FAILED");
        }
    }

    return hr;
}



