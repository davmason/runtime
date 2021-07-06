// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "GCCommon.h"
#include <algorithm>

// Modified print macros for use in ProfilerCommon.cpp
#undef DISPLAY
#undef LOG
#undef FAILURE

#define DISPLAY( message ) {                                                                        \
                                std::wstringstream temp;                                            \
                                temp << message;                                                    \
                                Display(temp);                                                      \
                           }

#define FAILURE( message ) {                                                                        \
                                std::wstringstream temp;                                            \
                                temp << message;                                                    \
                                Error(temp) ;                                                       \
                           }


// Modified ICorProfilerInfo macro for use in ProfilerCommon
#undef  PINFO
#define PINFO m_pInfo

GCCommon::GCCommon(ICorProfilerInfo3 * pInfo)
            :PrfCommon(pInfo)
{
}


GCCommon::~GCCommon()
{
    FreeAndEraseMap(m_ClassIDMap);
    FreeAndEraseMap(m_SuspensionMap);
    FreeAndEraseMap(m_ObjectMap);
    FreeAndEraseMap(m_GCHandleMap);
    FreeAndEraseMap(m_ModuleMap);
    FreeAndEraseMap(m_FunctionMap);
    FreeAndEraseMap(m_FrozenSegmentMap);
}



//---------------------------------------------------------------------------------------
//
// GCCommon::AddClass
//
// Arguments:
//      classID - <Description>
//      isPremature - True when ClassID in ObjectAllocated has not been seen before.
//
// Return Value:
//    Description of the values this function could return
//    and under what conditions
//
// Assumptions:
//      It is safe to call: 
//          GetClassIDInfo2 && GetModuleMetaData
//
// Notes:
//      EnumObjects calls this function from:
//          ClassLoadStarted
//          ObjectAllocated
//

HRESULT GCCommon::AddClass(ClassID classID, BOOL isPremature)
{
    HRESULT hr = S_OK;

    // Look for the ClassID
    auto iClassIDMap = m_ClassIDMap.find(classID);

    // If it is not found, add it.
    if (iClassIDMap == m_ClassIDMap.end())
    {
        PrfClassInfo *pClassInfo = new PrfClassInfo(classID);

        if (pClassInfo == NULL)
            return E_FAIL;

        pClassInfo->m_isPremature = isPremature;

        hr = m_pInfo->GetClassIDInfo2(pClassInfo->m_classID,
                                      &pClassInfo->m_moduleID,
                                      &pClassInfo->m_classToken,
                                      &pClassInfo->m_parentClassID,
                                      5,
                                      &pClassInfo->m_nTypeArgs,
                                      pClassInfo->m_typeArgs);

        if (SUCCEEDED(hr))
            GetClassIDName(classID, pClassInfo->m_className, false);

        m_ClassIDMap.insert(make_pair(classID, pClassInfo));
    }
    else
    {
        //FAILURE(L"NOT A BUG! - Do we ever actually load classes early?");

        if (iClassIDMap->second->m_isPremature == TRUE)
        {
            hr = m_pInfo->GetClassIDInfo2(iClassIDMap->second->m_classID,
                                          &iClassIDMap->second->m_moduleID,
                                          &iClassIDMap->second->m_classToken,
                                          &iClassIDMap->second->m_parentClassID,
                                          5,
                                          &iClassIDMap->second->m_nTypeArgs,
                                          iClassIDMap->second->m_typeArgs);

            if (SUCCEEDED(hr))
                GetClassIDName(classID, iClassIDMap->second->m_className, false);
        }
        else
        {
            FAILURE(L"Class already exists in table and was not loaded early.\n");
            return E_FAIL;
        }
    }
    return S_OK;
}


HRESULT GCCommon::RemoveClass(ClassID classID)
{
    if (classID == 0)
        return E_FAIL;

    // Look for the ClassID
    auto iClassIDMap = m_ClassIDMap.find(classID);

    // If it is found, delete it.
    if (iClassIDMap != m_ClassIDMap.end())
    {
        delete iClassIDMap->second;
        m_ClassIDMap.erase(iClassIDMap);
    }

    // Ngened images from Whidbey on perform class loading at Ngen time.  Since no ClassLoad callbacks are
    // received from the runtime it is legal to unload a class you did not load.

    return S_OK;
}


HRESULT GCCommon::AddSuspension(COR_PRF_SUSPEND_REASON suspendReason)
{
    HRESULT hr = S_OK;
    ThreadID threadID = NULL;

    lock_guard<mutex> guard(m_privateCriticalSection);

    //ToDO: Using threadID when error value returned.  Crazy.
    hr = m_pInfo->GetCurrentThreadID(&threadID);
    if ((hr == S_OK) || (hr == CORPROF_E_NOT_MANAGED_THREAD))
    {
        PrfSuspensionInfo *pPrfSuspensionInfo = NULL;
        pPrfSuspensionInfo = new PrfSuspensionInfo(threadID, suspendReason);

        if (pPrfSuspensionInfo != NULL)
            m_SuspensionMap.insert(make_pair(threadID, pPrfSuspensionInfo));
    }

    return S_OK;
}


void GCCommon::CleanSuspensionList(COR_PRF_SUSPEND_REASON *suspendReason)
{
    HRESULT hr = S_OK;
    ThreadID threadID = 0;

	lock_guard<mutex> guard(m_privateCriticalSection);
    if (m_SuspensionMap.size() < 1)
        FAILURE(L"Zero elements in the SuspensionList to clean.\n");

    hr = m_pInfo->GetCurrentThreadID(&threadID);
    if ((hr == S_OK) || (hr == CORPROF_E_NOT_MANAGED_THREAD))
    {
        auto iSuspensionMap = m_SuspensionMap.find(threadID);

        if (iSuspensionMap == m_SuspensionMap.end())
        {
            FAILURE(L"ThreadID element not found in SuspensionList.\n");
        }
        else
        {
            (*suspendReason) = iSuspensionMap->second->m_reason;

            delete iSuspensionMap->second;
            m_SuspensionMap.erase(iSuspensionMap);
        }
    }
}


HRESULT GCCommon::AddObject(ObjectID objectID, ClassID classID)
{
    auto iObjectMap = m_ObjectMap.find(objectID);

    // ObjectID not in table.  Add it.
    if (iObjectMap == m_ObjectMap.end())
    {
        PrfObjectInfo *pPrfObjectInfo = new PrfObjectInfo(objectID, classID);

        HRESULT hr = m_pInfo4->GetObjectSize2(objectID, &pPrfObjectInfo->m_size);
        if (SUCCEEDED(hr) && pPrfObjectInfo->m_size > ULONG_MAX)
        {            
            ULONG size;
            hr = m_pInfo->GetObjectSize(objectID, &size);
            if (hr != COR_E_OVERFLOW)
            {
                delete pPrfObjectInfo;
                FAILURE(L"ICorProfilerInfo::GetObjectSize returned hr = " << hr << L" in GCCommon::AddObject for Object ID = " << objectID << L"while we expect hr = COR_E_OVERFLOW");
                return E_FAIL;
            }
            else
            {
                DISPLAY(L"Seen 4GB+ object @ 0x" << HEX(objectID));
            }
        }

        m_ObjectMap.insert(make_pair(objectID, pPrfObjectInfo));
    }

    return S_OK;

}


HRESULT GCCommon::VerifyObject(PrfObjectInfo *pObjectInfo)
{
    if (pObjectInfo == NULL)
    {
        FAILURE(L"NULL PrfObjectInfo passed to GCCommon::VerifyObject.\n");
        return E_FAIL;
    }

    HRESULT hr = S_OK;
    SIZE_T size = 0;
    ClassID classID = NULL;

    hr = m_pInfo->GetClassFromObject(pObjectInfo->m_objectID, &classID);
    if (FAILED(hr))
    {
        FAILURE(L"ICorProfilerInfo::GetClassFromObject returned hr = " << hr << L" in GCCommon::VerifyObject for Object ID = " << pObjectInfo->m_objectID);
        return E_FAIL;
    }

	if (classID == 0)
	{
		return S_OK;
	}

    if (pObjectInfo->m_classID != classID)
    {
        FAILURE(L"ClassID's do not match (" << pObjectInfo->m_classID << L" != " << classID << L") for object " << pObjectInfo->m_objectID);
        return E_FAIL;
    }

    hr = m_pInfo4->GetObjectSize2(pObjectInfo->m_objectID, &size);
    if (FAILED(hr))
    {
        FAILURE(L"ICorProfilerInfo::GetObjectSize2 returned hr = " << hr << L" in GCCommon::VerifyObject for Object ID = " << pObjectInfo->m_objectID);
        return E_FAIL;
    }
    else
    {
        if (size > ULONG_MAX)
        {
            ULONG ulSize;
            hr = m_pInfo->GetObjectSize(pObjectInfo->m_objectID, &ulSize);
            if (hr != COR_E_OVERFLOW)
            {
                FAILURE(L"ICorProfilerInfo::GetObjectSize returned hr = " << hr << L" in GCCommon::VerifyObject for Object ID = " << pObjectInfo->m_objectID << L"while we expect hr = COR_E_OVERFLOW");
                return E_FAIL;
            }
            else
            {
                DISPLAY(L"Seen 4GB+ object @ 0x" << HEX(pObjectInfo->m_objectID));
            }
        }
    }
    

    if (pObjectInfo->m_size != size)
    {
        FAILURE(L"Sizes do not match (" << pObjectInfo->m_size << L" != " << size << L") for object " << pObjectInfo->m_objectID);
        return E_FAIL;
    }

    return S_OK;

}

VOID GCCommon::MarkObjectsAsVisited()
{
    ObjectMap::iterator iom;
     for ( iom = m_ObjectMap.begin(); iom != m_ObjectMap.end(); ++iom)
     {
        if (iom->second->m_newID != 0)
        {
            ObjectMap::iterator i2;
            i2 = m_ObjectMap.find(iom->second->m_newID);

            if (i2 != m_ObjectMap.end())
            {
                if (i2->second->m_newID == 0)
                {
                    i2->second->m_isVisited = TRUE;
                }
            }
         }
    }
}

VOID GCCommon::CheckMarkedObjects( PrfObjectInfo *pCursor )
{
    ObjectMap::iterator iom;
    // maybe there are no References
    if ( pCursor->m_referencesMap.size() != 0)
    {
        // loop through all the root references in the object list
        for ( pCursor->m_irm = pCursor->m_referencesMap.begin();
            pCursor->m_irm != pCursor->m_referencesMap.end();
              ++pCursor->m_irm )
        {
			ObjectID objID = pCursor->m_irm->second;
            iom = m_ObjectMap.find(objID);

            if (iom == m_ObjectMap.end())
            {
                FAILURE(L"ERROR: ObjectID was not found in the objects table.");
            }
            else if (iom->second->m_isVisited != TRUE)
            {
                FAILURE(L"ERROR: Never Received an ObjectReferences Callback for the Specific Object.");
            }
          }
    }
} // ProfilerCallback::_CheckMarkedObjects


VOID GCCommon::CleanObjectTable(GCEnum mode)
{
    //
    // this is a mark phase that is necessary to locate the elements that will not be ported over to the new
    // table
    //
    if (mode == GC_STARTS)
    {
        MarkObjectsAsVisited();
    }

    ObjectMap::iterator iom;
    BOOL increment = FALSE;
    for (iom = m_ObjectMap.begin(); iom != m_ObjectMap.end();)
    {
        increment = TRUE;
        //
        // deletes elements that were deleted before the GC started
        //
        switch (mode)
        {
            case GC_STARTS:
                {
                    if ( iom->second->m_isVisited == TRUE )
                    {
                        delete iom->second;
                        iom = m_ObjectMap.erase(iom);
                        increment = FALSE;
                    }
                    else
                    {
                        if (iom->second->m_newID != 0)
                        {
                            // We need to reinsert the PrfObjectInfo structure with the new ObjectID as the 
                            // key
                            PrfObjectInfo *pObjectInfo = iom->second;
                            iom->second = NULL;

                            pObjectInfo->m_objectID = pObjectInfo->m_newID;
                            pObjectInfo->m_newID = NULL;

                            m_ObjectMap.insert(make_pair(pObjectInfo->m_objectID, pObjectInfo));
                            iom = m_ObjectMap.erase(iom);
                            increment = FALSE;
                        }
                    }
                }
                break;
            case GC_COMPLETES:
                {
                    if ( iom->second->m_isVisited == FALSE )
                    {
                        delete iom->second;
                        iom = m_ObjectMap.erase(iom);
                        increment = FALSE;
                    }
                    else
                    {
                        // check the referenced objects
                        CheckMarkedObjects(iom->second);

                        if (iom->second->m_newID != 0)
                        {
                            // We need to reinsert the PrfObjectInfo structure with the new ObjectID as the 
                            // key
                            PrfObjectInfo *pObjectInfo = iom->second;
                            iom->second = NULL;

                            pObjectInfo->m_objectID = pObjectInfo->m_newID;
                            pObjectInfo->m_newID = NULL;

                            m_ObjectMap.insert(make_pair(pObjectInfo->m_objectID, pObjectInfo));
                            iom = m_ObjectMap.erase(iom);
                            increment = FALSE;
                        }
                    }
                }
                break;
			default:
				break;
        }

        if (increment == TRUE)
        {
            ++iom;
        }

    }

} // ProfilerCallback::_BuildTable


HRESULT GCCommon::UpdateAndVerifyReferences(ObjectID objectID, ULONG objectRefs, ObjectID objectRefIDs[])
{
    HRESULT hr = S_OK;

    auto iObjectMap = m_ObjectMap.find(objectID);

    if (iObjectMap == m_ObjectMap.end())
    {
        FAILURE(L"Object " << objectID << L" Was Not Found in the Objects Table");
        return E_FAIL;
    }

    PrfObjectInfo *pObjectInfo = iObjectMap->second;

    if (pObjectInfo->m_isVisited == TRUE)
    {
        FAILURE(L"TEST ERROR: Node in Object List Should Have Been Reset");
        return E_FAIL;
    }

    pObjectInfo->m_isVisited = TRUE;

    hr = VerifyObject(pObjectInfo);
    hr = UpdateObjectReferences(pObjectInfo, objectRefs, objectRefIDs);

    return hr;

}


HRESULT GCCommon::UpdateObjectReferences(PrfObjectInfo *pObjectInfo, ULONG objectRefs, ObjectID objectRefIDs[])
{
    for (ULONG i = 0; i < objectRefs; i++)
    {
        if (objectRefIDs[i] != 0)
        {
                pObjectInfo->m_referencesMap.insert(make_pair(objectRefIDs[i], objectRefIDs[i]));

                auto iObjectMap = m_ObjectMap.find(objectRefIDs[i]);
                if (iObjectMap != m_ObjectMap.end())
                {
                    iObjectMap->second->m_pParent = pObjectInfo;
                }
                else
                {
                    FAILURE(L"ERROR: Referenced ObjectID Does Not Exist In Object Table FAILED");
                    return E_FAIL;
                }
        }
    }

    return S_OK;
}


HRESULT GCCommon::VerifyObjectList()
{
    HRESULT hr = S_OK;

    CleanObjectTable( GC_COMPLETES );

    if (m_ObjectMap.size() != m_PrfGCVariables.m_objectReferences)
    {
        FAILURE(L"ERROR: Size of the ObjectTable After Deletions Not Equal to the ObjectReferences Received.");
    }

    // loop through all the root references in the object list
    ObjectMap::iterator iom;
    for ( iom = m_ObjectMap.begin(); iom != m_ObjectMap.end(); ++iom)
    {
        // reset the m_isRoot field
        iom->second->m_isRoot = FALSE;
        iom->second->m_isVisited = FALSE;

        // delete the old references
        for (iom->second->m_irm = iom->second->m_referencesMap.begin();
             iom->second->m_irm != iom->second->m_referencesMap.end(); )
        {
             iom->second->m_irm = iom->second->m_referencesMap.erase(iom->second->m_irm);
        }
    }

    ClassIDMap::iterator icm;
    for (icm = m_ClassIDMap.begin(); icm != m_ClassIDMap.end(); ++icm)
    {
        icm->second->m_objectsAllocated = 0;
    }

    return hr;
}


// Whidbey functions

//
// overloaded method for adding objects at MovedReferences callback
// cannot call ICorProfilerInfo::GetObjectSize because objectID still
// refers to old object, or invalid
//

HRESULT GCCommon::AddObject(ObjectID objectID, ClassID classID, ULONG size)
{
    auto iObjectMap = m_ObjectMap.find(objectID);

    // ObjectID not in table.  Add it.
    if (iObjectMap == m_ObjectMap.end())
    {
        PrfObjectInfo *pPrfObjectInfo = new PrfObjectInfo(objectID, classID);

        pPrfObjectInfo->m_size = size;

        m_ObjectMap.insert(make_pair(objectID, pPrfObjectInfo));
    }

    return S_OK;

}


HRESULT GCCommon::AddObjectReferences(ObjectID objectID, ULONG objectRefs, ObjectID objectRefIDs[], bool attach)
{
    ObjectMap::iterator iom;
    iom = m_ObjectMap.find(objectID);

    if (iom == m_ObjectMap.end())
    {
        if (attach == false)
        {
            FAILURE(L"AddObjectReferences called for ObjectID not in Map in non-Attach test.");
        }
        ClassID newClassID = NULL;
        MUST_PASS(PINFO->GetClassFromObject(objectID, &newClassID));
    	MUST_PASS(AddObject(objectID, newClassID));
		iom = m_ObjectMap.find(objectID);
    }

    for (ULONG i = 0; i < objectRefs; i++)
        iom->second->m_referencesMap.insert(make_pair(objectRefIDs[i], objectRefIDs[i]));

    return S_OK;
}


HRESULT GCCommon::VerifyObjectReferences(DWORD generationsCollected)
{
    HRESULT hr = S_OK;
    ObjectMap::iterator iom, iom2, iom_begin, iom_end;

    // all objects in the table has survived this GC the ones that doesn't have been removed at
    // gc_GarbageCollectionFinished reset the field to be used for verification
    for (iom = m_ObjectMap.begin(); iom != m_ObjectMap.end(); iom++)
         iom->second->m_isSurvived = FALSE;

    // keep track the range of unprocessed objects (to improve performance)
    iom_begin = m_ObjectMap.begin();
    iom_end = m_ObjectMap.end();

    // starting from root references, mark root and visited objects as survived, mark objects they referenced
    // as visited; objects marked as survived have be processed and can be skipped in subsequent passes.
    // repeat 'map::size' times so that the whole graph is traversed, or until
    // every object is processed.
    for (UINT i = 0; i < m_ObjectMap.size(); i++)
    {
        // update iom_begin ...
        while (iom_begin != m_ObjectMap.end() && iom_begin->second->m_isSurvived)
            iom_begin++;

        // ... done if the range of unprocessed objects is empty
        if (iom_begin == m_ObjectMap.end())
            break;

        // update iom_end to point to just beyond the last element to be processed
        iom_end--;
        while (iom_end != iom_begin && iom_end->second->m_isSurvived)
            iom_end--;
        iom_end++;

        for (iom = iom_begin; iom != iom_end; ++iom)
        {
            if ((iom->second->m_isRoot || iom->second->m_isVisited) && !iom->second->m_isSurvived)
            {
                // mark this object as survived
                iom->second->m_isSurvived = TRUE;

                // mark referenced objects as visited
                for (iom->second->m_irm = iom->second->m_referencesMap.begin();
                    iom->second->m_irm != iom->second->m_referencesMap.end(); )
                {
                    // referenced object must be in object map or within frozen segments
                    iom2 = m_ObjectMap.find(iom->second->m_irm->second);
                    if (iom2 == m_ObjectMap.end())
                    {
                        BOOL bFound = FALSE;

                        for (FrozenSegmentMap::iterator ifm = m_FrozenSegmentMap.begin(); ifm != m_FrozenSegmentMap.end(); ifm++)
                        {
                            if (ifm->second->rangeStart <= iom->second->m_irm->second && iom->second->m_irm->second < ifm->second->rangeStart + ifm->second->rangeLength)
                            {
                                bFound = TRUE;
                                break;
                            }
                        }

                        if (!bFound)
                        {
                            FAILURE(L"VerifyObjectReferences: object not found " << iom->second->m_irm->second);
                            hr = E_FAIL;
                        }
                    }
                    else
                        iom2->second->m_isVisited = TRUE;

                    iom->second->m_irm++;
                }
            }
        }
    }

    // verify all survived objects are reached, or in a generation not collected.
    for (iom = m_ObjectMap.begin(); iom != m_ObjectMap.end(); ++iom)
    {
        if (!iom->second->m_isSurvived)
        {
            COR_PRF_GC_GENERATION_RANGE range;
            m_pInfo->GetObjectGeneration(iom->second->m_objectID, &range);

            if ( !(generationsCollected & (1 << range.generation)) )
            {
                DISPLAY(L"Orphaned object " << iom->second->m_objectID << L" in generation " << range.generation);
            }
            else
            {
                // Not actually a failure, the GC can decide to keep any object it likes alive.
                DISPLAY(L"VerifyObjectReferences: survived object not reached " << iom->second->m_objectID << L" in generation " << range.generation);
            }
        }
        else
        {
            hr = VerifyObject(iom->second);
        }
    }

    return hr;
}


HRESULT GCCommon::ResetObjectTable()
{
    ObjectMap::iterator iom;

    for (iom = m_ObjectMap.begin(); iom != m_ObjectMap.end(); ++iom)
    {
        // reset fields
        iom->second->m_isRoot = FALSE;
        iom->second->m_isVisited = FALSE;
        iom->second->m_isSurvived = FALSE;
        iom->second->m_isQueuedForFinalize = FALSE;

        // delete object references
        for (iom->second->m_irm  = iom->second->m_referencesMap.begin();
             iom->second->m_irm != iom->second->m_referencesMap.end(); )
        {
            iom->second->m_irm = iom->second->m_referencesMap.erase(iom->second->m_irm);
        }
    }

    return S_OK;
}


HRESULT GCCommon::RemoveObject(ObjectID objectID)
{
    ObjectMap::iterator iom;

    iom = m_ObjectMap.find(objectID);

    // ObjectID in table. Remove it.
    if (iom != m_ObjectMap.end())
    {
        delete iom->second;
        m_ObjectMap.erase(iom);
        return S_OK;
    }

    return E_FAIL;
}


HRESULT GCCommon::AddGCHandle(GCHandleID handleID, ObjectID objectID)
{
    auto iGCHandleMap = m_GCHandleMap.find(handleID);

    // GCHandleID not in table. Add it.
    if (iGCHandleMap == m_GCHandleMap.end())
    {
        PrfGCHandleInfo *pPrfGCHandleInfo = NULL;
        pPrfGCHandleInfo = new PrfGCHandleInfo(handleID, objectID);

        if (pPrfGCHandleInfo == NULL)
            FAILURE(L"Allocation failed for PrfGCHandleInfo in GCCommon::AddGCHandle.");

        m_GCHandleMap.insert(make_pair(handleID, pPrfGCHandleInfo));
    }

    return S_OK;
}


HRESULT GCCommon::RemoveGCHandle(GCHandleID handleID)
{
    GCHandleMap::iterator ihm;

    ihm = m_GCHandleMap.find(handleID);

    // GCHandleID in table. Remove it.
    if (ihm != m_GCHandleMap.end())
    {
        delete ihm->second;
        m_GCHandleMap.erase(ihm);
        return S_OK;
    }

    return E_FAIL;
}


HRESULT GCCommon::AddFrozenSegment(COR_PRF_GC_GENERATION_RANGE frozenSegment)
{
    auto iFrozenSegmentMap = m_FrozenSegmentMap.find(frozenSegment.rangeStart);

    // frozon segment not in table. Add it.
    if (iFrozenSegmentMap == m_FrozenSegmentMap.end())
    {
        COR_PRF_GC_GENERATION_RANGE    *pFrozenSegment = new COR_PRF_GC_GENERATION_RANGE;
        if (pFrozenSegment == NULL)
        {
            FAILURE(L"Allocation failed for COR_PRF_GC_GENERATION_RANGE in GCCommon::AddFrozenSegment.");
        }

        *pFrozenSegment = frozenSegment;
        m_FrozenSegmentMap.insert(make_pair(frozenSegment.rangeStart, pFrozenSegment));
    }

    return S_OK;
}


PrfFunctionInfo * GCCommon::UpdateFunctionInfo(FunctionID funcId)
{
    PrfFunctionInfo *pfuncInfo = new(PrfFunctionInfo);

    pfuncInfo->m_functionID = funcId;

    // Get the ClassID, ModuleID, and FunctionToken of the FunctionID
    m_pInfo->GetFunctionInfo2(funcId,
                            NULL,
                            &pfuncInfo->m_classID,
                            &pfuncInfo->m_moduleID,
                            &pfuncInfo->m_functionToken,
                            0,
                            NULL,
                            NULL);

    // Update the Function Name
    GetFunctionIDName(funcId, pfuncInfo->m_functionName, true);

    // Get a pointer to the code for the method.
    m_pInfo->GetILFunctionBody(pfuncInfo->m_moduleID,
                            pfuncInfo->m_functionToken,
                            &pfuncInfo->m_pOldHeader,
                            &pfuncInfo->m_oldILCodeSize);

    pfuncInfo->m_newILCodeSize = pfuncInfo->m_oldILCodeSize;

    return pfuncInfo;

}


HRESULT GCCommon::AddFunctionToModule(PrfModuleInfo *pModInfo, FunctionID funcId)
{
    HRESULT hr = S_OK;

    PrfFunctionInfo *pFuncInfo = UpdateFunctionInfo(funcId);

    pModInfo->m_functionMap.insert(make_pair(funcId, pFuncInfo));

    return hr;
}


HRESULT GCCommon::AddFunctionToFunctionMap(FunctionID funcId)
{
    auto iFunctionMap = m_FunctionMap.find(funcId);

    if (iFunctionMap == m_FunctionMap.end())
    {
        PrfFunctionInfo *pFuncInfo = UpdateFunctionInfo(funcId);
        m_FunctionMap.insert(make_pair(funcId, pFuncInfo));
    }

    return S_OK;
}


PrfFunctionInfo *GCCommon::GetFunctionInfo(FunctionID funcId)
{
    auto iFunctionMap = m_FunctionMap.find(funcId);

    if (iFunctionMap != m_FunctionMap.end())
    {
        return iFunctionMap->second;
    }

    return NULL;
}
