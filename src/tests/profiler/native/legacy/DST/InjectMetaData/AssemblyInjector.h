#pragma once

class AssemblyInjector
{
public:
    AssemblyInjector(IPrfCom* pPrfCommon, ModuleID sourceModuleID, ModuleID targetModuleID, IMetaDataImport2* pSourceImport, IMetaDataImport2* pTargetImport, IMetaDataEmit2* pTargetEmit, IMethodMalloc* pTargetMethodMalloc);

    HRESULT InjectAll();
    HRESULT InjectTypes(const vector<const PLATFORM_WCHAR*> &targetTypes);

private:
    HRESULT InjectTypeDef(mdTypeDef sourceTypeDef, mdToken *pTargetTypeDef);
    HRESULT InjectTypeRef(mdTypeRef sourceTypeRef, mdToken *pTargetTypeRef);
    HRESULT InjectAssemblyRef(mdAssemblyRef tkTypeRef, mdToken *pTargetAssemblyRef);
    HRESULT InjectFieldDef(mdFieldDef sourceFieldDef, mdFieldDef *pTargetFieldDef);
    HRESULT InjectMethodDef(mdMethodDef sourceMethodDef, mdMethodDef *pTargetMethodDef);
    HRESULT InjectLocalVarSig(mdSignature sourceLocalVarSig, mdSignature *pTargetLocalVarSig);
    HRESULT InjectMemberRef(mdSignature sourceMemberRef, mdSignature *pTargetMemberRef);
    HRESULT InjectString(mdString sourceString, mdString *pTargetString);
    HRESULT InjectProperty(mdProperty sourceProperty, mdProperty *pTargetProperty);
    HRESULT InjectEvent(mdEvent sourceEvent, mdEvent *pTargetEvent);
    HRESULT InjectCustomAttribute(mdCustomAttribute sourceCustomAttribute, mdCustomAttribute *pTargetCustomAttribute);
    HRESULT InjectMethodImpl(mdToken sourceImplementationClass, mdToken sourceImplementationMethod, mdToken sourceDeclarationMethod);
    HRESULT InjectParam(mdParamDef sourceParam, mdParamDef* pTargetParam);
    HRESULT InjectModuleRef(mdModuleRef sourceModuleRef, mdModuleRef* pTargetModuleRef);
    HRESULT InjectTypeSpec(mdTypeSpec sourceTypeSpec, mdTypeSpec* pTargetTypeSpec);
    HRESULT InjectGenericParam(mdGenericParam sourceGenericParam, mdGenericParam* pTargetGenericParam);
    HRESULT InjectMethodSpec(mdMethodSpec sourceMethodSpec, mdMethodSpec* pTargetMethodSpec);

    HRESULT ConvertToken(mdToken token, mdToken* pTargetToken);
    HRESULT ConvertILCode(const BYTE* pSourceILCode, BYTE* pTargetILCode, ULONG32 ilCodeSize);
    HRESULT ConvertNonTypeSignatureCached(PCCOR_SIGNATURE* pSignature, ULONG* pcbSignature);
    HRESULT ConvertTypeSignatureCached(PCCOR_SIGNATURE* pSignature, ULONG* pcbSignature);
    HRESULT ConvertNonTypeSignature(SigParser & sig, MetadataSignatureBuilder & newSig);
    HRESULT ConvertMethodDefRefOrPropertySignature(SigParser & sig, MetadataSignatureBuilder & newSig, ULONG callConv);
    HRESULT ConvertFieldSignature(SigParser & sig, MetadataSignatureBuilder & newSig);
    HRESULT ConvertMethodSpecSignature(SigParser & sig, MetadataSignatureBuilder & newSig);
    HRESULT ConvertParamSignature(SigParser & sig, MetadataSignatureBuilder & newSig);
    HRESULT ConvertCustomModSignatureList(SigParser & sig, MetadataSignatureBuilder & newSig);
    HRESULT ConvertLocalVarSignature(SigParser & sig, MetadataSignatureBuilder & newSig);
    HRESULT ConvertTypeSignature(SigParser & sig, MetadataSignatureBuilder & newSig);
    HRESULT ConvertCustomModSignature(SigParser & sig, MetadataSignatureBuilder & newSig);
    HRESULT ConvertToken(SigParser & sig, MetadataSignatureBuilder & newSig);
    HRESULT ConvertData(SigParser & sig, MetadataSignatureBuilder & newSig);
    HRESULT ConvertElemType(SigParser & sig, MetadataSignatureBuilder & newSig);
    HRESULT ConvertLengthPrefixedDataList(SigParser & sig, MetadataSignatureBuilder & newSig);

    typedef std::unordered_map<PCCOR_SIGNATURE, MetadataSignatureBuilder> SignatureMap;
    SignatureMap m_sigToConvertedSigMap;
    typedef std::unordered_map<mdToken, mdToken> TokenMap;
    TokenMap m_typeDefMap;
    TokenMap m_typeRefMap;
    TokenMap m_assemblyRefMap;
    TokenMap m_fieldDefMap;
    TokenMap m_methodDefMap;
    TokenMap m_memberRefMap;
    TokenMap m_stringMap;
    TokenMap m_propertyMap;
    TokenMap m_eventMap;
    TokenMap m_customAttributeMap;
    TokenMap m_moduleRefMap;
    TokenMap m_typeSpecMap;
    TokenMap m_genericParamMap;
    TokenMap m_methodSpecMap;

    COMPtrHolder<IMetaDataImport2> m_pSourceImport;
    COMPtrHolder<IMetaDataImport2> m_pTargetImport;
    COMPtrHolder<IMetaDataEmit2> m_pTargetEmit;
    COMPtrHolder<IMethodMalloc> m_pTargetMethodMalloc;

    ModuleID m_sourceModuleID;
    ModuleID m_targetModuleID;
    IPrfCom* m_pPrfCom;
};