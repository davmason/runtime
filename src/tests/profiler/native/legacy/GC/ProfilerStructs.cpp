// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "ProfilerStructs.h"
#include "../ProfilerCommonHelper.h"

PrfClassInfo::PrfClassInfo( ClassID classID ) : 
    m_classID( classID ),
    m_parentClassID ( 0 ),
    m_moduleID( 0 ),
    m_classToken( mdTypeDefNil ),
    m_isPremature ( 0 ),
    m_nTypeArgs( 0 ),
    m_objectsAllocated( 0 )
{
    m_className += L"UNKNOWN";
} 


PrfSuspensionInfo::PrfSuspensionInfo(ThreadID threadID, COR_PRF_SUSPEND_REASON suspendReason)
{
    m_reason = suspendReason;
    m_threadID = threadID;
}


PrfObjectInfo::PrfObjectInfo(ObjectID objectID, ClassID classID)
{
    m_objectID = objectID;
    m_newID = NULL;
    m_classID = classID;
    m_size = 0;
    m_isRoot = FALSE;
    m_isVisited = FALSE;
    m_pParent = NULL;
}


PrfObjectInfo::~PrfObjectInfo()
{
    m_referencesMap.clear();
}


PrfGCHandleInfo::PrfGCHandleInfo(GCHandleID handleID, ObjectID objectID)
{
    m_handleID = handleID;
    m_objectID = objectID;
}


PrfGCVariables::PrfGCVariables()
{
    reset();
}

VOID PrfGCVariables::reset()
{
    m_threadID = NULL;
       m_resumeStarted = FALSE;
    m_suspendCompleted = FALSE;
    m_referencesUpdated = FALSE;
    m_shutdownMode = FALSE;
    m_GCInShutdownOccurred = FALSE;

    // reset counters
    m_rootReferences = 0; 
    m_movedReferences = 0;         
    m_objectReferences = 0;
    m_objectsAllocated = 0; 
    m_objectsAllocatedByClass = 0;
}


PrfModuleInfo::~PrfModuleInfo()
{
    FreeAndEraseMap(m_functionMap);
}


