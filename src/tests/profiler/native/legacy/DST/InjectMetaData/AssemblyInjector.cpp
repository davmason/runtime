#include "stdafx.h"

/* This code transfers metadata and code from a source assembly to a target assembly. The source assembly must be loaded enough to provide IMetaDataImport and a memory mapped image, but it need not be executable.
   The target assembly should be loaded within the runtime and ready for modification by the profiler APIs. The algorithm starts in InjectAll() using all the TypeDefs and CAs as a set of roots for graph exploration.
   From the types we branch out to methods, fields, base types, code bodies, signatures and tokens in that code, etc. As we traverse the metadata amd signatures we cache the translated results both for better
   performance and to eliminate infinite recursion. It is possible that the assembly has content in it that is never reached by the graph traversal in which case it won't injected. The content may have been
   superfluous for execution purposes, or if it does matter then we should adjust the set of roots in the graph traversal to include it.
   */

using namespace std;

AssemblyInjector::AssemblyInjector(IPrfCom* pPrfCom, ModuleID sourceModuleID, ModuleID targetModuleID, IMetaDataImport2* pSourceImport, IMetaDataImport2* pTargetImport, IMetaDataEmit2* pTargetEmit, IMethodMalloc* pTargetMethodMalloc) :
m_pSourceImport(pSourceImport),
m_pTargetImport(pTargetImport),
m_pTargetEmit(pTargetEmit),
m_pTargetMethodMalloc(pTargetMethodMalloc),
m_sourceModuleID(sourceModuleID),
m_targetModuleID(targetModuleID),
m_pPrfCom(pPrfCom)
{
}

HRESULT AssemblyInjector::InjectAll()
{
    TEST_DISPLAY(L"In AssemblyInjector::InjectAll");

    HCORENUM hEnum = NULL;
    mdTypeDef typeDef;
    ULONG cTokens;
    HRESULT hr;
    while (S_OK == (hr = m_pSourceImport->EnumTypeDefs(&hEnum, &typeDef, 1, &cTokens)))
    {
        mdToken targetTypeDef;
        InjectTypeDef(typeDef, &targetTypeDef);
    }
    IfFailRet(hr);

    hEnum = NULL;
    mdCustomAttribute curSourceCustomAttribute;
    while (S_OK == (hr = m_pSourceImport->EnumCustomAttributes(&hEnum, 0, NULL, &curSourceCustomAttribute, 1, &cTokens)))
    {
        mdCustomAttribute targetCustomAttribute;
        IfFailRet(InjectCustomAttribute(curSourceCustomAttribute, &targetCustomAttribute));
    }
    m_pSourceImport->CloseEnum(hEnum);
    IfFailRet(hr);

    return S_OK;
}

HRESULT AssemblyInjector::InjectTypes(const vector<const PLATFORM_WCHAR*> &targetTypes)
{
    TEST_DISPLAY(L"In AssemblyInjector::InjectTypes");

    HCORENUM hEnum = NULL;
    mdTypeDef typeDef;
    ULONG cTokens;
    HRESULT hr;
    while (S_OK == (hr = m_pSourceImport->EnumTypeDefs(&hEnum, &typeDef, 1, &cTokens)))
    {
        mdToken targetTypeDef;

        PROFILER_WCHAR szTypeDef[LONG_LENGTH];
        PLATFORM_WCHAR szTypeDefPlatform[LONG_LENGTH];
        ULONG cchTypeDef = 0;
        DWORD dwTypeDefFlags = 0;
        mdToken tkExtends = mdTokenNil;
        IfFailRet(m_pSourceImport->GetTypeDefProps(typeDef, szTypeDef, LONG_LENGTH, &cchTypeDef, &dwTypeDefFlags, &tkExtends));
        
        ConvertProfilerWCharToPlatformWChar(szTypeDefPlatform, LONG_LENGTH, szTypeDef, LONG_LENGTH);
        for (auto it = targetTypes.begin(); it != targetTypes.end(); ++it) 
        {
            if (wcscmp(*it, szTypeDefPlatform) == 0)
            { 
                InjectTypeDef(typeDef, &targetTypeDef);
            }
        }
    }
   
    m_pSourceImport->CloseEnum(hEnum);
    IfFailRet(hr);

    return S_OK;
}

HRESULT AssemblyInjector::InjectTypeDef(mdTypeDef sourceTypeDef, mdToken *pTargetTypeDef)
{
    if (sourceTypeDef == mdTokenNil || sourceTypeDef == mdTypeDefNil)
    {
        *pTargetTypeDef = sourceTypeDef;
        return S_OK;
    }

    TokenMap::iterator itr = m_typeDefMap.find(sourceTypeDef);
    if (itr != m_typeDefMap.end())
    {
        *pTargetTypeDef = itr->second;
        return S_OK;
    }

    WCHAR szTypeDef[1000];
    ULONG cchTypeDef = 0;
    DWORD dwTypeDefFlags = 0;
    mdToken tkExtends = mdTokenNil;
    IfFailRet(m_pSourceImport->GetTypeDefProps(sourceTypeDef, szTypeDef, 1000, &cchTypeDef, &dwTypeDefFlags, &tkExtends));

    mdToken targetExtends;
    IfFailRet(ConvertToken(tkExtends, &targetExtends));

    HCORENUM hEnum = NULL;
    IfFailRet(m_pSourceImport->EnumInterfaceImpls(&hEnum, sourceTypeDef, NULL, 0, NULL));
    ULONG ulCount = 0;
    IfFailRet(m_pSourceImport->CountEnum(hEnum, &ulCount));
    NewArrayHolder<mdInterfaceImpl> pInterfaceImpls;
    NewArrayHolder<mdToken> pImplements;
    if (ulCount != 0)
    {
        pInterfaceImpls = new mdInterfaceImpl[ulCount];
        IfFailRet(m_pSourceImport->EnumInterfaceImpls(&hEnum, sourceTypeDef, pInterfaceImpls, ulCount, NULL));
        pImplements = new mdToken[ulCount+1];
        for (unsigned int i = 0; i < ulCount; i++)
        {
            IfFailRet(m_pSourceImport->GetInterfaceImplProps(pInterfaceImpls[i], NULL, &(pImplements[i])));
            IfFailRet(ConvertToken(pImplements[i], &(pImplements[i])));
            DISPLAY(L"implements " << HEX(pImplements[i]));
        }
        pImplements[ulCount] = mdTokenNil;
    }
    m_pSourceImport->CloseEnum(hEnum);

    if (!IsTdNested(dwTypeDefFlags))
    {
        IfFailRet(m_pTargetEmit->DefineTypeDef(szTypeDef,
            dwTypeDefFlags,
            targetExtends,
            pImplements,
            pTargetTypeDef));
    }
    else
    {
        mdTypeDef sourceEnclosingTypeDef = mdTokenNil;
        mdTypeDef targetEnclosingTypeDef = mdTokenNil;
        IfFailRet(m_pSourceImport->GetNestedClassProps(sourceTypeDef, &sourceEnclosingTypeDef));
        IfFailRet(ConvertToken(sourceEnclosingTypeDef, &targetEnclosingTypeDef));
        IfFailRet(m_pTargetEmit->DefineNestedType(szTypeDef,
            dwTypeDefFlags,
            targetExtends,
            pImplements,
            targetEnclosingTypeDef,
            pTargetTypeDef));
    }


    m_typeDefMap[sourceTypeDef] = *pTargetTypeDef;
    DISPLAY(L"Injecting TypeDef " << szTypeDef << L"[" << HEX(*pTargetTypeDef) << L"] : [" << HEX(targetExtends) << L"]");


    hEnum = NULL;
    mdFieldDef curSourceField;
    ULONG cTokens;
    HRESULT hr;
    while (S_OK == (hr = m_pSourceImport->EnumFields(&hEnum, sourceTypeDef, &curSourceField, 1, &cTokens)))
    {
        mdFieldDef targetFieldDef;
        IfFailRet(InjectFieldDef(curSourceField, &targetFieldDef));
    }
    m_pSourceImport->CloseEnum(hEnum);
    IfFailRet(hr);

    hEnum = NULL;
    mdMethodDef curSourceMethod;
    while (S_OK == (hr = m_pSourceImport->EnumMethods(&hEnum, sourceTypeDef, &curSourceMethod, 1, &cTokens)))
    {
        mdMethodDef targetMethodDef;
        IfFailRet(InjectMethodDef(curSourceMethod, &targetMethodDef));
    }
    m_pSourceImport->CloseEnum(hEnum);
    IfFailRet(hr);

    hEnum = NULL;
    mdProperty curSourceProperty;
    while (S_OK == (hr = m_pSourceImport->EnumProperties(&hEnum, sourceTypeDef, &curSourceProperty, 1, &cTokens)))
    {
        mdProperty targetProperty;
        IfFailRet(InjectProperty(curSourceProperty, &targetProperty));
    }
    m_pSourceImport->CloseEnum(hEnum);
    IfFailRet(hr);

    hEnum = NULL;
    mdEvent curSourceEvent;
    while (S_OK == (hr = m_pSourceImport->EnumEvents(&hEnum, sourceTypeDef, &curSourceEvent, 1, &cTokens)))
    {
        mdEvent targetEvent;
        IfFailRet(InjectEvent(curSourceEvent, &targetEvent));
    }
    m_pSourceImport->CloseEnum(hEnum);
    IfFailRet(hr);

    hEnum = NULL;
    mdToken sourceImplementationMethod = mdTokenNil;
    mdToken sourceDeclarationMethod = mdTokenNil;
    while (S_OK == (hr = m_pSourceImport->EnumMethodImpls(&hEnum, sourceTypeDef, &sourceImplementationMethod, &sourceDeclarationMethod, 1, &cTokens)))
    {
        IfFailRet(InjectMethodImpl(sourceTypeDef, sourceImplementationMethod, sourceDeclarationMethod));
    }
    m_pSourceImport->CloseEnum(hEnum);
    IfFailRet(hr);

    if ((dwTypeDefFlags & tdExplicitLayout) != 0)
    {
        ULONG cFieldOffset = 0;
        IfFailRet(m_pSourceImport->GetClassLayout(sourceTypeDef, NULL, NULL, 0, &cFieldOffset, NULL));
        NewArrayHolder<COR_FIELD_OFFSET> fieldOffsets; 
        fieldOffsets = new COR_FIELD_OFFSET[cFieldOffset + 1];

        DWORD dwPackSize = 0;
        ULONG ulClassSize = 0;
        IfFailRet(m_pSourceImport->GetClassLayout(sourceTypeDef, &dwPackSize, fieldOffsets, cFieldOffset, NULL, &ulClassSize));
        for (unsigned int i = 0; i < cFieldOffset; i++)
        {
            IfFailRet(InjectFieldDef(fieldOffsets[i].ridOfField, &(fieldOffsets[i].ridOfField)));
        }
        fieldOffsets[cFieldOffset].ridOfField = mdFieldDefNil;
        fieldOffsets[cFieldOffset].ulOffset = 0;
        IfFailRet(m_pTargetEmit->SetClassLayout(*pTargetTypeDef, dwPackSize, fieldOffsets, ulClassSize));
    }

    hEnum = NULL;
    mdGenericParam curSourceGenericParam = mdTokenNil;
    while (S_OK == (hr = m_pSourceImport->EnumGenericParams(&hEnum, sourceTypeDef, &curSourceGenericParam, 1, NULL)))
    {
        mdGenericParam targetGenericParam = mdTokenNil;
        IfFailRet(InjectGenericParam(curSourceGenericParam, &targetGenericParam));
    }
    m_pSourceImport->CloseEnum(hEnum);
    IfFailRet(hr);

    return S_OK;
}


HRESULT AssemblyInjector::InjectTypeRef(mdTypeRef sourceTypeRef, mdToken *pTargetTypeRef)
{
    if (sourceTypeRef == mdTokenNil || sourceTypeRef == mdTypeRefNil)
    {
        *pTargetTypeRef = sourceTypeRef;
        return S_OK;
    }

    TokenMap::iterator itr = m_typeRefMap.find(sourceTypeRef);
    if (itr != m_typeRefMap.end())
    {
        *pTargetTypeRef = itr->second;
        return S_OK;
    }

    mdToken tkResolutionScope;
    WCHAR szName[1000];
    ULONG cchName = 0;
    IfFailRet(m_pSourceImport->GetTypeRefProps(sourceTypeRef,
        &tkResolutionScope,
        szName,
        1000,
        &cchName));

    if (TypeFromToken(tkResolutionScope) == mdtModuleRef)
    {
        FAILURE(L"TypeRef in module scope NYI");
        return E_NOTIMPL;
    }

    mdToken targetAssemblyScope = 0;
    IfFailRet(ConvertToken(tkResolutionScope, &targetAssemblyScope));
    if ( (TypeFromToken(targetAssemblyScope) == mdtAssembly && TypeFromToken(tkResolutionScope) == mdtAssemblyRef) ||
         (TypeFromToken(targetAssemblyScope) == mdtTypeDef))
    {
        mdToken tkEnclosingClass = mdTokenNil;
        if (TypeFromToken(targetAssemblyScope) == mdtTypeDef)
        {
            tkEnclosingClass = targetAssemblyScope;
        }
        IfFailRet(m_pTargetImport->FindTypeDefByName(szName, tkEnclosingClass, pTargetTypeRef));
    }
    else
    {
        HRESULT hr = m_pTargetImport->FindTypeRef(targetAssemblyScope, szName, pTargetTypeRef);
        if (FAILED(hr) && hr != CLDB_E_RECORD_NOTFOUND)
        {
            return hr;
        }
        if (*pTargetTypeRef == mdTypeRefNil || *pTargetTypeRef == mdTokenNil)
        {
            IfFailRet(m_pTargetEmit->DefineTypeRefByName(targetAssemblyScope, szName, pTargetTypeRef));
            DISPLAY(L"Injecting TypeRef " << szName << L"[" << HEX(*pTargetTypeRef) << L"]");
        }
    }
    m_typeRefMap[sourceTypeRef] = *pTargetTypeRef;
    return S_OK;
}

HRESULT AssemblyInjector::InjectAssemblyRef(mdAssemblyRef sourceAssemblyRef, mdToken *pTargetAssemblyScope)
{
    if (sourceAssemblyRef == mdTokenNil || sourceAssemblyRef == mdAssemblyRefNil)
    {
        *pTargetAssemblyScope = sourceAssemblyRef;
        return S_OK;
    }

    TokenMap::iterator itr = m_assemblyRefMap.find(sourceAssemblyRef);
    if (itr != m_assemblyRefMap.end())
    {
        *pTargetAssemblyScope = itr->second;
        return S_OK;
    }

    COMPtrHolder<IMetaDataAssemblyImport> pAssemblyImport;
    IfFailRet(m_pSourceImport->QueryInterface(IID_IMetaDataAssemblyImport, (void**)&pAssemblyImport));

    COMPtrHolder<IMetaDataAssemblyImport> pTargetAssemblyImport;
    IfFailRet(m_pTargetImport->QueryInterface(IID_IMetaDataAssemblyImport, (void**)&pTargetAssemblyImport));

    const void* pbPublicKeyOrToken = NULL;
    ULONG cbPublicKeyOrToken = 0;
    WCHAR szName[1000];
    ULONG cchName = 0;
    const void* pbHashValue;
    ASSEMBLYMETADATA metadata;
    memset(&metadata, 0, sizeof(metadata));
    ULONG cbHashValue;
    DWORD dwAssemblyRefFlags;
    IfFailRet(pAssemblyImport->GetAssemblyRefProps(sourceAssemblyRef,
        &pbPublicKeyOrToken,
        &cbPublicKeyOrToken,
        szName,
        1000,
        &cchName,
        &metadata,
        &pbHashValue,
        &cbHashValue,
        &dwAssemblyRefFlags));

    // Couldn't find the assembly ref, add it now
    COMPtrHolder<IMetaDataAssemblyEmit> pMetaDataAssemblyEmit;
    IfFailRet(m_pTargetEmit->QueryInterface(IID_IMetaDataAssemblyEmit, (void**)&pMetaDataAssemblyEmit));
    
    mdAssemblyRef newAssemblyRef;
    IfFailRet(pMetaDataAssemblyEmit->DefineAssemblyRef(
        pbPublicKeyOrToken,
        cbPublicKeyOrToken,
        szName,
        &metadata,
        pbHashValue,
        cbHashValue,
        dwAssemblyRefFlags,
        &newAssemblyRef
    ));

    *pTargetAssemblyScope = newAssemblyRef;
    m_assemblyRefMap[sourceAssemblyRef] = *pTargetAssemblyScope;
    return S_OK;
}

HRESULT AssemblyInjector::InjectFieldDef(mdFieldDef sourceFieldDef, mdFieldDef *pTargetFieldDef)
{
    if (sourceFieldDef == mdTokenNil || sourceFieldDef == mdFieldDefNil)
    {
        *pTargetFieldDef = sourceFieldDef;
        return S_OK;
    }

    TokenMap::iterator itr = m_fieldDefMap.find(sourceFieldDef);
    if (itr != m_fieldDefMap.end())
    {
        *pTargetFieldDef = itr->second;
        return S_OK;
    }

    mdTypeDef tkClass;
    WCHAR szName[1000];
    ULONG cchName = 0;
    DWORD dwAttr = 0;
    PCCOR_SIGNATURE pvSigBlob = NULL;
    ULONG cbSigBlob = 0;
    DWORD dwCPlusTypeFlag = 0;
    UVCP_CONSTANT pValue = NULL;
    ULONG cchValue = 0;
    IfFailRet(m_pSourceImport->GetFieldProps(sourceFieldDef,
        &tkClass,
        szName,
        1000,
        &cchName,
        &dwAttr,
        &pvSigBlob,
        &cbSigBlob,
        &dwCPlusTypeFlag,
        &pValue,
        &cchValue));

    mdToken targetTypeDef;
    IfFailRet(InjectTypeDef(tkClass, &targetTypeDef));
    IfFailRet(ConvertNonTypeSignatureCached(&pvSigBlob, &cbSigBlob));
    IfFailRet(m_pTargetEmit->DefineField(targetTypeDef,
        szName,
        dwAttr & ~(fdReservedMask&~(fdHasFieldRVA | fdRTSpecialName)),
        pvSigBlob,
        cbSigBlob,
        dwCPlusTypeFlag,
        pValue,
        cchValue,
        pTargetFieldDef));

    DISPLAY(L"Injecting FieldDef " << szName << L" [" << HEX(*pTargetFieldDef) << L"]");
    m_fieldDefMap[sourceFieldDef] = *pTargetFieldDef;

    if ((dwAttr & fdHasFieldMarshal) != 0)
    {
        PCCOR_SIGNATURE pvNativeType = NULL;
        ULONG cbNativeType = 0;
        IfFailRet(m_pSourceImport->GetFieldMarshal(sourceFieldDef, &pvNativeType, &cbNativeType));
        IfFailRet(m_pTargetEmit->SetFieldMarshal(*pTargetFieldDef, pvNativeType, cbNativeType));
    }

    return S_OK;
}

HRESULT AssemblyInjector::InjectMethodDef(mdMethodDef sourceMethodDef, mdMethodDef *pTargetMethodDef)
{
    if (sourceMethodDef == mdTokenNil || sourceMethodDef == mdMethodDefNil)
    {
        *pTargetMethodDef = sourceMethodDef;
        return S_OK;
    }

    TokenMap::iterator itr = m_methodDefMap.find(sourceMethodDef);
    if (itr != m_methodDefMap.end())
    {
        *pTargetMethodDef = itr->second;
        return S_OK;
    }

    mdTypeDef tkClass;
    WCHAR szName[1000];
    ULONG cchName = 0;
    DWORD dwAttr = 0;
    PCCOR_SIGNATURE pvSigBlob = NULL;
    ULONG cbSigBlob = 0;
    ULONG ulCodeRVA = 0;
    DWORD dwImplFlags = 0;
    IfFailRet(m_pSourceImport->GetMethodProps(sourceMethodDef,
        &tkClass,
        szName,
        1000,
        &cchName,
        &dwAttr,
        &pvSigBlob,
        &cbSigBlob,
        &ulCodeRVA,
        &dwImplFlags));

    mdToken targetTypeDef;
    IfFailRet(InjectTypeDef(tkClass, &targetTypeDef));
    IfFailRet(ConvertNonTypeSignatureCached(&pvSigBlob, &cbSigBlob));


    IfFailRet(m_pTargetEmit->DefineMethod(targetTypeDef,
        szName,
        dwAttr,
        pvSigBlob,
        cbSigBlob,
        0,
        dwImplFlags,
        pTargetMethodDef));

    m_methodDefMap[sourceMethodDef] = *pTargetMethodDef;

    HCORENUM hEnum = NULL;
    mdParamDef curSourceParam = mdTokenNil;
    HRESULT hr;
    while (S_OK == (hr = m_pSourceImport->EnumParams(&hEnum, sourceMethodDef, &curSourceParam, 1, NULL)))
    {
        mdParamDef curTargetParam = mdTokenNil;
        IfFailRet(InjectParam(curSourceParam, &curTargetParam));
    }
    m_pSourceImport->CloseEnum(hEnum);
    IfFailRet(hr);

    if ((dwAttr & mdPinvokeImpl) != 0)
    {
        DWORD dwMappingFlags;
        WCHAR szImportName[1000];
        ULONG cchImportName = 0;
        mdModuleRef sourceImportDll = mdTokenNil;
        IfFailRet(m_pSourceImport->GetPinvokeMap(sourceMethodDef,
            &dwMappingFlags,
            szImportName,
            1000,
            &cchImportName,
            &sourceImportDll));

        mdModuleRef targetImportDll;
        IfFailRet(InjectModuleRef(sourceImportDll, &targetImportDll));
        IfFailRet(m_pTargetEmit->DefinePinvokeMap(*pTargetMethodDef, dwMappingFlags, szImportName, targetImportDll));
    }

    hEnum = NULL;
    mdGenericParam curSourceGenericParam = mdTokenNil;
    while (S_OK == (hr = m_pSourceImport->EnumGenericParams(&hEnum, sourceMethodDef, &curSourceGenericParam, 1, NULL)))
    {
        mdGenericParam targetGenericParam = mdTokenNil;
        IfFailRet(InjectGenericParam(curSourceGenericParam, &targetGenericParam));
    }
    m_pSourceImport->CloseEnum(hEnum);
    IfFailRet(hr);

    if (ulCodeRVA != 0)
    {
        VOID* pSourceCode;
        m_pPrfCom->m_pInfo->GetILFunctionBody(m_sourceModuleID, sourceMethodDef, (LPCBYTE*)(&pSourceCode), NULL);
        IMAGE_COR_ILMETHOD_TINY* pSourceCodeTinyHeader = (IMAGE_COR_ILMETHOD_TINY*)pSourceCode;
        IMAGE_COR_ILMETHOD_FAT* pSourceCodeFatHeader = (IMAGE_COR_ILMETHOD_FAT*)pSourceCode;
        bool isTinyHeader = ((pSourceCodeTinyHeader->Flags_CodeSize & (CorILMethod_FormatMask >> 1)) == CorILMethod_TinyFormat);
        ULONG ilCodeSize = 0;
        ULONG headerSize = 0;
        ULONG ehClauseHeaderRVA = 0;
        IMAGE_COR_ILMETHOD_SECT_FAT* pFatEHHeader = NULL;
        IMAGE_COR_ILMETHOD_SECT_SMALL* pSmallEHHeader = NULL;
        ULONG totalCodeBlobSize = 0;
        if (isTinyHeader)
        {
            ilCodeSize = (((unsigned)pSourceCodeTinyHeader->Flags_CodeSize) >> (CorILMethod_FormatShift - 1));
            headerSize = sizeof(IMAGE_COR_ILMETHOD_TINY);
            totalCodeBlobSize = ilCodeSize + headerSize;
        }
        else
        {
            ilCodeSize = pSourceCodeFatHeader->CodeSize;
            headerSize = pSourceCodeFatHeader->Size * 4;
            if ((pSourceCodeFatHeader->Flags & CorILMethod_MoreSects) == 0)
            {
                totalCodeBlobSize = ilCodeSize + headerSize;
            }
            else
            {
                // EH section starts at the 4 byte aligned address after the code
                ehClauseHeaderRVA = ((ulCodeRVA + headerSize + ilCodeSize - 1) & ~3) + 4;
                VOID* pEHSectionHeader = (BYTE*)pSourceCode + (ehClauseHeaderRVA - ulCodeRVA);
                BYTE kind = *(BYTE*)pEHSectionHeader;
                ULONG dataSize = 0;
                if (kind & CorILMethod_Sect_FatFormat)
                {
                    pFatEHHeader = (IMAGE_COR_ILMETHOD_SECT_FAT*)pEHSectionHeader;
                    dataSize = pFatEHHeader->DataSize;
                }
                else
                {
                    pSmallEHHeader = (IMAGE_COR_ILMETHOD_SECT_SMALL*)pEHSectionHeader;
                    dataSize = pSmallEHHeader->DataSize;
                }
                // take the difference between the RVAs to ensure we account for the padding
                // bytes between the end of the IL code and the start of the EH clauses
                totalCodeBlobSize = (ehClauseHeaderRVA - ulCodeRVA) + dataSize;
            }
        }

        VOID* pTargetCode = m_pTargetMethodMalloc->Alloc(totalCodeBlobSize);
        IfNullRetOOM(pTargetCode);

        // convert header
        memcpy(pTargetCode, pSourceCode, headerSize);
        if (!isTinyHeader)
        {
            IMAGE_COR_ILMETHOD_FAT* pTargetCodeFatHeader = (IMAGE_COR_ILMETHOD_FAT*)pTargetCode;
            IfFailRet(InjectLocalVarSig(pTargetCodeFatHeader->LocalVarSigTok, &(pTargetCodeFatHeader->LocalVarSigTok)));
        }

        // convert IL code
        IfFailRet(ConvertILCode((BYTE*)pSourceCode + headerSize, (BYTE*)pTargetCode + headerSize, ilCodeSize));

        //convert EH
        if (pFatEHHeader != NULL)
        {
            IMAGE_COR_ILMETHOD_SECT_FAT* pTargetEHHeader = (IMAGE_COR_ILMETHOD_SECT_FAT*) ((BYTE*)pTargetCode + (ehClauseHeaderRVA - ulCodeRVA));
            pTargetEHHeader->Kind = pFatEHHeader->Kind;
            pTargetEHHeader->DataSize = pFatEHHeader->DataSize;
            IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT* pSourceEHClause = (IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT*)(pFatEHHeader + 1);
            IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT* pTargetEHClause = (IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT*)(pTargetEHHeader + 1);
            int numClauses = (pTargetEHHeader->DataSize - 4) / sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT);
            for (int i = 0; i < numClauses; i++)
            {
                pTargetEHClause->Flags = pSourceEHClause->Flags;
                pTargetEHClause->TryOffset = pSourceEHClause->TryOffset;
                pTargetEHClause->TryLength = pSourceEHClause->TryLength;
                pTargetEHClause->HandlerOffset = pSourceEHClause->HandlerOffset;
                pTargetEHClause->HandlerLength = pSourceEHClause->HandlerLength;
                if ((pSourceEHClause->Flags & COR_ILEXCEPTION_CLAUSE_FILTER) != 0)
                {
                    IfFailRet(ConvertToken(pSourceEHClause->ClassToken, (mdToken*)&(pTargetEHClause->ClassToken)));
                }
                else
                {
                    pTargetEHClause->FilterOffset = pSourceEHClause->FilterOffset;
                }
                pTargetEHClause++;
                pSourceEHClause++;
            }
        }
        else if (pSmallEHHeader != NULL)
        {
            IMAGE_COR_ILMETHOD_SECT_SMALL* pTargetEHHeader = (IMAGE_COR_ILMETHOD_SECT_SMALL*)((BYTE*)pTargetCode + (ehClauseHeaderRVA - ulCodeRVA));
            pTargetEHHeader->Kind = pSmallEHHeader->Kind;
            pTargetEHHeader->DataSize = pSmallEHHeader->DataSize;
            IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_SMALL* pSourceEHClause = (IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_SMALL*)(pSmallEHHeader + 1);
            IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_SMALL* pTargetEHClause = (IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_SMALL*)(pTargetEHHeader + 1);
            int numClauses = (pSmallEHHeader->DataSize - 4) / sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_SMALL);
            for (int i = 0; i < numClauses; i++)
            {
                pTargetEHClause->Flags = pSourceEHClause->Flags;
                pTargetEHClause->TryOffset = pSourceEHClause->TryOffset;
                pTargetEHClause->TryLength = pSourceEHClause->TryLength;
                pTargetEHClause->HandlerOffset = pSourceEHClause->HandlerOffset;
                pTargetEHClause->HandlerLength = pSourceEHClause->HandlerLength;
                if ((pSourceEHClause->Flags & COR_ILEXCEPTION_CLAUSE_FILTER) != 0)
                {
                    IfFailRet(ConvertToken(pSourceEHClause->ClassToken, (mdToken*)&(pTargetEHClause->ClassToken)));
                }
                else
                {
                    pTargetEHClause->FilterOffset = pSourceEHClause->FilterOffset;
                }
                pTargetEHClause++;
                pSourceEHClause++;
            }
        }

        //because the metadata record wasn't created with an RVA, this connects the methodDef to the code
        IfFailRet(m_pPrfCom->m_pInfo->SetILFunctionBody(m_targetModuleID, *pTargetMethodDef, (LPCBYTE)pTargetCode));
        DISPLAY(L"Injecting MethodDef " << szName << L" [" << HEX(*pTargetMethodDef) << L"] Code: " << HEX(pTargetCode));
    }
    else
    {
        DISPLAY(L"Injecting MethodDef " << szName << L" [" << HEX(*pTargetMethodDef) << L"] Code: 0");
    }

    return S_OK;
}

HRESULT AssemblyInjector::InjectLocalVarSig(mdSignature sourceLocalVarSig, mdSignature *pTargetLocalVarSig)
{
    if (sourceLocalVarSig == mdTokenNil || sourceLocalVarSig == mdSignatureNil)
    {
        *pTargetLocalVarSig = sourceLocalVarSig;
        return S_OK;
    }
    PCCOR_SIGNATURE pvSig = NULL;
    ULONG cbSig = 0;
    IfFailRet(m_pSourceImport->GetSigFromToken(sourceLocalVarSig, &pvSig, &cbSig));
    IfFailRet(ConvertNonTypeSignatureCached(&pvSig, &cbSig));
    IfFailRet(m_pTargetEmit->GetTokenFromSig(pvSig, cbSig, pTargetLocalVarSig));
    DISPLAY(L"Injecting LocalVarSig " << HEX(*pTargetLocalVarSig));
    return S_OK;
}

HRESULT AssemblyInjector::InjectMemberRef(mdSignature sourceMemberRef, mdSignature *pTargetMemberRef)
{
    if (sourceMemberRef == mdTokenNil || sourceMemberRef == mdMemberRefNil)
    {
        *pTargetMemberRef = sourceMemberRef;
        return S_OK;
    }

    TokenMap::iterator itr = m_memberRefMap.find(sourceMemberRef);
    if (itr != m_memberRefMap.end())
    {
        *pTargetMemberRef = itr->second;
        return S_OK;
    }

    mdToken declaringType = mdTokenNil;
    WCHAR szName[1000];
    ULONG cchName = 0;
    PCCOR_SIGNATURE pvSigBlob;
    ULONG cbSig = 0;
    IfFailRet(m_pSourceImport->GetMemberRefProps(sourceMemberRef,
        &declaringType,
        szName,
        1000,
        &cchName,
        &pvSigBlob,
        &cbSig));

    IfFailRet(ConvertNonTypeSignatureCached(&pvSigBlob, &cbSig));
    mdToken targetDeclaringType = mdTokenNil;
    IfFailRet(ConvertToken(declaringType, &targetDeclaringType));
    

    mdMemberRef existingTargetMemberRef = mdTokenNil;
    HRESULT hr = m_pTargetImport->FindMemberRef(targetDeclaringType, szName, pvSigBlob, cbSig, &existingTargetMemberRef);
    if (FAILED(hr) && hr != CLDB_E_RECORD_NOTFOUND)
    {
        return hr;
    }
    else if (SUCCEEDED(hr))
    {
        *pTargetMemberRef = existingTargetMemberRef;
        m_memberRefMap[sourceMemberRef] = *pTargetMemberRef;
        return S_OK;
    }
    else
    {
        // Some of the member refs we emit will be more verbose than is necessary. If the reference
        // resolves within the assembly we are injecting into then a member ref could be collapsed to
        // a FieldDef or MethodDef. At this point however compactness/canonicalization isn't a priority
        IfFailRet(m_pTargetEmit->DefineMemberRef(targetDeclaringType, szName, pvSigBlob, cbSig, pTargetMemberRef));
        DISPLAY(L"Injecting MemberRef " << HEX(*pTargetMemberRef));
        m_memberRefMap[sourceMemberRef] = *pTargetMemberRef;
        return S_OK;
    }
}

HRESULT AssemblyInjector::InjectString(mdString sourceString, mdString *pTargetString)
{
    if (sourceString == mdTokenNil || sourceString == mdStringNil)
    {
        *pTargetString = sourceString;
        return S_OK;
    }

    //check the cache
    TokenMap::iterator itr = m_stringMap.find(sourceString);
    if (itr != m_stringMap.end())
    {
        *pTargetString = itr->second;
        return S_OK;
    }

    ULONG cchString = 0;
    IfFailRet(m_pSourceImport->GetUserString(sourceString, NULL, 0, &cchString));
    NewArrayHolder<WCHAR> pStringBuffer;
    pStringBuffer = new WCHAR[cchString];
    IfFailRet(m_pSourceImport->GetUserString(sourceString, pStringBuffer, cchString, &cchString));

    // Some of the strings we emit may already exist in the target assembly. If we care more about
    // target assembly size we could search all existing strings first before adding a new one
    IfFailRet(m_pTargetEmit->DefineUserString(pStringBuffer, cchString, pTargetString));
    //DISPLAY(L"Injecting string " << (WCHAR*)pStringBuffer << " [" << HEX(*pTargetString) << L"]"); // TODO: This causes AV.
    m_stringMap[sourceString] = *pTargetString;
    return S_OK;
}

HRESULT AssemblyInjector::InjectProperty(mdProperty sourceProperty, mdProperty *pTargetProperty)
{
    if (sourceProperty == mdTokenNil || sourceProperty == mdPropertyNil)
    {
        *pTargetProperty = sourceProperty;
        return S_OK;
    }

    TokenMap::iterator itr = m_propertyMap.find(sourceProperty);
    if (itr != m_propertyMap.end())
    {
        *pTargetProperty = itr->second;
        return S_OK;
    }

    mdTypeDef sourceClass;
    WCHAR szName[1000];
    ULONG cchName = 0;
    DWORD dwPropFlags = 0;
    PCCOR_SIGNATURE pvSig = NULL;
    ULONG pbSig = 0;
    DWORD dwCPlusTypeFlag = 0;
    UVCP_CONSTANT pDefaultValue = NULL;
    ULONG cchDefaultValue = 0;
    mdMethodDef mdSourceSetter = mdTokenNil;
    mdMethodDef mdSourceGetter = mdTokenNil;
    ULONG cOtherMethod = 0;
    IfFailRet(m_pSourceImport->GetPropertyProps(sourceProperty,
        &sourceClass,
        szName,
        1000,
        &cchName,
        &dwPropFlags,
        &pvSig,
        &pbSig,
        &dwCPlusTypeFlag,
        &pDefaultValue,
        &cchDefaultValue,
        &mdSourceSetter,
        &mdSourceGetter,
        NULL,
        0,
        &cOtherMethod));
    if (cOtherMethod > 0)
    {
        FAILURE(L"Property with other associated methods NYI");
        return E_NOTIMPL;
    }

    IfFailRet(ConvertNonTypeSignatureCached(&pvSig, &pbSig));
    mdTypeDef targetClass;
    IfFailRet(InjectTypeDef(sourceClass, &targetClass));
    mdMethodDef mdTargetGetter = mdTokenNil;
    mdMethodDef mdTargetSetter = mdTokenNil;
    IfFailRet(InjectMethodDef(mdSourceGetter, &mdTargetGetter));
    IfFailRet(InjectMethodDef(mdSourceSetter, &mdTargetSetter));

    IfFailRet(m_pTargetEmit->DefineProperty(targetClass,
        szName,
        dwPropFlags,
        pvSig,
        pbSig,
        dwCPlusTypeFlag,
        pDefaultValue,
        cchDefaultValue,
        mdTargetGetter,
        mdTargetSetter,
        NULL,
        pTargetProperty));

    DISPLAY(L"Injecting Property " << szName << " [" << HEX(*pTargetProperty) << L"]");
    m_propertyMap[sourceProperty] = *pTargetProperty;
    return S_OK;
}

HRESULT AssemblyInjector::InjectEvent(mdEvent sourceEvent, mdEvent *pTargetEvent)
{
    if (sourceEvent == mdTokenNil || sourceEvent == mdEventNil)
    {
        *pTargetEvent = sourceEvent;
        return S_OK;
    }

    TokenMap::iterator itr = m_eventMap.find(sourceEvent);
    if (itr != m_eventMap.end())
    {
        *pTargetEvent = itr->second;
        return S_OK;
    }

    mdTypeDef sourceClass;
    WCHAR szName[1000];
    ULONG cchName = 0;
    DWORD dwEventFlags = 0;
    mdToken sourceEventType = 0;
    mdMethodDef mdSourceAddOn = mdTokenNil;
    mdMethodDef mdSourceRemoveOn = mdTokenNil;
    mdMethodDef mdSourceFire = mdTokenNil;
    ULONG cOtherMethod = 0;
    IfFailRet(m_pSourceImport->GetEventProps(sourceEvent,
        &sourceClass,
        szName,
        1000,
        &cchName,
        &dwEventFlags,
        &sourceEventType,
        &mdSourceAddOn,
        &mdSourceRemoveOn,
        &mdSourceFire,
        NULL,
        0,
        &cOtherMethod));
    if (cOtherMethod > 0)
    {
        FAILURE(L"Event with other associated methods NYI");
        return E_NOTIMPL;
    }

    
    mdTypeDef targetClass = mdTokenNil;
    IfFailRet(InjectTypeDef(sourceClass, &targetClass));
    mdToken targetEventType = mdTokenNil;
    IfFailRet(ConvertToken(sourceEventType, &targetEventType));
    mdMethodDef mdTargetAddOn = mdTokenNil;
    mdMethodDef mdTargetRemoveOn = mdTokenNil;
    mdMethodDef mdTargetFire = mdTokenNil;
    IfFailRet(InjectMethodDef(mdSourceAddOn, &mdTargetAddOn));
    IfFailRet(InjectMethodDef(mdSourceRemoveOn, &mdTargetRemoveOn));
    IfFailRet(InjectMethodDef(mdSourceFire, &mdTargetFire));
    

    IfFailRet(m_pTargetEmit->DefineEvent(targetClass,
        szName,
        dwEventFlags,
        targetEventType,
        mdTargetAddOn,
        mdTargetRemoveOn,
        mdTargetFire,
        NULL,
        pTargetEvent));

    DISPLAY(L"Injecting Event " << szName << " [" << HEX(*pTargetEvent) << L"]");
    m_eventMap[sourceEvent] = *pTargetEvent;
    return S_OK;
}

HRESULT AssemblyInjector::InjectCustomAttribute(mdCustomAttribute sourceCustomAttribute, mdCustomAttribute *pTargetCustomAttribute)
{
    if (sourceCustomAttribute == mdTokenNil || sourceCustomAttribute == mdCustomAttributeNil)
    {
        *pTargetCustomAttribute = sourceCustomAttribute;
        return S_OK;
    }

    TokenMap::iterator itr = m_eventMap.find(sourceCustomAttribute);
    if (itr != m_eventMap.end())
    {
        *pTargetCustomAttribute = itr->second;
        return S_OK;
    }

    mdToken sourceObj = mdTokenNil;
    mdToken sourceType = mdTokenNil;
    void const * pBlob = NULL;
    ULONG cbSize = 0;
    IfFailRet(m_pSourceImport->GetCustomAttributeProps(sourceCustomAttribute,
        &sourceObj,
        &sourceType,
        &pBlob,
        &cbSize));

    mdToken targetObj = mdTokenNil;
    mdToken targetType = mdTokenNil;
    HRESULT hr = ConvertToken(sourceObj, &targetObj);
    IfFailRet(hr);
    hr = ConvertToken(sourceType, &targetType);
    IfFailRet(hr);
    hr = m_pTargetEmit->DefineCustomAttribute(targetObj,
        targetType,
        pBlob,
        cbSize,
        pTargetCustomAttribute);
    //DISPLAY("custom attr hr = " << HEX(hr));
    IfFailRet(hr);
    DISPLAY(L"Injecting Custom Attribute " << " [" << HEX(*pTargetCustomAttribute) << L"]");
    m_eventMap[sourceCustomAttribute] = *pTargetCustomAttribute;
    return S_OK;
}

HRESULT AssemblyInjector::InjectMethodImpl(mdToken sourceImplementationClass, mdToken sourceImplementationMethod, mdToken sourceDeclarationMethod)
{
    mdToken targetImplementationClass = mdTokenNil;
    IfFailRet(ConvertToken(sourceImplementationClass, &targetImplementationClass));
    mdToken targetImplementationMethod = mdTokenNil;
    IfFailRet(ConvertToken(sourceImplementationMethod, &targetImplementationMethod));
    mdToken targetDeclarationMethod = mdTokenNil;
    IfFailRet(ConvertToken(sourceDeclarationMethod, &targetDeclarationMethod));

    IfFailRet(m_pTargetEmit->DefineMethodImpl(targetImplementationClass, targetImplementationMethod, targetDeclarationMethod));
    DISPLAY(L"Injecting MethodImpl Mapping " << " [" << HEX(targetImplementationMethod) << L"] => [" << HEX(targetDeclarationMethod) << L"]");
    return S_OK;
}

HRESULT AssemblyInjector::InjectParam(mdParamDef sourceParam, mdParamDef* pTargetParam)
{
    if (sourceParam == mdTokenNil)
    {
        *pTargetParam = sourceParam;
        return S_OK;
    }

    mdMethodDef sourceMethod = mdTokenNil;
    ULONG ulSequence;
    WCHAR szName[1000];
    ULONG cchName = 0;
    DWORD dwAttr = 0;
    DWORD dwCPlusTypeFlag = 0;
    UVCP_CONSTANT pValue = NULL;
    ULONG cchValue = 0;
    IfFailRet(m_pSourceImport->GetParamProps(sourceParam,
        &sourceMethod,
        &ulSequence,
        szName,
        1000,
        &cchName,
        &dwAttr,
        &dwCPlusTypeFlag,
        &pValue,
        &cchValue));

    mdMethodDef targetMethod = mdTokenNil;
    IfFailRet(InjectMethodDef(sourceMethod, &targetMethod));
    IfFailRet(m_pTargetEmit->DefineParam(targetMethod,
        ulSequence,
        szName,
        dwAttr,
        dwCPlusTypeFlag,
        pValue,
        cchValue,
        pTargetParam));

    DISPLAY(L"Injecting Param " << szName << " [" << HEX(*pTargetParam) << L"]");
    return S_OK;
}

HRESULT AssemblyInjector::InjectModuleRef(mdModuleRef sourceModuleRef, mdModuleRef* pTargetModuleRef)
{
    if (sourceModuleRef == mdTokenNil || sourceModuleRef == mdModuleRefNil)
    {
        *pTargetModuleRef = sourceModuleRef;
        return S_OK;
    }

    TokenMap::iterator itr = m_moduleRefMap.find(sourceModuleRef);
    if (itr != m_moduleRefMap.end())
    {
        *pTargetModuleRef = itr->second;
        return S_OK;
    }

    PROFILER_WCHAR szName[LONG_LENGTH];
    PLATFORM_WCHAR szNamePlatform[LONG_LENGTH];
    IfFailRet(m_pSourceImport->GetModuleRefProps(sourceModuleRef, szName, LONG_LENGTH, NULL));

    HCORENUM hEnum = NULL;
    HRESULT hr = S_OK;
    mdModuleRef curTargetModuleRef = mdTokenNil;
    BOOL bFoundMatch = FALSE;
    while (S_OK == (hr = m_pTargetImport->EnumModuleRefs(&hEnum, &curTargetModuleRef, 1, NULL)))
    {
        PROFILER_WCHAR szTargetModuleRefName[LONG_LENGTH];
        PLATFORM_WCHAR szTargetModuleRefNamePlatform[LONG_LENGTH];
        IfFailRet(m_pTargetImport->GetModuleRefProps(curTargetModuleRef, szTargetModuleRefName, LONG_LENGTH, NULL));

        ConvertProfilerWCharToPlatformWChar(szNamePlatform, LONG_LENGTH, szName, LONG_LENGTH);
        ConvertProfilerWCharToPlatformWChar(szTargetModuleRefNamePlatform, LONG_LENGTH, szTargetModuleRefName, LONG_LENGTH);
        if (wcscmp(szNamePlatform, szTargetModuleRefNamePlatform) == 0)
        {
            bFoundMatch = TRUE;
            m_moduleRefMap[sourceModuleRef] = curTargetModuleRef;
            *pTargetModuleRef = curTargetModuleRef;
            break;
        }
    }
    m_pTargetImport->CloseEnum(hEnum);
    IfFailRet(hr);

    if (!bFoundMatch)
    {
        IfFailRet(m_pTargetEmit->DefineModuleRef(szName, pTargetModuleRef));
        m_moduleRefMap[sourceModuleRef] = *pTargetModuleRef;
        DISPLAY(L"Injecting ModuleRef [" << HEX(*pTargetModuleRef) << L"]");
    }
    return S_OK;
}

HRESULT AssemblyInjector::InjectTypeSpec(mdTypeSpec sourceTypeSpec, mdTypeSpec* pTargetTypeSpec)
{
    if (sourceTypeSpec == mdTokenNil || sourceTypeSpec == mdTypeSpecNil)
    {
        *pTargetTypeSpec = sourceTypeSpec;
        return S_OK;
    }

    TokenMap::iterator itr = m_typeSpecMap.find(sourceTypeSpec);
    if (itr != m_typeSpecMap.end())
    {
        *pTargetTypeSpec = itr->second;
        return S_OK;
    }

    PCCOR_SIGNATURE pvSig = NULL;
    ULONG cbSig = 0;
    IfFailRet(m_pSourceImport->GetTypeSpecFromToken(sourceTypeSpec, &pvSig, &cbSig));
    IfFailRet(ConvertTypeSignatureCached(&pvSig, &cbSig));
    IfFailRet(m_pTargetEmit->GetTokenFromTypeSpec(pvSig, cbSig, pTargetTypeSpec));

    m_typeSpecMap[sourceTypeSpec] = *pTargetTypeSpec;
    DISPLAY(L"Injecting TypeSpec [" << HEX(*pTargetTypeSpec) << L"]");
    return S_OK;
}

HRESULT AssemblyInjector::InjectGenericParam(mdGenericParam sourceGenericParam, mdGenericParam* pTargetGenericParam)
{
    if (sourceGenericParam == mdTokenNil || sourceGenericParam == mdGenericParamNil)
    {
        *pTargetGenericParam = sourceGenericParam;
        return S_OK;
    }

    TokenMap::iterator itr = m_genericParamMap.find(sourceGenericParam);
    if (itr != m_genericParamMap.end())
    {
        *pTargetGenericParam = itr->second;
        return S_OK;
    }

    ULONG ulParamSeq = 0;
    DWORD dwParamFlags = 0;
    mdToken sourceOwner = mdTokenNil;
    DWORD reserved = 0;
    WCHAR szName[1000];
    IfFailRet(m_pSourceImport->GetGenericParamProps(sourceGenericParam,
        &ulParamSeq,
        &dwParamFlags,
        &sourceOwner,
        &reserved,
        szName,
        1000,
        NULL));

    HCORENUM hEnum = NULL;
    ULONG cGenericParamConstraints = 0;
    IfFailRet(m_pSourceImport->EnumGenericParamConstraints(&hEnum, sourceGenericParam, NULL, 0, NULL));
    mdGenericParamConstraint genericParamConstraints[1000];
    mdToken constraintTypes[1001];
    IfFailRet(m_pSourceImport->EnumGenericParamConstraints(&hEnum, sourceGenericParam, genericParamConstraints, 1000, &cGenericParamConstraints));
    DISPLAY(L"cGenericParamConstraints = " << cGenericParamConstraints);
    if (cGenericParamConstraints > 0)
    {
        for (unsigned int i = 0; i < cGenericParamConstraints; i++)
        {
            mdToken sourceConstraintType = mdTokenNil;
            IfFailRet(m_pSourceImport->GetGenericParamConstraintProps(genericParamConstraints[i], NULL, &sourceConstraintType));
            IfFailRet(ConvertToken(sourceConstraintType, &(constraintTypes[i])));
        }
    }
    constraintTypes[cGenericParamConstraints] = mdTokenNil;
    m_pSourceImport->CloseEnum(hEnum);

    mdToken targetOwner = mdTokenNil;
    IfFailRet(ConvertToken(sourceOwner, &targetOwner));
    IfFailRet(m_pTargetEmit->DefineGenericParam(targetOwner,
        ulParamSeq,
        dwParamFlags,
        szName,
        reserved,
        constraintTypes,
        pTargetGenericParam));

    m_genericParamMap[sourceGenericParam] = *pTargetGenericParam;
    DISPLAY(L"Injecting GenericParam " << szName << L" [" << HEX(*pTargetGenericParam) << L"]");
    return S_OK;
}

HRESULT AssemblyInjector::InjectMethodSpec(mdMethodSpec sourceMethodSpec, mdMethodSpec* pTargetMethodSpec)
{
    if (sourceMethodSpec == mdTokenNil || sourceMethodSpec == mdMethodSpecNil)
    {
        *pTargetMethodSpec = sourceMethodSpec;
        return S_OK;
    }

    TokenMap::iterator itr = m_methodSpecMap.find(sourceMethodSpec);
    if (itr != m_methodSpecMap.end())
    {
        *pTargetMethodSpec = itr->second;
        return S_OK;
    }

    mdToken sourceParent = mdTokenNil;
    PCCOR_SIGNATURE pvSigBlob = NULL;
    ULONG cbSigBlob = 0;
    IfFailRet(m_pSourceImport->GetMethodSpecProps(sourceMethodSpec,
        &sourceParent,
        &pvSigBlob,
        &cbSigBlob));
    mdToken targetParent = mdTokenNil;
    IfFailRet(ConvertToken(sourceParent, &targetParent));
    IfFailRet(ConvertNonTypeSignatureCached(&pvSigBlob, &cbSigBlob));
    IfFailRet(m_pTargetEmit->DefineMethodSpec(targetParent, pvSigBlob, cbSigBlob, pTargetMethodSpec));

    m_methodSpecMap[sourceMethodSpec] = *pTargetMethodSpec;
    DISPLAY(L"Injecting MethodSpec " << L" [" << HEX(*pTargetMethodSpec) << L"]");
    return S_OK;
}

HRESULT AssemblyInjector::ConvertToken(mdToken sourceToken, mdToken* pTargetToken)
{
    if (TypeFromToken(sourceToken) == mdtTypeDef)
    {
        IfFailRet(InjectTypeDef(sourceToken, pTargetToken));
    }
    else if (TypeFromToken(sourceToken) == mdtTypeRef)
    {
        IfFailRet(InjectTypeRef(sourceToken, pTargetToken));
    }
    else if (TypeFromToken(sourceToken) == mdtFieldDef)
    {
        IfFailRet(InjectFieldDef(sourceToken, pTargetToken));
    }
    else if (TypeFromToken(sourceToken) == mdtMethodDef)
    {
        IfFailRet(InjectMethodDef(sourceToken, pTargetToken));
    }
    else if (TypeFromToken(sourceToken) == mdtMemberRef)
    {
        IfFailRet(InjectMemberRef(sourceToken, pTargetToken));
    }
    else if (TypeFromToken(sourceToken) == mdtString)
    {
        IfFailRet(InjectString(sourceToken, pTargetToken));
    }
    else if (TypeFromToken(sourceToken) == mdtAssembly)
    {
        COMPtrHolder<IMetaDataAssemblyImport> pTargetAssemblyImport;
        IfFailRet(m_pTargetImport->QueryInterface(IID_IMetaDataAssemblyImport, (void**)&pTargetAssemblyImport));
        IfFailRet(pTargetAssemblyImport->GetAssemblyFromScope(pTargetToken));
    }
    else if (TypeFromToken(sourceToken) == mdtAssemblyRef)
    {
        IfFailRet(InjectAssemblyRef(sourceToken, pTargetToken));
    }
    else if (TypeFromToken(sourceToken) == mdtModuleRef)
    {
        IfFailRet(InjectModuleRef(sourceToken, pTargetToken));
    }
    else if (TypeFromToken(sourceToken) == mdtTypeSpec)
    {
        IfFailRet(InjectTypeSpec(sourceToken, pTargetToken));
    }
    else if (TypeFromToken(sourceToken) == mdtMethodSpec)
    {
        IfFailRet(InjectMethodSpec(sourceToken, pTargetToken));
    }
    else
    {
        FAILURE(L"Token conversion doesn't handle this token type yet - NYI: " << HEX(TypeFromToken(sourceToken)));
        return E_NOTIMPL;
    }
    return S_OK;
}

enum OPCLSA				  // Operand Class
{
    opclsaInlineNone,   // No operand

    // Immediate constants
    opclsaShortInlineI,		  // I1: signed  8-bit immediate value
    opclsaInlineI,			  // I4: signed 32-bit immediate value
    opclsaInlineI8,			  // I8: signed 64-bit immediate value
    opclsaShortInlineR,		  // R4: single precision real immediate
    opclsaInlineR,			  // R8: double precising real immediate

    // Locals and args
    opclsaShortInlineVar,		  // U1: local or arg index, unsigned 8-bit
    opclsaInlineVar,			  // U2: local or arg index, unsigned 16-bit

    // Branch labels
    opclsaShortInlineBrTarget,	   // I1: signed 8-bit offset from instruction after the branch
    opclsaInlineBrTarget,		  // I4: signed 32-bit offset from instruction after the branch
    opclsaInlineSwitch,		  // Multiple operands for switch statement

    // Relative Virtual Address
    opclsaInlineRVA,			  // U4: for ldptr

    // Token operands (T is equivalent to U4)
    opclsaInlineMethod,		  // T: Token for identifying a methods
    opclsaInlineType, 		  // T: Token for identifying an object type (box, unbox, ldobj, cpobj, initobj, stobj, mkrefany, refanyval, castclass, isinst, newarr, ldelema)
    opclsaInlineSig,			  // T: Token for identifying a method signature (calli)
    opclsaInlineField,		  // T: Token for identifying a field (ldflda, ldsflda, ldfld, ldsfld, ldsfld, stfld, stsfld)
    opclsaInlineString,		  // T: Token for identifying a string (ldstr)
    opclsaInlineTok,			  // T: Token for ldtoken
};

static OPCLSA g_opcodeInlineValues[] = {
#define OPDEF(op, name, stack1, stack2, i, kind, length, b1, b2, controlflow) opclsa ## i,
#include "opcode.def"
#undef OPDEF
};


HRESULT AssemblyInjector::ConvertILCode(const BYTE* pSourceILCode, BYTE* pTargetILCode, ULONG32 bufferSize)
{
    HRESULT hr = S_OK;
    for (ULONG32 cursor = 0; cursor < bufferSize;)
    {
        // MSIL opcodes can be either 1 byte or 2 bytes, possible followed by an inline value whose size is opcode dependent.
        ULONG32 opCode = pSourceILCode[cursor];
        pTargetILCode[cursor] = pSourceILCode[cursor];
        cursor++;
        // All valid 2 byte opcodes start with 0xFE, otherwise it is invalid or a 1 byte opcode.
        if (opCode == 0xFE)
        {
            if (cursor >= bufferSize)
                return E_FAIL;
            pTargetILCode[cursor] = pSourceILCode[cursor];
            opCode = pSourceILCode[cursor] + 256;
            cursor++;
        }
        // make sure we stay within the array
        if (opCode > MAX_MSIL_OPCODE)
            return E_FAIL;
        OPCLSA inlineValueKind = g_opcodeInlineValues[opCode];
        switch (inlineValueKind)
        {
        case opclsaInlineNone:
            break;

        case opclsaShortInlineI:		  // I1: signed  8-bit immediate value
        case opclsaShortInlineVar:		  // U1: local or arg index, unsigned 8-bit
        case opclsaShortInlineBrTarget:	   // I1: signed 8-bit offset from instruction after the branch
        {
            if (cursor + 1 > bufferSize)
                return E_FAIL;
            pTargetILCode[cursor] = pSourceILCode[cursor];
            cursor++;
            break;
        }

        case opclsaInlineVar:			  // U2: local or arg index, unsigned 16-bit
        {
            if (cursor + 2 > bufferSize)
                return E_FAIL;
            pTargetILCode[cursor] = pSourceILCode[cursor];
            pTargetILCode[cursor + 1] = pSourceILCode[cursor + 1];
            cursor += 2;
            break;
        }
        case opclsaInlineRVA:			  // U4: for ldptr
        {
            FAILURE(L"Converting IL code with inline RVA not supported");
            return E_NOTIMPL;
        }
        case opclsaInlineSig:			  // T: Token for identifying a method signature (calli)
        {
            FAILURE(L"Converting IL code with calli not supported");
            return E_NOTIMPL;
        }
        case opclsaInlineI:			  // I4: signed 32-bit immediate value
        case opclsaShortInlineR:      // R4: single precision real immediate
        case opclsaInlineBrTarget:		  // I4: signed 32-bit offset from instruction after the branch
        {
            if (cursor + 4 > bufferSize)
                return E_FAIL;
            memcpy(pTargetILCode + cursor, pSourceILCode + cursor, 4);
            cursor += 4;
            break;
        }

        case opclsaInlineI8:	      // I8: signed 64-bit immediate value  
        case opclsaInlineR:			  // R8: double precising real immediate
        {
            if (cursor + 8 > bufferSize)
                return E_FAIL;
            memcpy(pTargetILCode + cursor, pSourceILCode + cursor, 8);
            cursor += 8;
            break;
        }

        case opclsaInlineSwitch:		  // Multiple operands for switch statement
        {
            if (cursor + 4 > bufferSize)
                return E_FAIL;
            ULONG32 numCases = *(ULONG32*)(pSourceILCode + cursor);
            memcpy(pTargetILCode + cursor, pSourceILCode + cursor, 4);
            cursor += 4;
            if (cursor + 4 * numCases > bufferSize)
                return E_FAIL;
            memcpy(pTargetILCode + cursor, pSourceILCode + cursor, 4 * numCases);
            cursor += 4 * numCases;
            break;
        }

        case opclsaInlineMethod:		  // T: Token for identifying a methods
        case opclsaInlineType: 		  // T: Token for identifying an object type (box, unbox, ldobj, cpobj, initobj, stobj, mkrefany, refanyval, castclass, isinst, newarr, ldelema)
        case opclsaInlineField:		  // T: Token for identifying a field (ldflda, ldsflda, ldfld, ldsfld, ldsfld, stfld, stsfld)
        case opclsaInlineTok:			  // T: Token for ldtoken
        case opclsaInlineString:		  // T: Token for identifying a string (ldstr)
        {
            if (cursor + 4 > bufferSize)
                return E_FAIL;
            mdToken tk = *(mdToken*)(pSourceILCode + cursor);
            IfFailRet(ConvertToken(tk, &tk));
            *(mdToken*)(pTargetILCode + cursor) = tk;
            cursor += 4;
            break;
        }
        }
    }
    return S_OK;
}


HRESULT AssemblyInjector::ConvertNonTypeSignatureCached(PCCOR_SIGNATURE* ppSignature, ULONG* pcbSignature)
{
    if (ppSignature == NULL)
    {
        return S_OK;
    }
    MetadataSignatureBuilder sigBuilder;
    SignatureMap::iterator itr = m_sigToConvertedSigMap.find(*ppSignature);
    if (itr == m_sigToConvertedSigMap.end())
    {
        SigParser parser(*ppSignature, *pcbSignature);
        HRESULT hr = ConvertNonTypeSignature(parser, sigBuilder);
        if (FAILED(hr))
        {
            sigBuilder.DeleteSignature();
            return hr;
        }
        m_sigToConvertedSigMap[*ppSignature] = sigBuilder;
    }
    else
    {
        sigBuilder = itr->second;
    }

    sigBuilder.GetSignature(ppSignature, pcbSignature);
    return S_OK;
}

HRESULT AssemblyInjector::ConvertTypeSignatureCached(PCCOR_SIGNATURE* ppSignature, ULONG* pcbSignature)
{
    MetadataSignatureBuilder sigBuilder;
    SignatureMap::iterator itr = m_sigToConvertedSigMap.find(*ppSignature);
    if (itr == m_sigToConvertedSigMap.end())
    {
        SigParser parser(*ppSignature, *pcbSignature);
        HRESULT hr = ConvertTypeSignature(parser, sigBuilder);
        if (FAILED(hr))
        {
            sigBuilder.DeleteSignature();
            return hr;
        }
        m_sigToConvertedSigMap[*ppSignature] = sigBuilder;
    }
    else
    {
        sigBuilder = itr->second;
    }

    sigBuilder.GetSignature(ppSignature, pcbSignature);
    return S_OK;
}

HRESULT AssemblyInjector::ConvertMethodDefRefOrPropertySignature(SigParser & sig, MetadataSignatureBuilder & newSig, ULONG callConv)
{
    HRESULT hr = S_OK;
    if ((callConv & IMAGE_CEE_CS_CALLCONV_GENERIC) != 0)
    {
        IfFailRet(ConvertData(sig, newSig)); // genericParamCount
    }

    ULONG countParams;
    IfFailRet(sig.PeekData(&countParams));
    IfFailRet(ConvertData(sig, newSig));

    IfFailRet(ConvertParamSignature(sig, newSig)); // return value
    for (ULONG i = 0; i < countParams; i++)
    {
        IfFailRet(ConvertParamSignature(sig, newSig));
    }
    return hr;
}

HRESULT AssemblyInjector::ConvertNonTypeSignature(SigParser & sig, MetadataSignatureBuilder & newSig)
{
    HRESULT hr = S_OK;
    ULONG callConv;
    IfFailRet(sig.GetCallingConvInfo(&callConv));
    IfFailRet(newSig.WriteCallingConvInfo(callConv));
    ULONG maskedCallConv = callConv & IMAGE_CEE_CS_CALLCONV_MASK;
    if (maskedCallConv == IMAGE_CEE_CS_CALLCONV_DEFAULT || maskedCallConv == IMAGE_CEE_CS_CALLCONV_VARARG ||
        maskedCallConv == IMAGE_CEE_CS_CALLCONV_PROPERTY)
    {
        IfFailRet(ConvertMethodDefRefOrPropertySignature(sig, newSig, callConv));
    }
    else if (maskedCallConv == IMAGE_CEE_CS_CALLCONV_FIELD)
    {
        IfFailRet(ConvertFieldSignature(sig, newSig));
    }
    else if (maskedCallConv == IMAGE_CEE_CS_CALLCONV_GENERICINST)
    {
        IfFailRet(ConvertMethodSpecSignature(sig, newSig));
    }
    else if (maskedCallConv == IMAGE_CEE_CS_CALLCONV_LOCAL_SIG)
    {
        IfFailRet(ConvertLocalVarSignature(sig, newSig));
    }
    else
    {
        _ASSERTE(!"Unexpected signature type");
    }
    return S_OK;
}

HRESULT AssemblyInjector::ConvertLocalVarSignature(SigParser & sig, MetadataSignatureBuilder & newSig)
{
    HRESULT hr = S_OK;
    ULONG argCount = 0;
    IfFailRet(sig.PeekData(&argCount));
    IfFailRet(ConvertData(sig, newSig));
    for (ULONG i = 0; i < argCount; i++)
    {
        CorElementType et;
        IfFailRet(sig.PeekElemType(&et));
        if (et == ELEMENT_TYPE_TYPEDBYREF)
        {
            IfFailRet(ConvertElemType(sig, newSig));
        }
        else
        {
            while (et == ELEMENT_TYPE_CMOD_OPT || et == ELEMENT_TYPE_CMOD_REQD || et == ELEMENT_TYPE_PINNED)
            {
                if (et == ELEMENT_TYPE_PINNED)
                {
                    IfFailRet(ConvertElemType(sig, newSig));
                }
                else
                {
                    IfFailRet(ConvertCustomModSignature(sig, newSig));
                }
                IfFailRet(sig.PeekElemType(&et));
            }
            if (et == ELEMENT_TYPE_BYREF)
            {
                IfFailRet(ConvertElemType(sig, newSig));
            }
            IfFailRet(ConvertTypeSignature(sig, newSig));
        }
    }
    return S_OK;
}

HRESULT AssemblyInjector::ConvertMethodSpecSignature(SigParser & sig, MetadataSignatureBuilder & newSig)
{
    HRESULT hr = S_OK;
    ULONG argCount = 0;
    IfFailRet(sig.PeekData(&argCount));
    IfFailRet(ConvertData(sig, newSig));
    for (ULONG i = 0; i < argCount; i++)
    {
        IfFailRet(ConvertTypeSignature(sig, newSig));
    }
    return S_OK;
}

HRESULT AssemblyInjector::ConvertFieldSignature(SigParser & sig, MetadataSignatureBuilder & newSig)
{
    HRESULT hr = S_OK;
    IfFailRet(ConvertCustomModSignatureList(sig, newSig));
    IfFailRet(ConvertTypeSignature(sig, newSig));
    return S_OK;
}

HRESULT AssemblyInjector::ConvertTypeSignature(SigParser & sig, MetadataSignatureBuilder & newSig)
{
    HRESULT hr = S_OK;
    CorElementType et;
    IfFailRet(sig.PeekData((ULONG*)&et)); // avoid implicit conversion that occurs in PeekElemType
    IfFailRet(ConvertElemType(sig, newSig));
    if (!CorIsPrimitiveType(et))
    {
        switch (et)
        {
        case ELEMENT_TYPE_ARRAY:
            IfFailRet(ConvertTypeSignature(sig, newSig)); // componentType
            IfFailRet(ConvertData(sig, newSig)); // rank
            IfFailRet(ConvertLengthPrefixedDataList(sig, newSig)); // sizes
            IfFailRet(ConvertLengthPrefixedDataList(sig, newSig)); // low bounds
            break;
        case ELEMENT_TYPE_FNPTR:
            IfFailRet(ConvertNonTypeSignature(sig, newSig)); // methodref sig for function
            break;
        case ELEMENT_TYPE_PTR:
            IfFailRet(ConvertCustomModSignatureList(sig, newSig));
            IfFailRet(ConvertTypeSignature(sig, newSig)); // component type
            break;
        case ELEMENT_TYPE_SZARRAY:
            IfFailRet(ConvertCustomModSignatureList(sig, newSig));
            IfFailRet(ConvertTypeSignature(sig, newSig)); // component type
            break;
        case ELEMENT_TYPE_CLASS:
            IfFailRet(ConvertToken(sig, newSig)); // typeDefOrRef
            break;
        case ELEMENT_TYPE_GENERICINST:
            IfFailRet(ConvertElemType(sig, newSig)); // VALUETYPE or CLASS
            IfFailRet(ConvertToken(sig, newSig)); // typeDefOrRef, open type
            ULONG numTypes;
            IfFailRet(sig.PeekData(&numTypes));
            IfFailRet(ConvertData(sig, newSig)); // # type arguments
            for (ULONG i = 0; i < numTypes; i++)
            {
                IfFailRet(ConvertTypeSignature(sig, newSig)); // type arguments
            }
            break;
        case ELEMENT_TYPE_OBJECT:
            break;
        case ELEMENT_TYPE_MVAR:
            IfFailRet(ConvertData(sig, newSig)); // param #
            break;
        case ELEMENT_TYPE_VAR:
            IfFailRet(ConvertData(sig, newSig)); // param #
            break;
        case ELEMENT_TYPE_VALUETYPE:
            IfFailRet(ConvertToken(sig, newSig)); // typeDefOrRef
            break;
        case ELEMENT_TYPE_BYREF:
            IfFailRet(ConvertTypeSignature(sig, newSig)); // component type
            break;
        case ELEMENT_TYPE_TYPEDBYREF:
            break;
        default:
            _ASSERTE(!"Unexpected CorElementType");
        }
    }
    return hr;
}

HRESULT AssemblyInjector::ConvertToken(SigParser & sig, MetadataSignatureBuilder & newSig)
{
    mdToken tk;
    IfFailRet(sig.GetToken(&tk));
    IfFailRet(ConvertToken(tk, &tk));
    IfFailRet(newSig.WriteToken(tk));
    return S_OK;
}

HRESULT AssemblyInjector::ConvertElemType(SigParser & sig, MetadataSignatureBuilder & newSig)
{
    CorElementType et;
    IfFailRet(sig.GetElemType(&et));
    IfFailRet(newSig.WriteElemType(et));
    return S_OK;
}

HRESULT AssemblyInjector::ConvertData(SigParser & sig, MetadataSignatureBuilder & newSig)
{
    ULONG data;
    IfFailRet(sig.GetData(&data));
    IfFailRet(newSig.WriteData(data));
    return S_OK;
}

HRESULT AssemblyInjector::ConvertLengthPrefixedDataList(SigParser & sig, MetadataSignatureBuilder & newSig)
{
    HRESULT hr = S_OK;
    ULONG length = 0;
    IfFailRet(sig.PeekData(&length));
    IfFailRet(ConvertData(sig, newSig)); // length
    for (ULONG i = 0; i < length; i++)
    {
        IfFailRet(ConvertData(sig, newSig));
    }
    return S_OK;
}

HRESULT AssemblyInjector::ConvertParamSignature(SigParser & sig, MetadataSignatureBuilder & newSig)
{
    HRESULT hr = S_OK;
    IfFailRet(ConvertCustomModSignatureList(sig, newSig));
    IfFailRet(ConvertTypeSignature(sig, newSig));
    return S_OK;
}

HRESULT AssemblyInjector::ConvertCustomModSignatureList(SigParser & sig, MetadataSignatureBuilder & newSig)
{
    for (;;)
    {
        // peak at the next CorElementType to distinguish custom mod from
        // following Type/VOID
        CorElementType et;
        sig.PeekElemType(&et);
        if (et == ELEMENT_TYPE_CMOD_REQD || et == ELEMENT_TYPE_CMOD_OPT)
        {
            ConvertCustomModSignature(sig, newSig);
        }
        else
        {
            break;
        }
    }
    return S_OK;
}

HRESULT AssemblyInjector::ConvertCustomModSignature(SigParser & sig, MetadataSignatureBuilder & newSig)
{
    HRESULT hr = S_OK;
    IfFailRet(ConvertElemType(sig, newSig));
    IfFailRet(ConvertToken(sig, newSig));
    return S_OK;
}