// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// ProfilerCommon.cpp
//
// Implements PrfCommon, the instantiation of the IPrfCom interface.
// 
// ======================================================================================

#ifndef __LOADER_TYPE_SYSTEM_COMMON__
#define __LOADER_TYPE_SYSTEM_COMMON__

#include "../ProfilerCommon.h"
#include <thread>

#define PLTSCOM pILTSCom

// Typedefs to keep track of Profiler ID's and related info
typedef map<ClassID, bool> ClassInspectMap;
typedef pair<ObjectID, bool> InspectedPair;


class DECLSPEC_NOVTABLE ILTSCom
{
public:
    
    // Inspect any ObjectID
    virtual HRESULT InspectObjectID(
                        ObjectID objectId, 
                        ClassID classId = NULL, 
                        mdTypeDef corTypeDef[] = NULL, 
                        const BOOL fullInspection = TRUE) = 0;

    virtual HRESULT InspectClass(
                        ObjectID objectId,
                        ClassID classId,
                        mdTypeDef corTypeDef[] = NULL,
                        BOOL fullInspection = TRUE) = 0;

    virtual HRESULT InspectString(
                        ObjectID objectId) = 0;

    virtual HRESULT InspectPrimitive(
                        PVOID offSet,
                        CorElementType cet) = 0;

    virtual HRESULT InspectArray(
                        ObjectID objectId,
                        ClassID classId,
                        mdTypeDef corTypeDef[] = NULL) = 0;

    virtual HRESULT GetStaticFieldAddress(
                        IMetaDataImport * pIMDImport,
                        ClassID classId,
                        mdToken token,
                        DWORD fieldAttribs,
                        ThreadID * pThreadId,
                        ContextID * pContextId,
                        AppDomainID * pAppdomainId,
                        PVOID * ppData) = 0;
};


class LTSCommon : public ILTSCom,
                  public PrfCommon
{
public:
    LTSCommon(ICorProfilerInfo3 * pInfo);
    ~LTSCommon() = default;

    virtual HRESULT InspectObjectID(
                        ObjectID objectId, 
                        ClassID classId = NULL, 
                        mdTypeDef corTypeDef[] = NULL, 
                        const BOOL fullInspection = TRUE);

    virtual HRESULT InspectClass(
                        ObjectID objectId,
                        ClassID classId,
                        mdTypeDef corTypeDef[] = NULL,
                        BOOL fullInspection = TRUE);
    
    virtual HRESULT InspectString(
                        ObjectID objectId);

    virtual HRESULT InspectPrimitive(
                        PVOID offSet,
                        CorElementType cet);

    virtual HRESULT InspectArray(
                        ObjectID objectId,
                        ClassID classId,
                        mdTypeDef corTypeDef[] = NULL);

    virtual HRESULT GetStaticFieldAddress(
                        IMetaDataImport * pIMDImport,
                        ClassID classId,
                        mdToken token,
                        DWORD fieldAttribs,
                        ThreadID * pThreadId,
                        ContextID * pContextId,
                        AppDomainID * pAppdomainId,
                        PVOID * ppData);

private:
    // Private Inscpection Routines
    HRESULT InspectArrayElement(
                        BYTE *pData, 
                        SIZE_T curIndex, 
                        CorElementType baseType, 
                        ClassID elementClass, 
                        mdTypeDef corTypeDef[] = NULL);

    HRESULT InspectArrayRanks(
                        ULONG curRank, 
                        ULONG maxRank, 
                        ULONG32 dimensionSizes[], 
                        int dimLowBounds[], 
                        SIZE_T &curIndex, 
                        BYTE *pData, 
                        CorElementType baseType, 
                        ClassID elementClass, 
                        mdTypeDef corTypeDef[] = NULL);

    ObjectID GetObjectIdToFollow(
                        ObjectID start, 
                        PVOID staticOffset, 
                        ULONG32 boxOffset, 
                        COR_FIELD_OFFSET corFieldOffset[], 
                        ULONG currentProfilerToken, 
                        BOOL isFieldStatic);

    ObjectID GetPrimitiveAddrToFollow(
                        ObjectID start, 
                        PVOID staticOffset, 
                        ULONG32 boxOffset, 
                        COR_FIELD_OFFSET corFieldOffset[], 
                        ULONG currentProfilerToken, 
                        BOOL isFieldStatic);
};

bool IsMscorlib(const PLATFORM_WCHAR *str);

#endif // __LOADER_TYPE_SYSTEM_COMMON__