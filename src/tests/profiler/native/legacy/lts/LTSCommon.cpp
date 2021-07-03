// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
//
// ProfilerCommon.cpp
//
// Implements LTSCommon, the instantiation of the IPrfCom interface.
//
// ======================================================================================

#include "LTSCommon.h"

// Modified print macros for use in LTSCommon.cpp
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

#undef  PINFO
#define PINFO m_pInfo

LTSCommon::LTSCommon(ICorProfilerInfo3 * pInfo)
            :PrfCommon(pInfo)
{
    return;
}

HRESULT LTSCommon::InspectObjectID(
                        ObjectID objectId,
                        ClassID classId,
                        mdTypeDef corTypeDef[],
                        const BOOL fullInspection)
{
    HRESULT hr = S_OK;
    ClassID objectClassId = NULL;

    // Where is NULL an ok ObjectID?
    if (objectId == 0)
    {
        return E_FAIL;
    }

    MUST_PASS(PINFO->GetClassFromObject(objectId, &objectClassId));
    if (objectClassId == 0)
    {
        FAILURE(L"GetClassFromObject returned a NULL ClassID for ObjectID " << HEX(objectId));
    }
    if ((classId != 0) && (objectClassId != classId))
    {
        FAILURE(L"GetClassFromObject ClassID " << HEX(objectClassId) << L" for ObjectID " << HEX(objectId) << L" != ClassID " << HEX(classId));
    }

    ModuleID objectModuleId      = NULL;
    mdTypeDef objectTypeDefToken = NULL;
    ClassID objectParentClassId  = NULL;

    hr = PINFO->GetClassIDInfo2(objectClassId, &objectModuleId, &objectTypeDefToken, &objectParentClassId, NULL, NULL, NULL);
    if (FAILED(hr))
    {
        HRESULT hr2 = PINFO->IsArrayClass(objectClassId, NULL, NULL, NULL);
        if (SUCCEEDED(hr2))
        {
            // We have an array class.
            if (fullInspection == TRUE)
            {
                return InspectArray(objectId, objectClassId, corTypeDef);
            }
            else
            {
                return S_OK;
            }
        }
        else
        {
            FAILURE(L"GetClassIDInfo2 returned HR=" << HEX(hr) << L" for ClassID=" << HEX(objectClassId));
        }
    }

    if (corTypeDef != NULL && objectTypeDefToken == corTypeDef[ELEMENT_TYPE_STRING])
    {
        return InspectString(objectId);
    }

    MUST_PASS(InspectClass(objectId, objectClassId, corTypeDef, fullInspection));

    while (fullInspection == TRUE && objectParentClassId != 0)
    {
        ClassID newClassId = objectParentClassId;
        ModuleID newModuleId = NULL;

        MUST_PASS(PINFO->GetClassIDInfo2(newClassId, &newModuleId, NULL, &objectParentClassId, NULL, NULL, NULL));
        MUST_PASS(InspectClass(objectId, newClassId, corTypeDef, fullInspection));
    }

    return S_OK;
}


// helper function to get the destination pointer from an object taking into account if we're a static or
// not.
ObjectID LTSCommon::GetObjectIdToFollow(
                        ObjectID start,
                        PVOID staticOffset,
                        ULONG32 boxOffset,
                        COR_FIELD_OFFSET corFieldOffset[],
                        ULONG currentProfilerToken,
                        BOOL isFieldStatic)
{
    ObjectID result = NULL;

    if (isFieldStatic)
    {
        result = *(ObjectID*)staticOffset;
    }
    else
    {
        if(currentProfilerToken == INFINITE)
        {
            FAILURE(L"Unknown field offset");
        }

        result = *(ObjectID*)(start + corFieldOffset[currentProfilerToken].ulOffset + boxOffset);
    }
    if (result != 0)
    {
        //MUST_RETURN_VALUE(IsBadReadPtr((PVOID)(result), sizeof(ObjectID)), 0x00);
    }

    return result;
}

ObjectID LTSCommon::GetPrimitiveAddrToFollow(
                        ObjectID start,
                        PVOID staticOffset,
                        ULONG32 boxOffset,
                        COR_FIELD_OFFSET corFieldOffset[],
                        ULONG currentProfilerToken,
                        BOOL isFieldStatic)
{
    ObjectID result = NULL;

    if (isFieldStatic)
    {
        result = (ObjectID)staticOffset;
    }
    else
    {
        if(currentProfilerToken == INFINITE)
        {
            FAILURE(L"Unknown field offset");
        }

        result = (ObjectID)(start + corFieldOffset[currentProfilerToken].ulOffset + boxOffset);
    }
    if(result != 0)
    {
        //MUST_RETURN_VALUE(IsBadReadPtr((PVOID)(result), sizeof(ObjectID)), 0x00);
    }

    return result;
}

HRESULT LTSCommon::GetStaticFieldAddress(
                        IMetaDataImport * pIMDImport,
                        ClassID classId,
                        mdToken token,
                        DWORD fieldAttribs,
                        ThreadID * pThreadId,
                        ContextID * pContextId,
                        AppDomainID * pAppdomainId,
                        PVOID * ppData)
{
    HRESULT hr = S_OK;
    PVOID staticAddress = NULL;
    BOOL isLiteral = IsFdLiteral(fieldAttribs);
	PLATFORM_WCHAR staticType[STRING_LENGTH];
    COR_PRF_STATIC_TYPE fieldInfo = COR_PRF_FIELD_NOT_A_STATIC;
    if (!isLiteral)
    {
        MUST_PASS(PINFO->GetStaticFieldInfo(classId, token, &fieldInfo));
    }

    if (S_OK == pIMDImport->GetCustomAttributeByName(token, PROFILER_STRING("System.ThreadStaticAttribute"), NULL, NULL))
    {
        PLATFORM_WCHAR staticType[] = L"GetThreadStaticAddress";
        if (!isLiteral && !(fieldInfo & COR_PRF_FIELD_THREAD_STATIC))
        {
            FAILURE(L"MetaData and Profiler disagree on COR_PRF_FIELD_THREAD_STATIC\n");
        }
        if (*pThreadId == 0)
        {
            MUST_PASS(PINFO->GetCurrentThreadID(pThreadId));
        }
        hr = PINFO->GetThreadStaticAddress(classId, token, *pThreadId, &staticAddress);
    }
    else if (S_OK == pIMDImport->GetCustomAttributeByName(token, PROFILER_STRING("System.ContextStaticAttribute"), NULL, NULL))
    {
        PLATFORM_WCHAR staticType[] = L"GetContextStaticAddress";
        if (!isLiteral && !(fieldInfo & COR_PRF_FIELD_CONTEXT_STATIC))
        {
            FAILURE(L"MetaData and Profiler disagree on COR_PRF_FIELD_CONTEXT_STATIC\n");
        }
        if (*pThreadId == 0)
        {
            MUST_PASS(PINFO->GetCurrentThreadID(pThreadId));
        }
        if (*pContextId == 0)
        {
            MUST_PASS(PINFO->GetThreadContext(*pThreadId, pContextId));
        }
        hr = PINFO->GetContextStaticAddress(classId, token, *pContextId, &staticAddress);
    }
    else if (IsFdHasFieldRVA(fieldAttribs))
    {
        PLATFORM_WCHAR staticType[] = L"GetRVAStaticAddress";
        if (!isLiteral && !(fieldInfo & COR_PRF_FIELD_RVA_STATIC))
        {
            FAILURE(L"MetaData and Profiler disagree on COR_PRF_FIELD_RVA_STATIC\n");
        }
        hr = PINFO->GetRVAStaticAddress(classId, token, &staticAddress);
    }
    else
    {
        PLATFORM_WCHAR staticType[] = L"GetAppDomainStaticAddress";
        if (!isLiteral && !(fieldInfo & COR_PRF_FIELD_APP_DOMAIN_STATIC))
        {
            FAILURE(L"MetaData and Profiler disagree on COR_PRF_FIELD_APP_DOMAIN_STATIC\n");
        }
        if (*pThreadId == 0)
        {
            MUST_PASS(PINFO->GetCurrentThreadID(pThreadId));
        }
        if (*pAppdomainId == 0)
        {
            MUST_PASS(PINFO->GetThreadAppDomain(*pThreadId, pAppdomainId));
        }
        hr = PINFO->GetAppDomainStaticAddress(classId, token, *pAppdomainId, &staticAddress);
    }

    if (hr == CORPROF_E_DATAINCOMPLETE || hr == CORPROF_E_LITERALS_HAVE_NO_ADDRESS || hr == COR_E_ARGUMENT)
    {
        return hr;
    }
    else if (FAILED(hr))
    {
        FAILURE(staticType << L" returned HR=" << HEX(hr) << L"\n\nToken\t\t= " << HEX(token) << L"\nClassID\t\t= "
                << HEX(classId) << L"\nAttributes\t= " << HEX(fieldAttribs) << L"\nThreadID \t= " << HEX(*pThreadId)
                << L"\nContextID\t= " << HEX(*pContextId) << L"\nAppDomainID\t= " << HEX(*pAppdomainId));
        return hr;
    }
    else
    {
        *ppData = reinterpret_cast<PVOID>(staticAddress);
    }
    return S_OK;
}


HRESULT LTSCommon::InspectClass(
                        ObjectID objectId,
                        ClassID classId,
                        mdTypeDef corTypeDef[],
                        BOOL fullInspection)
{
    HRESULT hr = S_OK;

    WCHAR tokenName[256];
    ULONG nameLength = NULL;
    DWORD corElementType = NULL;
    PCCOR_SIGNATURE pvSig = NULL;
    ULONG cbSig = NULL;
    DWORD fieldAttributes = NULL;

    ModuleID classModuleId = NULL;
    MUST_PASS(PINFO->GetClassIDInfo2(classId, &classModuleId, NULL, NULL, NULL, NULL, NULL));

    // Grab the MetaData pointer.  We need to use this a lot.
    COMPtrHolder<IMetaDataImport> pIMDImport;
    MUST_PASS(PINFO->GetModuleMetaData(classModuleId, ofRead, IID_IMetaDataImport, (IUnknown **)&pIMDImport));

    // Get the ThreadID, AppDomainID, and ContextID to be used by static inspection functions
    ThreadID    threadId    = NULL;
    ContextID   contextId   = NULL;
    AppDomainID appDomainId = NULL;
    MUST_PASS(PINFO->GetCurrentThreadID(&threadId));
    MUST_PASS(PINFO->GetThreadContext(threadId, &contextId));
    MUST_PASS(PINFO->GetThreadAppDomain(NULL, &appDomainId));

    // Get the class layout to find where all non-static fields live
    COR_FIELD_OFFSET corFieldOffset[STRING_LENGTH];
    ULONG cfieldOffset  = NULL;
    ULONG ulClassSize   = NULL;
    ULONG32 ulBoxOffset = NULL;
    hr = PINFO->GetClassLayout(classId, corFieldOffset, STRING_LENGTH, &cfieldOffset, &ulClassSize);

    if (hr == CORPROF_E_DATAINCOMPLETE)
    {
        return S_OK;
    }

    if (SUCCEEDED(hr))
    {
        if (cfieldOffset>0)
            for (unsigned int i=0; i < cfieldOffset; ++i)
            {
                if (corFieldOffset[i].ulOffset >= ulClassSize )
                {
                    FAILURE(L"Class "<< classId <<L" field "<< i << L" has a offset "<< corFieldOffset[cfieldOffset-1].ulOffset << L" greater than the class size "<< ulClassSize);
                }
            }
    }

    hr = PINFO->GetBoxClassLayout(classId, &ulBoxOffset);
    if (FAILED(hr))
    {
        ulBoxOffset = 0;
    }

    // Get class token to enum all fields from MetaData.  (Needed for statics)
    mdFieldDef fieldTokens[STRING_LENGTH];
    HCORENUM hEnum  = NULL;
    mdTypeDef token = NULL;
    ULONG cTokens   = NULL;
    hr = PINFO->GetClassIDInfo2(classId, NULL, &token, NULL, NULL, NULL, NULL);
    if (hr == CORPROF_E_DATAINCOMPLETE)
    {
        // Class load not complete.  We can not inspect yet.
        return S_OK;
    }
    if (FAILED(hr))
    {
        FAILURE(L"GetClassIDInfo2returned HR=" << HEX(hr));
    }

    // Enum all fields of the class from the MetaData
    MUST_PASS(pIMDImport->EnumFields(&hEnum, token, fieldTokens, STRING_LENGTH, &cTokens));

    // Sanity check
    if (cTokens < cfieldOffset)
    {
        FAILURE(L"MetaData API returned fewer fields than the Profiler API\n");
    }

    // Remember if the class should have statics
    BOOL classHasStatics = FALSE;
    if (cTokens > cfieldOffset)
    {
        classHasStatics = TRUE;
    }

    // Loop through each reported MetaData token.
    for (ULONG i = 0; i < cTokens; i++)
    {
        tokenName[0] = NULL;
        PVOID staticOffSet = NULL;

        // Make sure each MetaData token is reported by the
        ULONG currentProfilerToken = 0;
        while((currentProfilerToken < cfieldOffset) &&
              (corFieldOffset[currentProfilerToken].ridOfField != fieldTokens[i]))
        {
            currentProfilerToken++;
        }

        if (currentProfilerToken == cfieldOffset)
        {
            currentProfilerToken = INFINITE;
            if(!classHasStatics)
            {
                FAILURE(L"Token missing from profiler in class that doesn't have statics. " << fieldTokens[i]);
            }
        }

        ClassID fieldClassId = NULL;
        mdTypeDef fieldClassToken;

        MUST_PASS(pIMDImport->GetFieldProps(fieldTokens[i],    // The field for which to get props.
                                            &fieldClassToken,  // Put field's class here.
                                            tokenName,         // Put field's name here.
                                            256,               // Size of szField buffer in wide chars.
                                            &nameLength,       // Put actual size here
                                            &fieldAttributes,  // Put flags here.
                                            &pvSig,            // [OUT] point to the blob value of meta data
                                            &cbSig,            // [OUT] actual size of signature blob
                                            &corElementType,   // [OUT] flag for value type. selected ELEMENT_TYPE_*
                                            NULL,              // [OUT] constant value
                                            NULL));            // [OUT] size of constant value, string only, wide chars

        hr = PINFO->GetClassFromToken(classModuleId, fieldClassToken, &fieldClassId);
        if(FAILED(hr)
            && hr != CORPROF_E_TYPE_IS_PARAMETERIZED
            // Since some FCALLs were moved to managed code we can see object allocation
            // before the base system classes are finished loading.
            && hr != CORPROF_E_RUNTIME_UNINITIALIZED)
        {
            FAILURE(L"GetClassFromToken returned HR="<< HEX(hr) << L" for Field " << HEX(fieldClassToken));
        }

        if(IsFdLiteral(fieldAttributes))
        {
            // TODO: Pull the const value out of MetaData.
            continue;
        }

        // Remember if this field is static.  Inspection changes for instance/static fields
        BOOL isFieldStatic = FALSE;
        if(IsFdStatic(fieldAttributes))
        {
            isFieldStatic = TRUE;
            if (classHasStatics == FALSE)
            {
                FAILURE(L"MetaData and Profiler implied statics, but they do not exist.\n");
            }

            hr = GetStaticFieldAddress(pIMDImport, classId, fieldTokens[i], fieldAttributes, &threadId, &contextId, &appDomainId, &staticOffSet);
            if (hr == CORPROF_E_DATAINCOMPLETE)
            {
                // not a failure, but we cannot inspect a class if it is not fully loaded.  Continue on
                continue;
            }
            if (staticOffSet == NULL)
            {
                // VSWhidbey 525762 will not be fixed in Whidbey.  So profiler writers must be aware that static
                // address methods will return S_OK will a null static offset.
                continue;
            }
        }

        // Find the CorElementType of the current field for inspection.
        ULONG b = NULL;
        b = CorSigUncompressData(pvSig);
        corElementType = CorSigUncompressElementType(pvSig);

        // The field we are looking at is a generic type parameter
        if ((corElementType == ELEMENT_TYPE_VAR) && (corTypeDef != NULL))
        {
            b = CorSigUncompressData(pvSig);
            ULONG32 cTypeArgs = NULL;
            ClassID typeArgs[SHORT_LENGTH];

            // Get the type args of the class we are inspecting.
            MUST_PASS(PINFO->GetClassIDInfo2(classId, NULL, NULL, NULL, SHORT_LENGTH, &cTypeArgs, typeArgs));

            mdTypeDef fieldToken = NULL;
            ModuleID fieldModuleId = NULL;

            // Now figure out which type arg we are dealing with
            MUST_PASS(PINFO->GetClassIDInfo2(typeArgs[b], &fieldModuleId, &fieldToken, NULL, SHORT_LENGTH, &cTypeArgs, typeArgs));

            BOOL found = FALSE;

            // Check if token is one of our known Primitives ...
            for (int j = ELEMENT_TYPE_BOOLEAN; j <= ELEMENT_TYPE_R8; j++)
            {
                if(corTypeDef[j] == fieldToken)
                {
                    found = TRUE;
                    corElementType = static_cast<CorElementType>(j);
                }
            }

            // ... or a string ...
            if (found == FALSE && corTypeDef[ELEMENT_TYPE_STRING] == fieldToken)
            {
                found = TRUE;
                corElementType =  ELEMENT_TYPE_STRING;
            }

            // ... or a class.  Let's dig a little further ...
            if (found == FALSE)
            {
                corElementType = ELEMENT_TYPE_CLASS;
            }

        }
        else if(corElementType == ELEMENT_TYPE_CMOD_REQD || corElementType == ELEMENT_TYPE_CMOD_OPT)
        {
            mdToken token;
            pvSig += CorSigUncompressToken(pvSig, &token);

            //TODO: Inspect token
            continue;
        }

        // Now that we know what it is, lets inspect it.
        if (corElementType > ELEMENT_TYPE_VOID && corElementType < ELEMENT_TYPE_STRING)
        {
            MUST_PASS(InspectPrimitive((PVOID)GetPrimitiveAddrToFollow(objectId, staticOffSet, ulBoxOffset, corFieldOffset, currentProfilerToken, isFieldStatic),
                     static_cast<CorElementType>(corElementType)));
        }
        else if (corElementType == ELEMENT_TYPE_STRING)
        {
            MUST_PASS(InspectString(GetObjectIdToFollow(objectId, staticOffSet, ulBoxOffset, corFieldOffset, currentProfilerToken, isFieldStatic)));
        }
        else if(corElementType == ELEMENT_TYPE_I || corElementType == ELEMENT_TYPE_U)
        {
            // pointer or unsigned pointer, this is opaque to us.
        }
        else if(corElementType == ELEMENT_TYPE_PTR)
        {
            // TODO: We could walk this since have the type it points to.
        }
        else if(corElementType == ELEMENT_TYPE_VALUETYPE)
        {
            if (fullInspection == TRUE)
            {
               MUST_PASS(InspectClass(GetPrimitiveAddrToFollow(objectId, staticOffSet, ulBoxOffset, corFieldOffset, currentProfilerToken, isFieldStatic),
                                      fieldClassId,
                                      corTypeDef));
            }
        }
        else
        {
            ObjectID fieldObjectId = GetObjectIdToFollow(objectId, staticOffSet, ulBoxOffset, corFieldOffset, currentProfilerToken, isFieldStatic);

            if (fieldObjectId != 0)
            {
                fieldClassId = NULL;
                MUST_PASS(PINFO->GetClassFromObject(fieldObjectId, &fieldClassId));

                DISPLAY(L"\tField " << tokenName << L" CET(" << HEX(corElementType) << L" : " << b << L" is a class.  Digging recursing ...");

                if (fullInspection == TRUE)
                {
                    hr = PINFO->IsArrayClass(fieldClassId, NULL, NULL, NULL);
                    if(hr == S_FALSE)
                    {
                        MUST_PASS(InspectClass(fieldObjectId, fieldClassId, corTypeDef, fullInspection));
                    }
                    else
                    {
                        MUST_PASS(InspectArray(fieldObjectId, fieldClassId, corTypeDef));
                    }
                }
            }
        }
    }

    return S_OK;
}

HRESULT LTSCommon::InspectArrayElement(
                        BYTE *pData,
                        SIZE_T curIndex,
                        CorElementType baseType,
                        ClassID elementType,
                        mdTypeDef corTypeDef[])
{
    // helper macros for extracting our values from raw data.
#define EXTRACT_OBJECTID() static_cast<ObjectID>((reinterpret_cast<SIZE_T *>(pData))[curIndex])

    COR_FIELD_OFFSET corFieldOffset[SHORT_LENGTH];
    HRESULT   hr        = S_OK;
    ObjectID  curObjId  = NULL;
    ULONG     fieldCnt  = NULL;
    ULONG     classSize = NULL;
    ModuleID  module    = NULL;
    mdTypeDef typeToken = NULL;

    switch(baseType)
    {
        case ELEMENT_TYPE_VALUETYPE:
            // each value type will be one after another in the array
            MUST_PASS(PINFO->GetClassLayout(elementType, corFieldOffset, SHORT_LENGTH, &fieldCnt, &classSize));
            InspectClass(reinterpret_cast<ObjectID>(pData + curIndex*(classSize)), elementType, corTypeDef);
            break;

        case ELEMENT_TYPE_CLASS:
            // each object pointer will exist as an independent object.
            curObjId = EXTRACT_OBJECTID();
            if(curObjId != 0)
            {
                ClassID   objClass = NULL;
                ModuleID  moduleId = NULL;
                mdTypeDef typeDef  = NULL;

                MUST_PASS(PINFO->GetClassFromObject(curObjId, &objClass));
                InspectClass(curObjId, objClass, corTypeDef);
            }
            break;

        case ELEMENT_TYPE_STRING:
            curObjId = EXTRACT_OBJECTID();
            if(curObjId != 0)
            {
                hr = InspectString(curObjId);
            }
            break;

        case ELEMENT_TYPE_CHAR:
        case ELEMENT_TYPE_I2:
        case ELEMENT_TYPE_U2:
            InspectPrimitive(pData+curIndex*2,baseType);
            break;

        case ELEMENT_TYPE_I1:
        case ELEMENT_TYPE_U1:
            InspectPrimitive(pData+curIndex,baseType);
            break;

        case ELEMENT_TYPE_I4:
        case ELEMENT_TYPE_U4:
        case ELEMENT_TYPE_R4:
        case ELEMENT_TYPE_BOOLEAN:
            InspectPrimitive(pData+curIndex*4,baseType);
            break;

        case ELEMENT_TYPE_I8:
        case ELEMENT_TYPE_U8:
        case ELEMENT_TYPE_R8:
            InspectPrimitive(pData+curIndex*8,baseType);
            break;

        case ELEMENT_TYPE_ARRAY:
        case ELEMENT_TYPE_SZARRAY:
        case ELEMENT_TYPE_I:
        case ELEMENT_TYPE_U:
            break;

        default:
            FAILURE(L"Unknown type in InspectArrayElement. CET=" << HEX(baseType));
            break;
    }
    return S_OK;
}


HRESULT LTSCommon::InspectArrayRanks(
                        ULONG curRank,
                        ULONG maxRank,
                        ULONG32 dimensionSizes[],
                        int dimLowBounds[],
                        SIZE_T &curIndex,
                        BYTE *pData,
                        CorElementType baseType,
                        ClassID elementClass,
                        mdTypeDef corTypeDef[])
{
    if(curRank == maxRank - 1)
    {
        // walk the elements
        for(ULONG i = dimLowBounds[curRank]; i<dimensionSizes[curRank]; i++)
        {
            MUST_PASS(InspectArrayElement(pData, curIndex,  baseType, elementClass, corTypeDef));
            curIndex++;
        }
    }
    else
    {
        // walk the ranks
        MUST_PASS(InspectArrayRanks(curRank+1, maxRank, dimensionSizes, dimLowBounds, curIndex, pData, baseType, elementClass, corTypeDef));
    }

    return S_OK;
}


HRESULT LTSCommon::InspectArray(
                        ObjectID objectId,
                        ClassID classId,
                        mdTypeDef corTypeDef[])
{
    ULONG32 pDimensionSizes[256];
    INT pDimensionLowerBounds[256];
    CorElementType baseType;
    ULONG rank;
    ClassID baseTypeId;
    BYTE *pData;
    HRESULT hr;

    // first get the rank of the array..
    hr = PINFO->IsArrayClass(classId, &baseType, &baseTypeId, &rank);
    if(FAILED(hr))
    {
        FAILURE(L"InspectArray called w/ non-array class hr=" << HEX(hr) << L" for ObjectID " << HEX(classId));
        return hr;
    }

    // make sure we don't overflow our provided buffer.
    if (rank > 256)
    {
        //Not enough storage.  Return for now.
        return S_OK;
    }
    else
    {
        pDimensionSizes[rank] = 0xbaadf00d;
        pDimensionLowerBounds[rank] = 0xbaadf00d;
    }

    MUST_PASS(PINFO->GetArrayObjectInfo(objectId, rank, pDimensionSizes, pDimensionLowerBounds, &pData));
    if(pDimensionSizes[rank] != 0xbaadf00d)
    {
        FAILURE(L"pDimensionSizes buffer overflowed w/ value " << pDimensionSizes[rank]);
    }
    if(pDimensionLowerBounds[rank] != 0xbaadf00d)
    {
        FAILURE(L"pDimensionLowerBounds buffer overflowed w/ value " << pDimensionLowerBounds[rank]);
    }

    // attempt to inspect the individual objects in the array.
    SIZE_T curIndex = 0;
    MUST_PASS(InspectArrayRanks(0, rank, pDimensionSizes, pDimensionLowerBounds, curIndex, pData, baseType, baseTypeId, corTypeDef));

    return S_OK;
}


HRESULT LTSCommon::InspectPrimitive(
                        PVOID offSet,
                        CorElementType cet)
{
	/*
    if (IsBadReadPtr((PVOID)(offSet), sizeof(INT)) != 0)
    {
        FAILURE(L"IsBadReadPtr failed for offset " << offSet);
        return E_FAIL;
    }
	*/

    // Helper macros for extracting our values from raw data.
    // TODO: Properly sign extend values for the INT type.
    #define EXTRACT_INT(type)    (int)         ((type*)offSet)[0]
    #define EXTRACT_UINT(type)   (unsigned int)((type*)offSet)[0]
    #define EXTRACT_FLOAT(type)  (float)       ((type*)offSet)[0]
    #define EXTRACT_DOUBLE(type) (double)      ((type*)offSet)[0]

    switch(cet)
    {
        case ELEMENT_TYPE_VOID:                             break;
        case ELEMENT_TYPE_BOOLEAN:                          break;
        case ELEMENT_TYPE_CHAR:    EXTRACT_INT(WCHAR);      break;
        case ELEMENT_TYPE_I1:      EXTRACT_INT(BYTE);       break;
        case ELEMENT_TYPE_U1:      EXTRACT_UINT(BYTE);      break;
        case ELEMENT_TYPE_I2:      EXTRACT_INT(WORD);       break;
        case ELEMENT_TYPE_U2:      EXTRACT_UINT(WORD);      break;
        case ELEMENT_TYPE_I4:      EXTRACT_INT(DWORD);      break;
        case ELEMENT_TYPE_U4:      EXTRACT_UINT(DWORD);     break;
        case ELEMENT_TYPE_I8:      EXTRACT_INT(DWORD64);    break;
        case ELEMENT_TYPE_U8:      EXTRACT_UINT(DWORD64);   break;
        case ELEMENT_TYPE_R4:      EXTRACT_FLOAT(DWORD);    break;
        case ELEMENT_TYPE_R8:      EXTRACT_DOUBLE(DWORD64); break;
        default:
            FAILURE(L"Unknown primitive type " << HEX(cet));
            break;
    }
    return S_OK;
}


HRESULT LTSCommon::InspectString(
                        ObjectID objectId)
{
    if(objectId == 0)
    {
        // Null String
        return S_OK;
    }

    ULONG bufLenOfs = NULL;
    ULONG strLenOfs = NULL;
    ULONG bufOfs    = NULL;
    MUST_PASS(PINFO->GetStringLayout(&bufLenOfs, &strLenOfs, &bufOfs));

    DWORD buflen      = *reinterpret_cast<DWORD *>((reinterpret_cast<BYTE *>(objectId) + bufLenOfs));
    DWORD strlen      = *reinterpret_cast<DWORD *>((reinterpret_cast<BYTE *>(objectId) + strLenOfs));
    WCHAR * strBuffer =  reinterpret_cast<WCHAR *>((reinterpret_cast<BYTE *>(objectId) + bufOfs));

    // sanity check
    if(buflen < strlen)
    {
        FAILURE(L"Buffer length (" << buflen << L") < strlen (" << strlen << L")");
    }

    return S_OK;
}

bool IsMscorlib(const PLATFORM_WCHAR *wszDLL)
{
    return (std::wcsstr(wszDLL, L"mscorlib") != NULL)
            || (std::wcsstr(wszDLL, L"mscorlib.ni") != NULL)
            || (std::wcsstr(wszDLL, L"System.Private.CoreLib") != NULL)
            || (std::wcsstr(wszDLL, L"System.Private.CoreLib.ni") != NULL);
}