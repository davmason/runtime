// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "../ProfilerCommon.h"
#include "ProfilerStructs.h"


#undef  PINFO
#define PINFO pPrfCom->m_pInfo
#undef  PINFO4
#define PINFO4 pPrfCom->m_pInfo4
#undef  PPRFCOM
#define PPRFCOM pPrfCom
#define PGCCOM pIGCCom

typedef multimap<ClassID, PrfClassInfo *> ClassIDMap;
typedef map<ThreadID, PrfSuspensionInfo *> SuspensionMap;
typedef map<ObjectID, PrfObjectInfo *> ObjectMap;
typedef map<GCHandleID, PrfGCHandleInfo *> GCHandleMap;
typedef map<ModuleID, PrfModuleInfo *> ModuleMap;
typedef map<FunctionID, PrfFunctionInfo *> FunctionMap;
typedef map<ObjectID, COR_PRF_GC_GENERATION_RANGE *> FrozenSegmentMap;

// TODO: Move to separate file when ProfilerCommon is split.
enum GCEnum
{
    GC_STARTS,
    GC_COMPLETES,
    GC_OTHER,
};


class DECLSPEC_NOVTABLE IGCCom
{
public:

        // Structures and function for keeping track of ClassID info
        ClassIDMap m_ClassIDMap;
        virtual HRESULT AddClass(ClassID classID, BOOL isPremature = FALSE) = 0;
        virtual HRESULT RemoveClass(ClassID classID) = 0;

        // Structures and function for keeping track of Suspension info
        SuspensionMap m_SuspensionMap;
        virtual HRESULT AddSuspension(COR_PRF_SUSPEND_REASON suspendReason) = 0;
        virtual void CleanSuspensionList(COR_PRF_SUSPEND_REASON *suspendReason) = 0;

        // Structures and function for keeping track of ObjectID info
        ObjectMap m_ObjectMap;
        virtual HRESULT AddObject(ObjectID objectID, ClassID classID) = 0;
        virtual HRESULT VerifyObjectList() = 0;
        virtual HRESULT VerifyObject(PrfObjectInfo *pObjectInfo) = 0;
        virtual VOID CleanObjectTable(GCEnum mode ) = 0;
        virtual HRESULT UpdateAndVerifyReferences(ObjectID objectID, ULONG objectRefs, ObjectID objectRefIDs[]) = 0;
        virtual HRESULT UpdateObjectReferences(PrfObjectInfo *pObjectInfo, ULONG objectRefs, ObjectID objectRefIDs[]) = 0;

        // Whidbey functions
        virtual HRESULT AddObject(ObjectID objectID, ClassID classID, ULONG size) = 0;
        virtual HRESULT AddObjectReferences(ObjectID objectID, ULONG objectRefs, ObjectID objectRefIDs[], bool attach = false) = 0;
        virtual HRESULT VerifyObjectReferences(DWORD generationsCollected) = 0;
        virtual HRESULT ResetObjectTable() = 0;
        virtual HRESULT RemoveObject(ObjectID objectID) = 0;

        // Structures and function for keeping track of GCHandleID info
        GCHandleMap m_GCHandleMap;
        virtual HRESULT AddGCHandle(GCHandleID handleID, ObjectID objectID) = 0;
        virtual HRESULT RemoveGCHandle(GCHandleID handleID) = 0;

        // Structures and function for keeping track of ModuleID info
        ModuleMap m_ModuleMap;
        virtual HRESULT AddFunctionToModule(PrfModuleInfo *pModInfo, FunctionID funcId) = 0;

        // Structures and function for keeping track of FunctionID info
        FunctionMap m_FunctionMap;
        virtual HRESULT AddFunctionToFunctionMap(FunctionID funcId) = 0;
        virtual PrfFunctionInfo *GetFunctionInfo(FunctionID funcId) = 0;

        // Structures and function for keeping track of frozen heap segment info
        FrozenSegmentMap m_FrozenSegmentMap;
        virtual HRESULT AddFrozenSegment(COR_PRF_GC_GENERATION_RANGE frozenSegment) = 0;

        // Structures and function for keeping track of current GC state
        PrfGCVariables m_PrfGCVariables;

        std::mutex m_privateCriticalSection;

        // The profiling API does not provide a direct mechanism for object catch-up/enumeration
        // upon attach. Hence we are forced to trigger a GC ourselves (using Info::ForceGC) and
        // rely on the objectIDs provided to us from the subsequently generated ObjectReference callbacks. 
        BOOL m_bForcedObjectCatchupMode; 
};


class GCCommon : public PrfCommon,
                 public IGCCom
{
public:
    GCCommon(ICorProfilerInfo3 * pInfo);
    ~GCCommon();

    virtual HRESULT AddFunctionToModule(PrfModuleInfo *pModInfo, FunctionID funcId);
    virtual HRESULT AddFunctionToFunctionMap(FunctionID funcId);
    virtual PrfFunctionInfo *GetFunctionInfo(FunctionID funcId);

    virtual HRESULT AddClass(ClassID classID, BOOL isPremature = FALSE);
    virtual HRESULT RemoveClass(ClassID classID);

    virtual HRESULT AddSuspension(COR_PRF_SUSPEND_REASON suspendReason);
    virtual void CleanSuspensionList(COR_PRF_SUSPEND_REASON *suspendReason);

    virtual HRESULT AddObject(ObjectID objectID, ClassID classID);
    virtual HRESULT VerifyObject(PrfObjectInfo *pObjectInfo);
    virtual HRESULT VerifyObjectList();
    virtual VOID CleanObjectTable(GCEnum mode );
    virtual HRESULT UpdateAndVerifyReferences(ObjectID objectID, ULONG objectRefs, ObjectID objectRefIDs[]);
    virtual HRESULT UpdateObjectReferences(PrfObjectInfo *pObjectInfo, ULONG objectRefs, ObjectID objectRefIDs[]);

    virtual HRESULT AddObject(ObjectID objectID, ClassID classID, ULONG size);
    virtual HRESULT AddObjectReferences(ObjectID objectID, ULONG objectRefs, ObjectID objectRefIDs[], bool attach = false);
    virtual HRESULT VerifyObjectReferences(DWORD generationsCollected);
    virtual HRESULT ResetObjectTable();
    virtual HRESULT RemoveObject(ObjectID objectID);

    virtual HRESULT AddGCHandle(GCHandleID handleID, ObjectID objectID);
    virtual HRESULT RemoveGCHandle(GCHandleID handleID);

    virtual HRESULT AddFrozenSegment(COR_PRF_GC_GENERATION_RANGE frozenSegment);

    // Private routines used in keeping track of ObjectID info.
    VOID MarkObjectsAsVisited();
    VOID CheckMarkedObjects(PrfObjectInfo *pCursor);
    PrfFunctionInfo * UpdateFunctionInfo(FunctionID funcId);
};
