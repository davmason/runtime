// DynamicIL.cpp : Defines the entry point for the DLL application.
//

// TODO: reference additional headers your program requires here
#include <assert.h>
#include "profiler.h"
#include "corhlpr.h"

#define HEADERSIZE_TINY 1
#define HEADERSIZE_FAT 12
#define HEADERSIZE_TINY_MAX 64

#include "OpcodeHelper.h"
#include "CMethodInfo.h"
#include "CModuleInfo.h"
#include "DynamicIL.h"

// #include <corhlpr.cpp>

#ifdef _MANAGED
#pragma managed(push, off)
#endif

#pragma warning(push)
#pragma warning( disable : 4996)

void DynamicIL::VerifyAllocator(ModuleID moduleID, BOOL expectSuccess)
{
    if (!m_bVerifyAllocator)
        return;

    IMethodMalloc* pMethodMalloc = NULL;

    HRESULT hr = m_pPrfCom->m_pInfo->GetILFunctionBodyAllocator(moduleID, &pMethodMalloc);

    if (FAILED(hr) && expectSuccess)
    {
        if (hr == CORPROF_E_DATAINCOMPLETE)
        {
            DISPLAY(L"CORPROF_E_DATAINCOMPLETE");
            return;
        }
        else
        {
            FAILURE(L"Was not able to get ILFunctionBody Allocator\n");
        }
    }
    if (!expectSuccess)
    {
        _ASSERTE(pMethodMalloc == NULL);
        return;
    }
    _ASSERTE(pMethodMalloc);


    //Can't release this memory
    //  Will this leak????
    void* memory = pMethodMalloc->Alloc(0);
    if (memory != NULL)
    {
        FAILURE(L"Zero sized allocation succeeded, this should always fail");
    }
    memory = pMethodMalloc->Alloc(m_ulAllocationSize);
    if (memory == NULL && (m_ulAllocationSize > 0 && m_ulAllocationSize <= 0xFFFFF))
    {
        //Allocation's of 1MB or less should always succeed
        FAILURE(L"Allocation Failed " << m_ulAllocationSize);
        return;
    }
    if (memory != NULL)
    {
        memset(memory, 0x0, m_ulAllocationSize);
    }
    pMethodMalloc->Release();
}

BOOL DynamicIL::ShouldInstrumentMethod(CMethodInfo* pMethodInfo)
{
    if (IsManagedWrapperDLL(pMethodInfo->m_moduleID))
    {
        //This is our special Managed Wrapper DLL so don't instrument its functions
        return FALSE;
    }

    if (IsMscorlibDLL(pMethodInfo->m_moduleID))
    {
        if (_wcsicmp(pMethodInfo->m_wszClassName, L"System.Security.PermissionSet") == 0)
        {
            return FALSE;
        }
        if (_wcsicmp(pMethodInfo->m_wszClassName, L"System.Object") == 0)
        {
            return FALSE;
        }
    }

    return TRUE;
}


HRESULT DynamicIL::JitCompilationStarted(FunctionID functionId,
    BOOL fIsSafeToBlock)
{
    HRESULT hr = S_OK;
    static bool mscorlibinstrumented = false;
    //if (mscorlibinstrumented)
    //    return S_OK;
    //Create the MethodInfo on the stack to avoid allocating
    CMethodInfo* pMethodInfo = new CMethodInfo();
    //CMethodInfo* pMethodInfo = &methodInfo;

    hr = CreateMethodInfoFromFunctionID(functionId, pMethodInfo);

    if (FAILED(hr))
    {
        DISPLAY(L"FAILURE: Failed to create MethodInfo\n");
        return E_FAIL;
    }

    VerifyAllocator(pMethodInfo->m_moduleID, TRUE);

    if (!ShouldInstrumentMethod(pMethodInfo))
    {
        return S_OK;
    }

    //Remove methods from CacheLookup list
    if (m_bVerifyCacheToJitCount)
    {
        lock_guard<mutex> guard(m_csHashLock);
        size_t removed = m_phCacheLookupMethods->erase(functionId);
    }

    if (fIsSafeToBlock)
    {
        //DISPLAY(L"JitCompilationStarted(SafeToBlock=true) " << functionId << " " << pMethodInfo->FullSignature());
    }
    else
    {
        //DISPLAY(L"JitCompilationStarted(SafeToBlock=false) " << functionId << " " <<  pMethodInfo->FullSignature());
    }

    hr = m_pPrfCom->m_pInfo->GetILFunctionBody(pMethodInfo->m_moduleID,
        pMethodInfo->m_tkMethodDef,
        &pMethodInfo->m_pMethodHeader,
        &pMethodInfo->m_cbMethodSize);
    if (FAILED(hr))
    {
        return hr;
    }
    else if (hr == CORPROF_E_FUNCTION_NOT_IL)
    {
        //DISPLAY(L"MethodId points to method without IL <Verify this is PInvoke or abstract>\n");
    }

#if 0
    DISPLAY("**********Original CODE**********\n");
    ShowMethod(pMethodInfo);
    DISPLAY("**********Original END**********\n");
#endif

    if (!CreateProlog(pMethodInfo))
    {
        //The mothod prolog is not created
        //So just return without doing the IL rewrite
        return S_OK;
    }

    if (pMethodInfo->IsTinyHeader())
    {
        //If the Header is within certain bounds of the Tiny header lets convert to a FatHeader
        if (!pMethodInfo->CanModifyTinyHeader())
        {
            //DISPLAY(L"Converting Tiny Header to Fat Header\n");
            pMethodInfo->ConvertToFatHeader();
            //ShowMethod(pMethodInfo);
        }
    }

    if (pMethodInfo->IsTinyHeader())
    {
        _ASSERTE(!pMethodInfo->IsFatHeader());
        CreateNewFunctionBodyTiny(pMethodInfo);
    }
    else
    {
        _ASSERTE(!pMethodInfo->IsTinyHeader());
        CreateNewFunctionBodyFat(pMethodInfo);
    }

    hr = m_pPrfCom->m_pInfo->SetILFunctionBody(pMethodInfo->m_moduleID,
        pMethodInfo->m_tkMethodDef,
        pMethodInfo->m_pNewMethodHeader);

    if (FAILED(hr))
    {
        DISPLAY(L"Failed to set the new method body\n");
    }

    CMethodInfo newMethodInfo;
    CMethodInfo* pNewMethodInfo = &newMethodInfo;
    hr = CreateMethodInfoFromFunctionID(functionId, pNewMethodInfo);
    if (FAILED(hr))
    {
        DISPLAY(L"FAILURE: Failed to create MethodInfo\n");
        return E_FAIL;
    }

    hr = m_pPrfCom->m_pInfo->GetILFunctionBody(pNewMethodInfo->m_moduleID,
        pNewMethodInfo->m_tkMethodDef,
        &pNewMethodInfo->m_pMethodHeader,
        &pNewMethodInfo->m_cbMethodSize);

    if (FAILED(hr))
    {
        DISPLAY(L"FAILURE: Failed to retrieve MethodInfo after reseting the Function Body\n");
        FAILURE(L"FAILURE: " << HEX(hr));
    }

#if 0
    DISPLAY("**********ReWritten CODE**********\n");
    ShowMethod(pNewMethodInfo);
    DISPLAY("**********ReWritten CODE**********\n");
#endif

    _ASSERTE(pNewMethodInfo->m_cbMethodSize == pMethodInfo->m_cbNewMethodSize);
    delete pMethodInfo;
    pMethodInfo = NULL;
    pMethodInfo = new CMethodInfo();
    CreateMethodInfoFromFunctionID(functionId, pMethodInfo);

    if (m_bVerifyJitToCallbackCount && !pMethodInfo->m_bIsPartOfCER)
    {

        lock_guard<mutex> guard(m_csHashLock);
        m_phProbeEnterMethods->insert(FuncMethodPair(functionId, pMethodInfo));
    }
    ++m_ulInstrumented;
    return S_OK;
}

HRESULT DynamicIL::ModuleLoadFinished(ModuleID moduleID)
{
    VerifyAllocator(moduleID, TRUE);
    HRESULT hr = S_OK;

    CModuleInfo moduleInfo;
    CModuleInfo* pModuleInfo = new CModuleInfo();

    m_pModuleInfos->insert(ModuleInfoPair(moduleID, pModuleInfo));


    DISPLAY(L"ModuleLoadFinished");

    pModuleInfo->m_moduleID = moduleID;

    hr = m_pPrfCom->m_pInfo->GetModuleInfo(pModuleInfo->m_moduleID,
        &pModuleInfo->m_pBaseLoadAddress,
        pModuleInfo->m_cchModuleName,
        &pModuleInfo->m_cchModuleName,
        pModuleInfo->m_wszModuleName,
        &pModuleInfo->m_assemblyID);
    if (FAILED(hr))
    {
        DISPLAY(L"FAILURE: Failed to retrieve ModuleInfo\n");
        return hr;
    }
    _ASSERTE(pModuleInfo->m_pBaseLoadAddress != NULL);

    PLATFORM_WCHAR m_wszModuleNameTemp[MAX_PATH];
    ConvertProfilerWCharToPlatformWChar(m_wszModuleNameTemp, MAX_PATH, pModuleInfo->m_wszModuleName, pModuleInfo->m_cchModuleName);
    DISPLAY(L"ModuleLoadFinished: " << m_wszModuleNameTemp);

    //Once we've found our helper module we don't need to look anymore
    if (m_uDynILManagedModuleID == 0)
    {
        PLATFORM_WCHAR wszDLL[MAX_PATH];
        wszDLL[0] = L'\0';
#ifdef WINDOWS
        _wsplitpath_s(m_wszModuleNameTemp, NULL, 0, NULL, 0, wszDLL, MAX_PATH, NULL, 0);
        if (_wcsicmp(wszDLL, m_wszManagedProfilerDllNameNoExt) == 0)
#else
        int i = 0, start = 0, dot = -1;
        while (i < 256 && m_wszModuleNameTemp[i] != L'\0')
        {
            if (m_wszModuleNameTemp[i] == L'/')
                start = i + 1;
            else if (m_wszModuleNameTemp[i] == L'.')
                dot = i;
            ++i;
        }
        wstring a = wstring(m_wszModuleNameTemp).substr(start, dot - start);
        if (_wcsicmp(a.c_str(), m_wszManagedProfilerDllNameNoExt) == 0)
#endif
        {
            //Helper DLL
            //  -Don't instrument this wrapper
            //  -Don't add any metadata
            //
            DISPLAY(L"FOUND HELPER DLL\n");
            m_uDynILManagedModuleID = moduleID;
            return S_OK;
        }
    }

    if (m_uMscorlibModuleID == 0)
    {
        PLATFORM_WCHAR wszDLL[MAX_PATH];
        wszDLL[0] = L'\0';
#ifdef WINDOWS
        _wsplitpath_s(m_wszModuleNameTemp, NULL, 0, NULL, 0, wszDLL, MAX_PATH, NULL, 0);
        if ((_wcsicmp(wszDLL, L"mscorlib") == 0)
            || (_wcsicmp(wszDLL, L"mscorlib.ni") == 0)
            || (_wcsicmp(wszDLL, L"System.Private.CoreLib") == 0)
            || (_wcsicmp(wszDLL, L"System.Private.CoreLib.ni") == 0))
#else
        int i = 0, start = 0, dot = -1;
        while (i < 256 && m_wszModuleNameTemp[i] != L'\0')
        {
            if (m_wszModuleNameTemp[i] == L'/')
                start = i + 1;
            else if (m_wszModuleNameTemp[i] == L'.')
                dot = i;
            ++i;
        }
        wstring a = wstring(m_wszModuleNameTemp);
        wstring b = a.substr(start, dot - start);
        if ((_wcsicmp(b.c_str(), L"mscorlib") == 0)
            || (_wcsicmp(b.c_str(), L"mscorlib.ni") == 0)
            || (_wcsicmp(b.c_str(), L"System.Private.CoreLib") == 0)
            || (_wcsicmp(b.c_str(), L"System.Private.CoreLib.ni") == 0))

#endif
        {
            //Mscorlib
            //  Store off ModuleID so we can correctly probe the method
            DISPLAY(L"FOUND MSCORLIB DLL\n");
            m_uMscorlibModuleID = moduleID;
        }
    }


    //Don't perform any work on resource files
    //  Since there is no easy way to determine this just check if we can get the
    //  MetaDataEmit.
    COMPtrHolder<IMetaDataEmit2> pMetaDataEmit;
    hr = GetMetaData(pModuleInfo->m_moduleID,
        ofRead | ofWrite,
        IID_IMetaDataEmit2,
        (IUnknown**)&pMetaDataEmit);
    if (hr == DYNAMICIL_RESOURCEMODULE)
    {
        return S_OK;
    }

    if (IsMscorlibDLL(moduleID))
    {
        COMPtrHolder<IMetaDataImport2> pMetaDataImport;
        hr = GetMetaData(pModuleInfo->m_moduleID,
            ofRead,
            IID_IMetaDataImport2,
            (IUnknown**)&pMetaDataImport);

        if (FAILED(hr))
        {
            FAILURE(L"Failed to get MetaData for Mscorlib: " << HEX(hr));
        }
        hr = pMetaDataImport->FindTypeDefByName(PROFILER_STRING("System.Object"), NULL, &m_tkSystemObject);
        if (FAILED(hr))
        {
            FAILURE(L"Failed to find System.Object: " << HEX(hr));
        }
        hr = pMetaDataImport->FindTypeDefByName(PROFILER_STRING("System.Runtime.ConstrainedExecution.CriticalFinalizerObject"),
            NULL,
            &m_tkCriticalFinalizerObject);
        if (FAILED(hr))
        {
            FAILURE(L"Failed to find System.Runtime.ConstrainedExecution.CriticalFinalizerObject: " << HEX(hr));
        }

        CreateClassInMscorlib(pModuleInfo);

        if (m_eProlog == PROLOG_PINVOKE)
        {
            CreatePInvokeMethod(pModuleInfo);
        }

        return S_OK;
    }

    //if(m_eProlog == PROLOG_PINVOKE)
    //{
    //    //CreatePInvokeMethod(pModuleInfo);
    //}
    else
        if (m_eProlog == PROLOG_DYNAMICCLASS)
        {
            CreateDynamicClass(pModuleInfo);
        }
        else if (m_eProlog == PROLOG_MANAGEDWRAPPERDLL)
        {
            CreateManagedWrapperAssemblyRef(pModuleInfo);
        }

    ModuleInfoMapIterator piter;
    for (piter = m_pModuleInfos->begin(); piter != m_pModuleInfos->end(); piter++)
    {
        DISPLAY(L"pModuleInfo " << piter->second->m_moduleID);
    }

    return S_OK;
}

void DynamicIL::CreateClassInMscorlib(CModuleInfo* pModuleInfo)
{
    _ASSERTE(pModuleInfo != NULL);
    HRESULT hr = S_OK;
    COMPtrHolder<IMetaDataEmit2> pMetaDataEmit;
    COMPtrHolder<IMetaDataImport2> pMetaDataImport;

    hr = GetMetaData(pModuleInfo->m_moduleID,
        ofRead | ofWrite,
        IID_IMetaDataEmit2,
        (IUnknown**)&pMetaDataEmit);
    if (FAILED(hr))
    {
        FAILURE(L"Failed to get MetaData for Mscorlib: " << HEX(hr));
        return;
    }
    hr = GetMetaData(pModuleInfo->m_moduleID,
        ofRead,
        IID_IMetaDataImport2,
        (IUnknown**)&pMetaDataImport);
    if (FAILED(hr))
    {
        FAILURE(L"Failed to get MetaData for Mscorlib: " << HEX(hr));
        return;
    }

    mdToken tkSystemObjectDef;

    hr = pMetaDataImport->FindTypeDefByName(PROFILER_STRING("System.Object"),
        mdTypeDefNil,
        &tkSystemObjectDef);

    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: CreateClassInMscorlib, Failed to Find System.Object TypeDef: " << HEX(hr));
    }

    mdToken tkInstrumentationProbes;

    hr = pMetaDataEmit->DefineTypeDef(PROFILER_STRING("Instrumentation.Probes"),
        tdPublic | tdClass | tdAutoLayout,
        tkSystemObjectDef,
        NULL,
        &tkInstrumentationProbes);
    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: CreateClassInMscorlib, Failed to Emit TypeDef: " << HEX(hr));
    }

    //COR_SIGNATURE representation
    //   Calling convention
    //   Number of Arguments
    //   Return type
    //   Argument type
    //   ...

    COR_SIGNATURE newMethodSignature[] = { IMAGE_CEE_CS_CALLCONV_DEFAULT, //__stdcall
        2,
        ELEMENT_TYPE_VOID,
        ELEMENT_TYPE_I,
        ELEMENT_TYPE_I };

    /*  HRESULT DefineMethod(mdTypeDef td, LPCWSTR wzName,
    //        DWORD dwMethodFlags, PCCOR_SIGNATURE pvSig, ULONG cbSig,
    //        ULONG ulCodeRVA, DWORD dwImplFlags, mdMethodDef *pmd)
    //  [in] td - Typedef token of parent
    //     Passing mdTokenNil for mdTypeDef creates the method as global
    //  [in] wzName - Member name in Unicode
    //  [in] dwMethodFlags - Member attributes
    //     If PInvoke, must set mdStatic, clear mdSynchronized, clear mdAbstract
    //  [in] pvSig - Method signature
    //  [in] cbSig - Count of bytes in pvSig
    //  [in] ulCodeRVA - Address of code
    //     This can be 0
    //     If PInvoke, must be 0
    //  [in] dwImplFlags - Implementation flags for method
    //     If PInvoke, must set miNative, set miUnmanaged
    //  [out] pmd - Member token
    */

    PROFILER_WCHAR m_wszUnmanagedCallbackFunctionNameTemp[MAX_PATH];
    ConvertPlatformWCharToProfilerWChar(m_wszUnmanagedCallbackFunctionNameTemp, MAX_PATH, m_wszUnmanagedCallbackFunctionName, MAX_PATH);
    hr = pMetaDataEmit->DefineMethod(tkInstrumentationProbes,
        m_wszUnmanagedCallbackFunctionNameTemp,
        ~mdAbstract & (mdStatic | mdPublic | mdPinvokeImpl),
        newMethodSignature,
        sizeof(newMethodSignature),
        0,
        miPreserveSig,
        &m_tkIProbesPInvokeMethodDef);

    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: CreateClassInMscorlib Failed to Define PInvoke Method, " << m_wszUnmanagedCallbackFunctionName);
        return;
    }
    _ASSERTE(m_tkIProbesPInvokeMethodDef != NULL);

    PROFILER_WCHAR m_wszUnmangedProfilerDllNameNoExtTemp[MAX_PATH];
    ConvertPlatformWCharToProfilerWChar(m_wszUnmangedProfilerDllNameNoExtTemp, MAX_PATH, m_wszUnmangedProfilerDllNameNoExt, MAX_PATH);
    hr = pMetaDataEmit->DefineModuleRef(m_wszUnmangedProfilerDllNameNoExtTemp,
        &m_tkDynamicILModuleRef);

    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: CreateClassInMscorlib Failed to Create Module Ref\n");
        return;
    }

    hr = pMetaDataEmit->DefinePinvokeMap(m_tkIProbesPInvokeMethodDef,
        pmCallConvStdcall | pmNoMangle,
        m_wszUnmanagedCallbackFunctionNameTemp,
        m_tkDynamicILModuleRef);

    AddSuppressUnmanagedCodeSecurityAttributeToMethod(pMetaDataImport,
        pMetaDataEmit,
        m_tkIProbesPInvokeMethodDef);

}

void DynamicIL::CreateDynamicClass(CModuleInfo* pModuleInfo)
{
    _ASSERTE(pModuleInfo != NULL);

    HRESULT hr = S_OK;
    COMPtrHolder<IMetaDataEmit2> pMetaDataEmit;
    COMPtrHolder<IMetaDataAssemblyEmit> pMetaDataAssemblyEmit;
    COMPtrHolder<IMetaDataImport2> pMetaDataImport;

    hr = GetMetaData(pModuleInfo->m_moduleID,
        ofRead | ofWrite,
        IID_IMetaDataEmit2,
        (IUnknown**)&pMetaDataEmit);
    if (FAILED(hr))
    {
        FAILURE(L"Failed to get MetaData: " << HEX(hr));
        return;
    }

    hr = GetMetaData(pModuleInfo->m_moduleID,
        ofRead,
        IID_IMetaDataImport2,
        (IUnknown**)&pMetaDataImport);
    if (FAILED(hr))
    {
        FAILURE(L"Failed to get MetaData: " << HEX(hr));
        return;
    }

    mdTypeRef tkSystemObjectRef;

    hr = pMetaDataImport->FindTypeRef(GetMscorlibAssemblyRef(pModuleInfo),
        PROFILER_STRING("System.Object"),
        &tkSystemObjectRef);
    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: CreateDynamicClass, Failed to Find System.Object TypeRef: " << HEX(hr));
    }

    mdToken tkSystemObjectDef;

    hr = pMetaDataImport->FindTypeDefByName(PROFILER_STRING("System.Object"),
        mdTypeDefNil,
        &tkSystemObjectDef);

    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: CreateDynamicClass, Failed to Find System.Object TypeDef: " << HEX(hr));
    }

    hr = pMetaDataEmit->DefineTypeDef(PROFILER_STRING("DynamicILGenericClass"),
        tdPublic | tdClass | tdAutoLayout,
        m_tkSystemObject,
        NULL,
        &m_tkDynamicILGenericClass);
    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: CreateDynamicClass, Failed to Emit TypeDef: " << HEX(hr));
    }

    hr = pMetaDataEmit->DefineGenericParam(m_tkDynamicILGenericClass,
        0,
        0,
        PROFILER_STRING("T"),
        NULL,
        NULL,
        &m_tkDynamicILGenericClassParam);

    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: CreateDynamicClass, DefineGenericParam Failed: " << HEX(hr));
    }

    //Signature
    //  Calling Convention
    //  Number of type Parameters
    //  Number of Parameters
    //  Return Type
    //  Parameter types (If any)
    //  Previous Parameter type's type parameter id (i.e. type parameter #0)

    COR_SIGNATURE methodSig[] = { IMAGE_CEE_CS_CALLCONV_GENERIC | IMAGE_CEE_CS_CALLCONV_HASTHIS,
        1,
        1,
        ELEMENT_TYPE_VOID,
        ELEMENT_TYPE_VAR,
        0 };

    hr = pMetaDataEmit->DefineMethod(m_tkDynamicILGenericClass,
        PROFILER_STRING("DynamicILGenericMethod"),
        mdPublic,
        methodSig,
        sizeof(methodSig),
        0,
        miIL | miNoInlining,
        &m_tkDynamicILGenericMethod);

    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: CreateDynamicClass, DefineMethod failed: " << HEX(hr));
    }
}

void DynamicIL::CreateManagedWrapperAssemblyRef(CModuleInfo* pModuleInfo)
{
    _ASSERTE(pModuleInfo != NULL);

    HRESULT hr = S_FALSE;
    COMPtrHolder<IMetaDataEmit2> pMetaDataEmit;
    COMPtrHolder<IMetaDataImport2> pMetaDataImport;
    COMPtrHolder<IMetaDataAssemblyEmit> pMetaDataAssembly;

    hr = GetMetaData(pModuleInfo->m_moduleID,
        ofRead | ofWrite,
        IID_IMetaDataEmit2,
        (IUnknown**)&pMetaDataEmit);
    if (FAILED(hr))
    {
        FAILURE(L"Failed to get MetaData: " << HEX(hr));
        return;
    }
    hr = GetMetaData(pModuleInfo->m_moduleID,
        ofRead,
        IID_IMetaDataImport2,
        (IUnknown**)&pMetaDataImport);
    if (FAILED(hr))
    {
        FAILURE(L"Failed to get MetaData: " << HEX(hr));
        return;
    }
    hr = GetMetaData(pModuleInfo->m_moduleID,
        ofRead,
        IID_IMetaDataAssemblyEmit,
        (IUnknown**)&pMetaDataAssembly);
    if (FAILED(hr))
    {
        FAILURE(L"Failed to get MetaData: " << HEX(hr));
        return;
    }

    //COR_SIGNATURE representation
    //   Calling convention
    //   Number of Arguments
    //   Return type
    //   Argument type
    //   ...

    COR_SIGNATURE newMethodSignature[] = { IMAGE_CEE_CS_CALLCONV_DEFAULT, //__stdcall
        2,
        ELEMENT_TYPE_VOID,
        ELEMENT_TYPE_I,
        ELEMENT_TYPE_I };

    mdAssemblyRef assemblyRef = NULL;

    //BYTE publickeyToken[] = {B7, 7A, 5C, 56, 19, 34, E0, 89};

    /*
    STDMETHOD(DefineAssemblyRef)(           // S_OK or error.
    const void  *pbPublicKeyOrToken,    // [IN] Public key or token of the assembly.
    ULONG       cbPublicKeyOrToken,     // [IN] Count of bytes in the public key or token.
    LPCWSTR     szName,                 // [IN] Name of the assembly being referenced.
    const ASSEMBLYMETADATA *pMetaData,  // [IN] Assembly MetaData.
    const void  *pbHashValue,           // [IN] Hash Blob.
    ULONG       cbHashValue,            // [IN] Count of bytes in the Hash Blob.
    DWORD       dwAssemblyRefFlags,     // [IN] Flags.
    mdAssemblyRef *pmdar) PURE;         // [OUT] Returned AssemblyRef token.
    */

    PROFILER_WCHAR lplocal[MAX_PATH];
    PLATFORM_WCHAR wszlocal[MAX_PATH];
    wcscpy(wszlocal, L"neutral");

    ConvertPlatformWCharToProfilerWChar(lplocal, MAX_PATH, wszlocal, MAX_PATH);

    ASSEMBLYMETADATA assemblyMetaData;
    assemblyMetaData.usMajorVersion = 1;
    assemblyMetaData.usMinorVersion = 0;
    assemblyMetaData.usBuildNumber = 0;
    assemblyMetaData.usRevisionNumber = 0;
    assemblyMetaData.szLocale = lplocal;
    assemblyMetaData.cbLocale = MAX_PATH;


    BYTE publicKey[] = { 0xDF,
        0x7F,
        0x67,
        0x9C,
        0x71,
        0x07,
        0x8A,
        0x5D };


    PROFILER_WCHAR m_wszManagedProfilerDllNameNoExtTemp[MAX_PATH];
    ConvertPlatformWCharToProfilerWChar(m_wszManagedProfilerDllNameNoExtTemp, MAX_PATH, m_wszManagedProfilerDllNameNoExt, MAX_PATH);
    hr = pMetaDataAssembly->DefineAssemblyRef((void *)publicKey,
        sizeof(publicKey),
        m_wszManagedProfilerDllNameNoExtTemp,
        &assemblyMetaData,
        NULL,
        NULL,
        0,
        &assemblyRef);

    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: Error defining AssemblyRef " << HEX(hr));
    }

    mdTypeRef typeRef;
    PROFILER_WCHAR m_wszManagedProfilerClassNameTemp[MAX_PATH];
    ConvertPlatformWCharToProfilerWChar(m_wszManagedProfilerClassNameTemp, MAX_PATH, m_wszManagedProfilerClassName, MAX_PATH);
    hr = pMetaDataEmit->DefineTypeRefByName(assemblyRef,
        m_wszManagedProfilerClassNameTemp,
        &typeRef);
    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: Failed to Create TypeRef for " << m_wszManagedProfilerClassName);
    }

    PROFILER_WCHAR m_wszManagedProfilerMethodEnterNameTemp[MAX_PATH];
    ConvertPlatformWCharToProfilerWChar(m_wszManagedProfilerMethodEnterNameTemp, MAX_PATH, m_wszManagedProfilerMethodEnterName, MAX_PATH);
    hr = pMetaDataEmit->DefineMemberRef(typeRef,
        m_wszManagedProfilerMethodEnterNameTemp,
        newMethodSignature,
        sizeof(newMethodSignature),
        &m_tkDynamicILManagedFunctionEnter);

    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: Failed to Create MemberRef for " << m_wszManagedProfilerMethodEnterName);
    }

}

void DynamicIL::AddSuppressUnmanagedCodeSecurityAttributeToMethod(IMetaDataImport2* pMetaDataImport, IMetaDataEmit2* pMetaDataEmit, mdToken tkOwner)
{
    //COR_SUPPRESS_UNMANAGED_CODE_CHECK_ATTRIBUTE L"System.Security.SuppressUnmanagedCodeSecurityAttribute"
    HRESULT hr = S_OK;

    DISPLAY(L"Adding SuppressUnmanagedCodeSecurityAttribute, " << m_wszUnmanagedCallbackFunctionName);

    mdMemberRef tkSSUCSAttributeType = mdTokenNil;

    hr = pMetaDataImport->FindTypeDefByName(PROFILER_STRING("System.Security.SuppressUnmanagedCodeSecurityAttribute"),
        mdTokenNil,
        &tkSSUCSAttributeType);

    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: Failed to locate SuppressUnmanagedCodeSecurityAttribute, " << m_wszUnmanagedCallbackFunctionName);
        return;
    }

    DWORD dwSigSize = 3;
    BYTE rgSigBytesSuppressCtor[] = { 0x20, 0x00, 0x01 };
    mdToken tkSSUCSAttribute = mdTokenNil;
    hr = pMetaDataImport->FindMember(tkSSUCSAttributeType,
        PROFILER_STRING(".ctor"),
        rgSigBytesSuppressCtor,
        dwSigSize,
        &tkSSUCSAttribute);
    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: Failed to locate Attribute .ctor, " << m_wszUnmanagedCallbackFunctionName);
        return;
    }

    mdToken tkOwnersCustomAttribute = mdTokenNil;
    hr = pMetaDataEmit->DefineCustomAttribute(tkOwner, //token to apply attribute to
        tkSSUCSAttribute,  //token of CustomAttribute
        NULL,          //Blob, contains constructor params in this case none
        0,             //Size of the blob
        &tkOwnersCustomAttribute);
    if (FAILED(hr))
    {
        FAILURE(L"FAILED: Failed to DefineCustomAttribute, " << m_wszUnmanagedCallbackFunctionName);
        return;
    }

    DISPLAY(L"SUCCESS: Added SuppressUnmanagedCodeSecurityAttribute, " << m_wszUnmanagedCallbackFunctionName);

}

void DynamicIL::CreatePInvokeMethod(CModuleInfo* pModuleInfo)
{
    _ASSERTE(pModuleInfo != NULL);

    DISPLAY(L"Creating PInvokeMethod, " << m_wszUnmanagedCallbackFunctionName);

    HRESULT hr = S_FALSE;
    COMPtrHolder<IMetaDataEmit2> pMetaDataEmit;
    COMPtrHolder<IMetaDataImport2> pMetaDataImport;

    hr = GetMetaData(pModuleInfo->m_moduleID,
        ofRead | ofWrite,
        IID_IMetaDataEmit2,
        (IUnknown**)&pMetaDataEmit);
    if (FAILED(hr))
    {
        FAILURE(L"Failed to get MetaData: " << HEX(hr));
        return;
    }

    hr = GetMetaData(pModuleInfo->m_moduleID,
        ofRead,
        IID_IMetaDataImport2,
        (IUnknown**)&pMetaDataImport);
    if (FAILED(hr))
    {
        FAILURE(L"Failed to get MetaData: " << HEX(hr));
        return;
    }
    //COR_SIGNATURE representation
    //   Calling convention
    //   Number of Arguments
    //   Return type
    //   Argument type
    //   ...

    COR_SIGNATURE newMethodSignature[] = { IMAGE_CEE_CS_CALLCONV_DEFAULT, //__stdcall
        2,
        ELEMENT_TYPE_VOID,
        ELEMENT_TYPE_I,
        ELEMENT_TYPE_I };

    /*  HRESULT DefineMethod(mdTypeDef td, LPCWSTR wzName,
    //        DWORD dwMethodFlags, PCCOR_SIGNATURE pvSig, ULONG cbSig,
    //        ULONG ulCodeRVA, DWORD dwImplFlags, mdMethodDef *pmd)
    //  [in] td - Typedef token of parent
    //     Passing mdTokenNil for mdTypeDef creates the method as global
    //  [in] wzName - Member name in Unicode
    //  [in] dwMethodFlags - Member attributes
    //     If PInvoke, must set mdStatic, clear mdSynchronized, clear mdAbstract
    //  [in] pvSig - Method signature
    //  [in] cbSig - Count of bytes in pvSig
    //  [in] ulCodeRVA - Address of code
    //     This can be 0
    //     If PInvoke, must be 0
    //  [in] dwImplFlags - Implementation flags for method
    //     If PInvoke, must set miNative, set miUnmanaged
    //  [out] pmd - Member token
    */

    PROFILER_WCHAR m_wszUnmanagedCallbackFunctionNameTemp[MAX_PATH];
    ConvertPlatformWCharToProfilerWChar(m_wszUnmanagedCallbackFunctionNameTemp, MAX_PATH, m_wszUnmanagedCallbackFunctionName, MAX_PATH);
    hr = pMetaDataEmit->DefineMethod(mdTokenNil,
        m_wszUnmanagedCallbackFunctionNameTemp,
        ~mdAbstract & (mdStatic | mdPublic | mdPinvokeImpl),
        newMethodSignature,
        sizeof(newMethodSignature),
        0,
        miNative | miUnmanaged | miPreserveSig,
        &pModuleInfo->m_tkPInvokeMethodDef);

    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: Failed to Define PInvoke Method, " << m_wszUnmanagedCallbackFunctionName);
        return;
    }
    _ASSERTE(pModuleInfo->m_tkPInvokeMethodDef != NULL);

    PROFILER_WCHAR m_wszUnmangedProfilerDllNameNoExtTemp[MAX_PATH];
    ConvertPlatformWCharToProfilerWChar(m_wszUnmangedProfilerDllNameNoExtTemp, MAX_PATH, m_wszUnmangedProfilerDllNameNoExt, MAX_PATH);
    hr = pMetaDataEmit->DefineModuleRef(m_wszUnmangedProfilerDllNameNoExtTemp,
        &m_tkDynamicILModuleRef);

    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: Failed to Create Module Ref\n");
        return;
    }

    hr = pMetaDataEmit->DefinePinvokeMap(pModuleInfo->m_tkPInvokeMethodDef,
        pmCallConvStdcall | pmNoMangle,
        m_wszUnmanagedCallbackFunctionNameTemp,
        m_tkDynamicILModuleRef);

    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: Failed to Define PInvokeMap\n");
        return;
    }

}

HRESULT DynamicIL::GetModuleNameFromModuleID(ModuleID moduleID, PLATFORM_WCHAR *buff, INT buffLength)
{
    HRESULT hr = S_OK;
    PLATFORM_WCHAR moduleName[LONG_LENGTH];
    PROFILER_WCHAR moduleNameTemp[LONG_LENGTH];

    if (moduleID == 0)
    {
        //_snwprintf_s(buff, buffLength, buffLength, L"Unknown_ModuleID");
        wcscpy(buff, L"Unknown ModuleID");
        return E_FAIL;
    }

    AssemblyID AssemblyID;
    LPCBYTE pBaseLoadAddress;

    hr = m_pPrfCom->m_pInfo->GetModuleInfo(moduleID,
        &pBaseLoadAddress,
        LONG_LENGTH, //ULONG cchName
        NULL, //this is the actual length of moduleName ULONG *pcchName
        moduleNameTemp,
        &AssemblyID);

    if (FAILED(hr))
    {
        FAILURE(L"GetModuleNameFromModuleID returned " << HEX(hr) << L" for ModuleID " << HEX(moduleID));
        return hr;
    }
    
    ConvertProfilerWCharToPlatformWChar(moduleName, LONG_LENGTH, moduleNameTemp, LONG_LENGTH);
    wcscpy(buff, moduleName);

    return hr;
}

HRESULT DynamicIL::CreateMethodInfoFromFunctionID(FunctionID functionID, CMethodInfo *pMethodInfo)
{
    _ASSERTE(functionID != NULL);
    _ASSERTE(pMethodInfo != NULL);

    COMPtrHolder<IMetaDataImport2> pMetaDataImport;
    HRESULT hr = S_OK;
    mdToken tkMethodDef;

    pMethodInfo->m_functionID = functionID;

    hr = m_pPrfCom->m_pInfo->GetTokenAndMetaDataFromFunction(pMethodInfo->m_functionID,
        IID_IMetaDataImport2,
        (IUnknown**)&pMetaDataImport,
        &tkMethodDef);
    if (FAILED(hr))
    {
        FAILURE(L"FAILURE in GetTokenAndMetaDataFromFunction\n");
        return hr;
    }
    if ((IMetaDataImport2*)pMetaDataImport == NULL)
    {
        FAILURE(L"FAILURE Null pMetaDataImport\n");
        return E_FAIL;
    }
    if (IsNilToken(tkMethodDef))
    {
        FAILURE(L"FAILURE tkMethodDef is NULL\n");
    }

    COR_PRF_FRAME_INFO frameInfo = NULL;

    hr = m_pPrfCom->m_pInfo->GetFunctionInfo2(pMethodInfo->m_functionID,
        frameInfo,
        &pMethodInfo->m_classID,
        &pMethodInfo->m_moduleID,
        &tkMethodDef,
        (ULONG32)(sizeof(pMethodInfo->m_typeArgs) / sizeof(pMethodInfo->m_typeArgs[0])),
        &pMethodInfo->m_cTypeArgs,
        pMethodInfo->m_typeArgs);
    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: GetFunctionInfo Failed on " << HEX(functionID));
        return hr;
    }
    _ASSERTE(!IsNilToken(tkMethodDef));
    pMethodInfo->m_tkMethodDef = tkMethodDef;

    PROFILER_WCHAR wszModule[MAX_PATH];
    PLATFORM_WCHAR wszModuleTemp[MAX_PATH];
    ULONG cchModule = sizeof(wszModule) / sizeof(wszModule[0]);

    hr = m_pPrfCom->m_pInfo->GetModuleInfo(pMethodInfo->m_moduleID,
        &pMethodInfo->m_pBaseLoadAddress,
        cchModule,
        &cchModule,
        wszModule,
        &pMethodInfo->m_assemblyID);

    ConvertProfilerWCharToPlatformWChar(wszModuleTemp, MAX_PATH, wszModule, MAX_PATH);
    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: GetModuleInfo\n");
        return hr;
    }

    _ASSERTE(pMethodInfo->m_pBaseLoadAddress);

    PROFILER_WCHAR wszMethod[MAX_PATH];
    PLATFORM_WCHAR wszMethodTemp[MAX_PATH];
    ULONG cchMethod = sizeof(wszMethod) / sizeof(wszMethod[0]);

    mdTypeDef tkTypeDef;

    hr = pMetaDataImport->GetMethodProps(pMethodInfo->m_tkMethodDef,
        &tkTypeDef,
        wszMethod,
        cchMethod,
        &cchMethod,
        &pMethodInfo->m_dwMethodAttr,
        &pMethodInfo->m_signatureBlob,
        &pMethodInfo->m_cbSignatureBlob,
        &pMethodInfo->m_ulCodeRVA,
        &pMethodInfo->m_dwMethodImplFlags);

    ConvertProfilerWCharToPlatformWChar(wszMethodTemp, MAX_PATH, wszMethod, MAX_PATH);

    if (FAILED(hr))
    {
        FAILURE(L"FAILURE: IMetaDataImport->GetMethodProps failed to retrieve method\n");
        return hr;
    }
    _ASSERTE(!IsNilToken(tkTypeDef));
    pMethodInfo->m_tkTypeDef = tkTypeDef;
    CheckIfMethodIsPartOfCER(pMetaDataImport, pMethodInfo);

    // WCHAR wszClassName[MAX_PATH];
    ULONG cchClassName = sizeof(PROFILER_WCHAR *) / sizeof(PROFILER_WCHAR);

    DWORD dwTypeDefProps;
    mdToken tkExtends;

    PROFILER_WCHAR m_wszClassNameTemp[MAX_PATH];
    pMetaDataImport->GetTypeDefProps(pMethodInfo->m_tkTypeDef,
        m_wszClassNameTemp,
        cchClassName,
        &cchClassName,
        &dwTypeDefProps,
        &tkExtends);
    ConvertProfilerWCharToPlatformWChar(pMethodInfo->m_wszClassName, MAX_PATH, m_wszClassNameTemp, MAX_PATH);

    PCCOR_SIGNATURE pcCorSignature = pMethodInfo->m_signatureBlob;

    ULONG ulCallConv = IMAGE_CEE_CS_CALLCONV_MAX;
    pcCorSignature += CorSigUncompressData(pcCorSignature, &ulCallConv);
    _ASSERTE(ulCallConv != IMAGE_CEE_CS_CALLCONV_MAX);

    //PROFILER_WCHAR signatureBufferTemp[MAX_PATH];
    PLATFORM_WCHAR signatureBuffer[MAX_PATH];
    //PLATFORM_WCHAR wszMethodParametersTemp[MAX_PATH*10];
    signatureBuffer[0] = '\0';

    if (ulCallConv != IMAGE_CEE_CS_CALLCONV_FIELD)
    {
        pcCorSignature += CorSigUncompressData(pcCorSignature, &pMethodInfo->m_nArgumentCount);
        pcCorSignature = DynamicIL::ParseSignature(pMetaDataImport, pcCorSignature, signatureBuffer);

        for (ULONG i = 0; i<pMethodInfo->m_nArgumentCount; i++)
        {
            signatureBuffer[0] = '\0';
            pcCorSignature = DynamicIL::ParseSignature(pMetaDataImport, pcCorSignature, signatureBuffer);
            //ConvertProfilerWCharToPlatformWChar(signatureBufferTemp, MAX_PATH, signatureBuffer, MAX_PATH);
            wcscat_s(pMethodInfo->m_wszMethodParameters, MAX_PATH * 10, signatureBuffer);
            if (i<pMethodInfo->m_nArgumentCount - 1)
            {
                wcscat_s(pMethodInfo->m_wszMethodParameters, MAX_PATH * 10, L",");
            }
        }
    }

    wcscpy(pMethodInfo->m_wszModuleName, wszModuleTemp);
    wcscpy(pMethodInfo->m_wszMethodName, wszMethodTemp);
    //<TODO> Beef up this code path to prevent errors in string manipulation
    PLATFORM_WCHAR* stripped = wcsrchr(wszModuleTemp, '\\');
    if (stripped == NULL)
    {
        wcscpy(pMethodInfo->m_wszModuleNameNoPath, wszModuleTemp);
    }
    else
    {
        stripped++;
        wcscpy(pMethodInfo->m_wszModuleNameNoPath, stripped);
    }

    return hr;
}

void DynamicIL::CheckIfMethodIsPartOfCER(IMetaDataImport2* pMetaDataImport, CMethodInfo* pMethodInfo)
{
    _ASSERTE(pMethodInfo->m_tkMethodDef != NULL);
    _ASSERTE(pMethodInfo->m_tkTypeDef != NULL);
    // check to see if the token already has a custom attribute
    const void  *pData = NULL;               // [OUT] Put pointer to data here.
    ULONG       cbData = 0;
    // check for dupes??
    if (S_OK == pMetaDataImport->GetCustomAttributeByName(pMethodInfo->m_tkMethodDef,
        PROFILER_STRING("System.Runtime.ConstrainedExecution.ReliabilityContractAttribute"),
        &pData,
        &cbData))
    {
        //[ReliabilityContract(Consistency.WillNotCorruptState, Cer.Success)]
        pMethodInfo->m_bIsPartOfCER = TRUE;
        return;
    }

    //Check if the Type Extends CriticalFinalizerObject
    pMethodInfo->m_bIsPartOfCER = CheckIfMethodExtendsCriticalObject(pMetaDataImport, pMethodInfo->m_tkTypeDef);
}

BOOL DynamicIL::CheckIfMethodExtendsCriticalObject(IMetaDataImport2* pMetaDataImport, mdToken tkTypeDef)
{
    //System.Object
    //   02000002
    //CriticalFinalizerObject
    //   0200008a

    if (tkTypeDef == m_tkSystemObject)
    {
        return FALSE;
    }
    if (tkTypeDef == m_tkCriticalFinalizerObject)
    {
        return TRUE;
    }

    //IMetaDataImport2* pTypeRefMetaData = NULL;
    mdToken tkParentClass;
    HRESULT hr = S_OK;

    if (TypeFromToken(tkTypeDef) == mdtTypeRef)
    {

        //Token is TypeRef
        hr = pMetaDataImport->ResolveTypeRef(tkTypeDef,
            IID_IMetaDataImport2,
            (IUnknown**)&pMetaDataImport,
            &tkParentClass);
        if (FAILED(hr))
        {
            FAILURE(L"ResolveTypeRef failed " << HEX(hr) << L", for Type " << tkTypeDef);
        }

    }
    else if (TypeFromToken(tkTypeDef) == mdtTypeDef)
    {
        //Token is TypeDef
        HRESULT hr = pMetaDataImport->GetTypeDefProps(tkTypeDef,
            NULL,
            0,
            NULL,
            NULL,
            &tkParentClass);
        if (FAILED(hr) || IsNilToken(tkParentClass))
        {
            FAILURE(L"GetTypeDefProps returned " << HEX(hr) << L", for Type " << tkTypeDef);
        }
        if (hr == S_FALSE)
        {
            FAILURE(L"Passed in a TypeRef to GetTypeDefProps for Type " << HEX(tkTypeDef));
        }
    }
    else
    {
        //DISPLAY(L"typedefref**********");
        return TRUE;
    }

    return CheckIfMethodExtendsCriticalObject(pMetaDataImport, tkParentClass);
}

PCCOR_SIGNATURE DynamicIL::ParseSignature(IMetaDataImport* pMetaDataImport, PCCOR_SIGNATURE pCorSignature, PLATFORM_WCHAR* wszBuffer)
{
    _ASSERTE(pCorSignature);

    switch (*pCorSignature++)
    {
    case ELEMENT_TYPE_VOID:
        wcscat_s(wszBuffer, MAX_PATH, L"Void");
        break;
    case ELEMENT_TYPE_BOOLEAN:
        wcscat_s(wszBuffer, MAX_PATH, L"Boolean");
        break;
    case ELEMENT_TYPE_CHAR:
        wcscat_s(wszBuffer, MAX_PATH, L"Char");
        break;
    case ELEMENT_TYPE_I1:
        wcscat_s(wszBuffer, MAX_PATH, L"SByte");
        break;
    case ELEMENT_TYPE_U1:
        wcscat_s(wszBuffer, MAX_PATH, L"Byte");
        break;
    case ELEMENT_TYPE_I2:
        wcscat_s(wszBuffer, MAX_PATH, L"Int16");
        break;
    case ELEMENT_TYPE_U2:
        wcscat_s(wszBuffer, MAX_PATH, L"UInt16");
        break;
    case ELEMENT_TYPE_I4:
        wcscat_s(wszBuffer, MAX_PATH, L"Int32");
        break;
    case ELEMENT_TYPE_U4:
        wcscat_s(wszBuffer, MAX_PATH, L"UInt32");
        break;
    case ELEMENT_TYPE_I8:
        wcscat_s(wszBuffer, MAX_PATH, L"Int64");
        break;
    case ELEMENT_TYPE_U8:
        wcscat_s(wszBuffer, MAX_PATH, L"UInt64");
        break;
    case ELEMENT_TYPE_R4:
        wcscat_s(wszBuffer, MAX_PATH, L"Single");
        break;
    case ELEMENT_TYPE_R8:
        wcscat_s(wszBuffer, MAX_PATH, L"Double");
        break;
    case ELEMENT_TYPE_STRING:
        wcscat_s(wszBuffer, MAX_PATH, L"String");
        break;
        /*
        // every type above PTR will be simple type
        ELEMENT_TYPE_PTR            = 0xf,      // PTR <type>
        ELEMENT_TYPE_BYREF          = 0x10,     // BYREF <type>

        // Please use ELEMENT_TYPE_VALUETYPE. ELEMENT_TYPE_VALUECLASS is deprecated.
        */
    case ELEMENT_TYPE_VALUETYPE:
    case ELEMENT_TYPE_CLASS:
        mdToken token;
        PROFILER_WCHAR szClassName[MAX_PATH];
        PLATFORM_WCHAR szClassNameTemp[MAX_PATH];
        szClassName[0] = '\0';
        pCorSignature += CorSigUncompressToken(pCorSignature, &token);
        if (TypeFromToken(token) != mdtTypeRef)
        {
            HRESULT hr = pMetaDataImport->GetTypeDefProps(token, szClassName, MAX_PATH, NULL, NULL, NULL);
            if (FAILED(hr))
            {
                FAILURE(L"ParseSignature failed GetTypeDefProps on " << HEX(token));
                szClassName[0] = '\0';
            }
        }
        ConvertProfilerWCharToPlatformWChar(szClassNameTemp, MAX_PATH, szClassName, MAX_PATH);
        wcscat_s(wszBuffer, MAX_PATH, szClassNameTemp);
        break;

        /*
        ELEMENT_TYPE_VAR            = 0x13,     // a class type variable VAR <U1>
        ELEMENT_TYPE_ARRAY          = 0x14,     // MDARRAY <type> <rank> <bcount> <bound1> ... <lbcount> <lb1> ...
        ELEMENT_TYPE_GENERICINST    = 0x15,     // GENERICINST <generic type> <argCnt> <arg1> ... <argn>
        */
        // TYPEDREF  (it takes no args) a typed referece to some other type
    case ELEMENT_TYPE_TYPEDBYREF:
        wcscat_s(wszBuffer, MAX_PATH, L"TypedReference");
        break;
    case ELEMENT_TYPE_I:
        wcscat_s(wszBuffer, MAX_PATH, L"IntPtr");
        break;
    case ELEMENT_TYPE_U:
        wcscat_s(wszBuffer, MAX_PATH, L"UIntPtr");
        break;
        /*
        ELEMENT_TYPE_FNPTR          = 0x1B,     // FNPTR <complete sig for the function including calling convention>
        */
    case ELEMENT_TYPE_OBJECT:
        wcscat_s(wszBuffer, MAX_PATH, L"Object");
        break;
        /*
        ELEMENT_TYPE_SZARRAY        = 0x1D,     // Shortcut for single dimension zero lower bound array
        // SZARRAY <type>
        ELEMENT_TYPE_MVAR           = 0x1e,     // a method type variable MVAR <U1>

        // This is only for binding
        ELEMENT_TYPE_CMOD_REQD      = 0x1F,     // required C modifier : E_T_CMOD_REQD <mdTypeRef/mdTypeDef>
        ELEMENT_TYPE_CMOD_OPT       = 0x20,     // optional C modifier : E_T_CMOD_OPT <mdTypeRef/mdTypeDef>

        // This is for signatures generated internally (which will not be persisted in any way).
        ELEMENT_TYPE_INTERNAL       = 0x21,     // INTERNAL <typehandle>

        // Note that this is the max of base type excluding modifiers
        ELEMENT_TYPE_MAX            = 0x22,     // first invalid element type


        ELEMENT_TYPE_MODIFIER       = 0x40,
        ELEMENT_TYPE_SENTINEL       = 0x01 | ELEMENT_TYPE_MODIFIER, // sentinel for varargs
        ELEMENT_TYPE_PINNED         = 0x05 | ELEMENT_TYPE_MODIFIER,
        ELEMENT_TYPE_R4_HFA         = 0x06 | ELEMENT_TYPE_MODIFIER, // used only internally for R4 HFA types
        ELEMENT_TYPE_R8_HFA         = 0x07 | ELEMENT_TYPE_MODIFIER, // used only internally for R8 HFA types
        */
    default:
    case ELEMENT_TYPE_END:
        //<TODO> Enable this Error, need to complete signature parsing before
        //FAILURE(L"Unknown ELEMENT_TYPE");
        wcscat_s(wszBuffer, MAX_PATH, L"??Unknown??");
        break;
    }
    //ConvertPlatformWCharToProfilerWChar(wszBuffer, MAX_PATH, wszBuffer, MAX_PATH);
    return pCorSignature;
}

void DynamicIL::CreateNewFunctionBodyTiny(CMethodInfo *pMethodInfo)
{
    ULONG cbNewMethodSizeWithoutHeader = 0;

    _ASSERTE(pMethodInfo);
    _ASSERTE(pMethodInfo->IsTinyHeader());
    _ASSERTE(pMethodInfo->CanModifyTinyHeader());
    pMethodInfo->m_cbNewMethodSize = pMethodInfo->m_cbILProlog + pMethodInfo->m_cbMethodSize;
    cbNewMethodSizeWithoutHeader = pMethodInfo->m_cbNewMethodSize - HEADERSIZE_TINY;

    COMPtrHolder<IMethodMalloc> pMethodMalloc;

    //Get a FunctionBodyAllocator
    //  Must make sure we release this.
    HRESULT hr = m_pPrfCom->m_pInfo->GetILFunctionBodyAllocator(pMethodInfo->m_moduleID, &pMethodMalloc);
    if (FAILED(hr))
    {
        FAILURE(L"Failed to get ILFunctionBodyAllocator\n");
    }
    _ASSERTE(pMethodMalloc);
    void* pNewMethodHeader = pMethodMalloc->Alloc(pMethodInfo->m_cbNewMethodSize);
    //Release the ILFunctionBodyAllocator

    _ASSERTE(pNewMethodHeader);
    if (pNewMethodHeader == NULL)
    {
        //Need to return Fail
        return;
    }

    //This is done so we can set the new MethodHeaderSize
    BYTE header = (BYTE)cbNewMethodSizeWithoutHeader;
    header = (header << 2) | CorILMethod_TinyFormat;
    _ASSERTE((header & (CorILMethod_FormatMask >> 1)) == CorILMethod_TinyFormat);
    //Copy over the new header
    memcpy(pNewMethodHeader, &header, sizeof(BYTE));
    //Copy over our prolog
    memcpy((BYTE*)pNewMethodHeader + HEADERSIZE_TINY, (void *)pMethodInfo->m_ILProlog, pMethodInfo->m_cbILProlog);
    //Copy the old function
    memcpy((BYTE*)pNewMethodHeader + HEADERSIZE_TINY + pMethodInfo->m_cbILProlog, pMethodInfo->m_pMethodHeader + HEADERSIZE_TINY, pMethodInfo->m_cbMethodSize - HEADERSIZE_TINY);

    pMethodInfo->m_pNewMethodHeader = (BYTE*)pNewMethodHeader;
}

void DynamicIL::CreateNewFunctionBodyFat(CMethodInfo *pMethodInfo)
{
    _ASSERTE(pMethodInfo);
    _ASSERTE(pMethodInfo->IsFatHeader());

    COR_ILMETHOD_FAT* pCorILMethodFat = (COR_ILMETHOD_FAT*)pMethodInfo->m_pMethodHeader;
    COR_ILMETHOD* pCorILMethod = (COR_ILMETHOD*)pCorILMethodFat;

    COR_ILMETHOD_DECODER method((const COR_ILMETHOD *)pCorILMethod);

    ULONG cbSehExtra = 0; // extra bytes for the exception tables
    ULONG cbSehMethodSize = 0; // "real" method size incl. method header
    int nDelta = 0; // old method: delta between last IL byte and first SEH header
    int nNewDelta = 0; // new method: delta between last IL byte and first SEH header

    ULONG cbPrologSize = pMethodInfo->m_cbILProlog;

    ULONG ulAlignment = 0; // to hold alignment bytes
                           // split cbMethodSize value to get "real" method size and SEH sections size
    if (pMethodInfo->HasSEH())
    {
        SEHWalker(pMethodInfo);
        // get "real" method size
        // 1. get header size ( currently 12 bytes )
        cbSehMethodSize = pCorILMethodFat->Size * 4; // size is DWords so multiply by 4 to get the actual bytes
        _ASSERTE(cbSehMethodSize == HEADERSIZE_FAT);
        // 2. get method IL body size
        ULONG alignDword = sizeof(DWORD) - 1;
        // cbSehMethodSize = headsize + codesieze (include seh)
        cbSehMethodSize += pCorILMethodFat->GetCodeSize();// + alignDword ) & (~alignDword) ; // IL code size
                                                          // 3. note that cbMethodSize already includes method header, IL body, and SEH sections
                                                          // it DOES NOT include alignment size (delta) !!!
                                                          // FAT header is always DWORD aligned!!!
                                                          // SEH header (FAT and SMALL) is always DWORD aligned!!!
                                                          // SEH clause (FAT and SMALL) is always DWORD aligned!!!
        cbSehExtra = pMethodInfo->m_cbMethodSize - cbSehMethodSize;

        _ASSERTE(cbSehExtra > 0);

        //<TODO>
        // get delta
        int nExtra = pMethodInfo->GetOldMethodExtraSize(nDelta);
        _ASSERTE(nExtra > 0);
        _ASSERTE((ULONG)nExtra == cbSehExtra - nDelta);

        // align size if method has SEH
        ulAlignment = ((cbPrologSize) % sizeof(DWORD));
        if (ulAlignment == 0)
        {
            //DWORD aligned with changes
            //   didn't break alignment we don't have to do anything
            nNewDelta = nDelta;
        }
        else
        {
            ULONG ulTotalDelta = ((cbPrologSize + cbSehMethodSize) % sizeof(DWORD));
            if (ulTotalDelta == 0)
            {
                nNewDelta = 0;
            }
            else
            {
                _ASSERTE((sizeof(DWORD) - ulTotalDelta) > 0);
                nNewDelta = sizeof(DWORD) - ulTotalDelta;
            }
        }
    }

    // new method size (including header)
    pMethodInfo->m_cbNewMethodSize = HEADERSIZE_FAT;
    pMethodInfo->m_cbNewMethodSize += cbPrologSize;
    if (!pMethodInfo->HasSEH())
    {
        // old method size w/o FAT HEADER
        pMethodInfo->m_cbNewMethodSize += (pMethodInfo->m_cbMethodSize - HEADERSIZE_FAT);
        _ASSERTE(pMethodInfo->m_cbNewMethodSize >= pMethodInfo->m_cbMethodSize);
    }
    else
    {
        // old method size w/o FAT HEADER
        pMethodInfo->m_cbNewMethodSize += (cbSehMethodSize - HEADERSIZE_FAT);
        _ASSERTE(pMethodInfo->m_cbNewMethodSize >= cbSehMethodSize);
    }
    ULONG cbNewMethodSizeWithoutHeader = (pMethodInfo->m_cbNewMethodSize - HEADERSIZE_FAT);
    ULONG cbMethodSizeWithoutHeader = 0;
    if (!pMethodInfo->HasSEH())
    {
        cbMethodSizeWithoutHeader = pMethodInfo->m_cbMethodSize - HEADERSIZE_FAT;
    }
    else
    {
        cbMethodSizeWithoutHeader = cbSehMethodSize - HEADERSIZE_FAT;
    }

    WORD cbOldMaxStackSize = (WORD)((IMAGE_COR_ILMETHOD*)pMethodInfo->m_pMethodHeader)->Fat.MaxStack;
    //Increase the MaxStack by a factor of half the prolog
    //   This is only a rough estimate but should suffice
    WORD cbNewMaxStackSize = cbOldMaxStackSize + ((WORD)cbPrologSize / 2);
    _ASSERTE(cbNewMaxStackSize >= cbOldMaxStackSize);

    // allocate memory
    COMPtrHolder<IMethodMalloc> pMethodMalloc;
    //Get a FunctionBodyAllocator
    //  Must make sure we release this.
    HRESULT hr = m_pPrfCom->m_pInfo->GetILFunctionBodyAllocator(pMethodInfo->m_moduleID, &pMethodMalloc);
    if (FAILED(hr))
    {
        FAILURE(L"Failed to get ILFunctionBodyAllocator\n");
    }

    _ASSERTE(pMethodMalloc);
    //New Size of the Method + alignment + SEH stuff for DWORD alignment
    void* pNewMethodHeader = pMethodMalloc->Alloc(pMethodInfo->m_cbNewMethodSize + nNewDelta + cbSehExtra);

    _ASSERTE(pNewMethodHeader);

    if (pNewMethodHeader == NULL)
    {
        DISPLAY(L"pNewMethodHeader == NULL");
        return;
    }

    // create new function
    // copy old header
    memcpy(pNewMethodHeader, pMethodInfo->m_pMethodHeader, HEADERSIZE_FAT);
    // change MaxStack
    memcpy((BYTE*)pNewMethodHeader + 2, &cbNewMaxStackSize, sizeof(WORD));
    // specify the new method size
    memcpy((BYTE*)pNewMethodHeader + 4, (void*)&cbNewMethodSizeWithoutHeader, sizeof(ULONG) /*DWORD*/);
    // copy the prolog
    _ASSERTE(pMethodInfo->m_ILProlog > 0);
    memcpy((BYTE*)pNewMethodHeader + HEADERSIZE_FAT, (void*)pMethodInfo->m_ILProlog, pMethodInfo->m_cbILProlog);
    // copy old function body
    _ASSERTE(cbMethodSizeWithoutHeader > 0);
    memcpy((BYTE*)pNewMethodHeader + HEADERSIZE_FAT + pMethodInfo->m_cbILProlog, (BYTE*)pMethodInfo->m_pMethodHeader + HEADERSIZE_FAT, cbMethodSizeWithoutHeader);

    //<TODO> Add the SEH Tables and fix them up
    if (pMethodInfo->HasSEH())
    {
        // copy SEH sections
        // cbSehExtra = (pLast - pFirst)!!!
        // Check that header is DWORD aligned
        _ASSERTE((((size_t)((BYTE*)pNewMethodHeader + HEADERSIZE_FAT + pMethodInfo->m_cbILProlog + cbMethodSizeWithoutHeader + nNewDelta)) & 3) == 0);
        memcpy((BYTE*)pNewMethodHeader + HEADERSIZE_FAT + cbPrologSize + cbMethodSizeWithoutHeader + nNewDelta,
            (BYTE*)pMethodInfo->m_pMethodHeader + HEADERSIZE_FAT + cbMethodSizeWithoutHeader + nDelta,
            cbSehExtra);


        // now fix the actual method size
        // cbNewMethodSize += nNewDelta; // add alignment, DO NOT UNCOMMENT THIS LINE!!!
        pMethodInfo->m_cbNewMethodSize += cbSehExtra; // add SEH extra bytes, DO NOT COUNT "delta"!!!
        pMethodInfo->m_cbNewMethodSize += (nNewDelta - nDelta); //Why NOT count "Delta"?
    }

#if 0
    DumpBytes((BYTE*)pMethodInfo->m_pMethodHeader, HEADERSIZE_FAT + cbMethodSizeWithoutHeader + nDelta + cbSehExtra);
    DumpBytes((BYTE*)pNewMethodHeader, HEADERSIZE_FAT + cbPrologSize + cbMethodSizeWithoutHeader + nNewDelta + cbSehExtra);
#endif

    pMethodInfo->m_pNewMethodHeader = (BYTE*)pNewMethodHeader;
    if (pMethodInfo->HasSEH())
    {
        // walk SEH sections to see if they all are OK
        int nCheckDelta = 0;
        int nExtra = pMethodInfo->GetNewMethodExtraSize(nCheckDelta);
        //_ASSERTE( nExtra > 0 );
        //_ASSERTE( (ULONG)nExtra == cbSehExtra );
        //_ASSERTE( nNewDelta == nCheckDelta );

        // shift offsets in SEH clauses
        FixSEHSections(pMethodInfo);
    }
}

void DynamicIL::SEHWalker(CMethodInfo *pMethodInfo)
{
    LPCBYTE pMethodHeader = pMethodInfo->m_pMethodHeader;
    if (pMethodInfo->HasSEH()) // only for FAT methods
    {
        _ASSERTE(pMethodInfo->HasSEH());
        COR_ILMETHOD_FAT* pMethod = (COR_ILMETHOD_FAT*)pMethodHeader;
        BYTE* pSectsBegin = (BYTE*)(pMethod->GetCode() + pMethod->GetCodeSize());
        _ASSERTE(pSectsBegin == (BYTE*)((BYTE*)pMethodHeader + HEADERSIZE_FAT + ((IMAGE_COR_ILMETHOD*)pMethodHeader)->Fat.CodeSize));

        // Enum all EH sections and for each of them its EH clauses >>
        COR_ILMETHOD* pMethodData = (COR_ILMETHOD*)pMethod;
        COR_ILMETHOD_DECODER method((const COR_ILMETHOD*)pMethodData);
        IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT*	 pFatEHClause;
        IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_SMALL* pSmallEHClause;
        // get the very first section
        // can be SMALL or FAT section, don't know yet
        COR_ILMETHOD_SECT_EH* pCurEHSect = (COR_ILMETHOD_SECT_EH*)method.EH;
        unsigned nCount, nIndex;
        mdToken	token;

        // declare and init counters
        int nFatSections(0), nSmallSections(0);

        do
        {
            // for a given EH section get # of its EH clauses
            nCount = pCurEHSect->EHCount();
            //DISPLAY( L"SEHWalker: EH section has '"<<HEX(nCount)<<L"' EH clause(s)\r\n" );
            if (pCurEHSect->IsFat())
            {
                ++nFatSections;
                //DISPLAY( L"SEHWalker: FAT clause\r\n" );
                pFatEHClause = pCurEHSect->Fat.Clauses;
                for (nIndex = 0; nIndex < nCount; ++nIndex)
                {
                    switch (pFatEHClause[nIndex].Flags)
                    {
                    case COR_ILEXCEPTION_CLAUSE_NONE: // typed handler
                        token = (mdToken)pFatEHClause[nIndex].ClassToken;
                        //DISPLAY( L"SEHWalker: FAT EH clause has 'CATCH' type. Exception Object Token '"<<HEX(token)<<L"'\r\n"  );
                        break;
                    case COR_ILEXCEPTION_CLAUSE_FILTER: //  = 0x0001, if this bit is on, then this EH entry is for a filter
                                                        //DISPLAY( L"SEHWalker: FAT EH clause has 'FILTER' type.\r\n" );
                                                        //DISPLAY( L"SEHWalker: FilterOffset (from beginning of the method IL code?): '"<<HEX(pFatEHClause[nIndex].FilterOffset)<<L"'\r\n" );
                        break;
                    case COR_ILEXCEPTION_CLAUSE_FINALLY: //  = 0x0002, this clause is a finally clause
                                                         //DISPLAY( L"SEHWalker: FAT EH clause is a finally clause.\r\n" );
                        break;
                    case COR_ILEXCEPTION_CLAUSE_FAULT:  // = 0x0004, fault clause (finally that is called on exception only)
                                                        //DISPLAY( L"SEHWalker: FAT EH clause is a fault clause.\r\n" );
                        break;
                    default:
                        _ASSERTE(FALSE);
                    } /* switch */

                      //DISPLAY( L"SEHWalker: FAT EH clause fields:\r\n" );
                      //DISPLAY( L"SEHWalker: TryOffset (from beginning of the method IL code): '"<<HEX(pFatEHClause[nIndex].TryOffset)<<L"'\r\n");
                      //DISPLAY( L"SEHWalker: TryLength (relative to start of try block): '"<<HEX(pFatEHClause[nIndex].TryLength)<<L"'\r\n");
                      //DISPLAY( L"SEHWalker: HandlerOffset: '"<<HEX(pFatEHClause[nIndex].HandlerOffset)<<L"'\r\n");
                      //DISPLAY( L"SEHWalker: HandlerLength (relative to start of handler block): '"<<HEX(pFatEHClause[nIndex].HandlerLength)<<L"'\r\n");

                }
            }
            else
            {
                ++nSmallSections;
                //DISPLAY( L"SEHWalker: SMALL clause\r\n" );
                pSmallEHClause = pCurEHSect->Small.Clauses;
                for (nIndex = 0; nIndex < nCount; ++nIndex)
                {
                    switch (pSmallEHClause[nIndex].Flags)
                    {
                    case COR_ILEXCEPTION_CLAUSE_NONE: // typed handler
                        token = (mdToken)pSmallEHClause[nIndex].ClassToken;
                        //DISPLAY( L"SEHWalker: SMALL EH clause has 'CATCH' type. Exception Object Token '"<<HEX(token)<<L"'\r\n");
                        break;
                    case COR_ILEXCEPTION_CLAUSE_FILTER: //  = 0x0001, if this bit is on, then this EH entry is for a filter
                                                        //DISPLAY( L"SEHWalker: SMALL EH clause has 'FILTER' type.\r\n" );
                                                        //DISPLAY( L"SEHWalker: FilterOffset (from beginning of the method IL code?): '"<<HEX(pSmallEHClause[nIndex].FilterOffset)<<L"'\r\n");
                        break;
                    case COR_ILEXCEPTION_CLAUSE_FINALLY: //  = 0x0002, this clause is a finally clause
                                                         //DISPLAY( L"SEHWalker: SMALL EH clause is a finally clause.\r\n" );
                        break;
                    case COR_ILEXCEPTION_CLAUSE_FAULT:  // = 0x0004, fault clause (finally that is called on exception only)
                                                        //DISPLAY( L"SEHWalker: SMALL EH clause is a fault clause.\r\n" );
                        break;
                    default:
                        _ASSERTE(FALSE);
                    } /* switch */

                      //DISPLAY( L"SEHWalker: SMALL EH clause has following fields:\r\n" );
                      //DISPLAY( L"SEHWalker: TryOffset (from beginning of the method IL code): '"<<HEX(pSmallEHClause[nIndex].TryOffset)<<L"'\r\n");
                      //DISPLAY( L"SEHWalker: TryLength (relative to start of try block): '"<<HEX(pSmallEHClause[nIndex].TryLength)<<L"'\r\n");
                      //DISPLAY( L"SEHWalker: HandlerOffset: '"<<HEX(pSmallEHClause[nIndex].HandlerOffset)<<L"'\r\n");
                      //DISPLAY( L"SEHWalker: HandlerLength (relative to start of handler block): '"<<HEX(pSmallEHClause[nIndex].HandlerLength)<<L"'\r\n");
                }
            }

            // Find the next EH section
            do
            {
                pCurEHSect = (COR_ILMETHOD_SECT_EH*)pCurEHSect->Next();
            } while (pCurEHSect && (pCurEHSect->Kind() & CorILMethod_Sect_KindMask) != CorILMethod_Sect_EHTable);

        } while (pCurEHSect);
        //DISPLAY( L"SEHWalker: method has '"<<nFatSections<<L"' FAT and '"<<nSmallSections<<L"' SMALL sections\r\n");
    } /* if */
}

void DynamicIL::FixSEHSections(CMethodInfo *pMethodInfo)
{
    LPCBYTE pMethodHeader;
    _ASSERTE(pMethodInfo->HasSEH());
    pMethodHeader = pMethodInfo->m_pNewMethodHeader;

    COR_ILMETHOD_FAT* pMethod = (COR_ILMETHOD_FAT*)pMethodHeader;
    BYTE* pSectsBegin = (BYTE*)(pMethod->GetCode() + pMethod->GetCodeSize());
    _ASSERTE(pSectsBegin == (BYTE*)((BYTE*)pMethodHeader + HEADERSIZE_FAT + ((IMAGE_COR_ILMETHOD*)pMethodHeader)->Fat.CodeSize));


    // Enum all EH sections and for each of them its EH clauses and, then, fix them >>
    COR_ILMETHOD* pMethodData = (COR_ILMETHOD*)pMethod;
    COR_ILMETHOD_DECODER method((const COR_ILMETHOD*)pMethodData);
    IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT*	 pFatEHClause;
    IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_SMALL* pSmallEHClause;
    // get the very first section
    // can be SMALL or FAT section, don't know yet
    COR_ILMETHOD_SECT_EH* pCurEHSect = (COR_ILMETHOD_SECT_EH*)method.EH;
    _ASSERTE((((size_t)pCurEHSect) & 3) == 0); // SEH section header is DWORD aligned
    unsigned nCount, nIndex;
    mdToken	token;

    // sections counters
    int nFatSections(0), nSmallSections(0);

    do
    {
        // for a given EH section get # of its EH clauses
        nCount = pCurEHSect->EHCount();
        if (pCurEHSect->IsFat()) // FAT SEH section
        {
            ++nFatSections;
            pFatEHClause = pCurEHSect->Fat.Clauses;
            for (nIndex = 0; nIndex < nCount; ++nIndex)
            {
                switch (pFatEHClause[nIndex].Flags)
                {
                case COR_ILEXCEPTION_CLAUSE_NONE: // typed handler
                    token = (mdToken)pFatEHClause[nIndex].ClassToken;
                    break;
                case COR_ILEXCEPTION_CLAUSE_FILTER: //  = 0x0001, if this bit is on, then this EH entry is for a filter
                                                    // since it has 4 bytes size
                    pFatEHClause[nIndex].FilterOffset += pMethodInfo->m_cbILProlog;
                    break;
                case COR_ILEXCEPTION_CLAUSE_FINALLY: //  = 0x0002, this clause is a finally clause
                case COR_ILEXCEPTION_CLAUSE_FAULT:  // = 0x0004, fault clause (finally that is called on exception only)
                    break;
                default:
                    _ASSERTE(FALSE);
                }
                //DISPLAY( L"FixSEHInfo: old FAT EH clause has following fields:\r\n" );
                //DISPLAY( L"FixSEHInfo: TryOffset (from beginning of the method IL code): '"<<HEX(pFatEHClause[nIndex].TryOffset)<<L"'\r\n");
                //DISPLAY( L"FixSEHInfo: TryLength (relative to start of try block): '"<<HEX(pFatEHClause[nIndex].TryLength )<<L"'\r\n");
                //DISPLAY( L"FixSEHInfo: HandlerOffset: '"<<HEX(pFatEHClause[nIndex].HandlerOffset)<<L"'\r\n");
                //DISPLAY( L"FixSEHInfo: HandlerLength (relative to start of handler block): '"<<HEX(pFatEHClause[nIndex].HandlerLength)<<L"'\r\n");

                // modified sizes
                //DISPLAY( L"FixSEHInfo: new FAT EH clause has following fields:\r\n" );
                // modify TryOffset
                pFatEHClause[nIndex].TryOffset += pMethodInfo->m_cbILProlog;
                //DISPLAY( L"FixSEHInfo: TryOffset (from beginning of the method IL code): '"<<HEX(pFatEHClause[nIndex].TryOffset)<<L"'\r\n");
                //DISPLAY( L"FixSEHInfo: TryLength (relative to start of try block): '"<<HEX(pFatEHClause[nIndex].TryLength)<<L"'\r\n");
                // modify HandlerOffset
                pFatEHClause[nIndex].HandlerOffset += pMethodInfo->m_cbILProlog;
                //DISPLAY( L"FixSEHInfo: HandlerOffset: '"<<HEX(pFatEHClause[nIndex].HandlerOffset)<<L"'\r\n");
                // same
                //DISPLAY( L"FixSEHInfo: HandlerLength (relative to start of handler block): '"<<HEX(pFatEHClause[nIndex].HandlerLength)<<L"'\r\n");
            }
        }
        else
        {
            ++nSmallSections;
            pSmallEHClause = pCurEHSect->Small.Clauses;
            for (nIndex = 0; nIndex < nCount; ++nIndex)
            {
                switch (pSmallEHClause[nIndex].Flags)
                {
                case COR_ILEXCEPTION_CLAUSE_NONE: // typed handler
                    token = (mdToken)pSmallEHClause[nIndex].ClassToken;
                    break;
                case COR_ILEXCEPTION_CLAUSE_FILTER: //  = 0x0001, if this bit is on, then this EH entry is for a filter
                                                    // since it has 4 bytes size
                    pSmallEHClause[nIndex].FilterOffset += pMethodInfo->m_cbILProlog;
                    break;
                case COR_ILEXCEPTION_CLAUSE_FINALLY: //  = 0x0002, this clause is a finally clause
                case COR_ILEXCEPTION_CLAUSE_FAULT:  // = 0x0004, fault clause (finally that is called on exception only)
                    break;
                default:
                    _ASSERTE(FALSE);
                }

                //DISPLAY( L"FixSEHInfo: old SMALL EH clause has following fields:\r\n" );
                //DISPLAY( L"FixSEHInfo: TryOffset (from beginning of the method IL code): '"<<HEX(pSmallEHClause[nIndex].TryOffset)<<L"'\r\n");
                //DISPLAY( L"FixSEHInfo: TryLength (relative to start of try block): '"<<HEX(pSmallEHClause[nIndex].TryLength)<<L"'\r\n");
                //DISPLAY( L"FixSEHInfo: HandlerOffset: '"<<HEX(pSmallEHClause[nIndex].HandlerOffset)<<L"'\r\n");
                //DISPLAY( L"FixSEHInfo: HandlerLength (relative to start of handler block): '"<<HEX(pSmallEHClause[nIndex].HandlerLength)<<L"'\r\n");

                // modified sizes
                //DISPLAY( L"FixSEHInfo: new SMALL EH clause has following fields:\r\n" );
                // modify TryOffset
                if ((pSmallEHClause[nIndex].TryOffset + pMethodInfo->m_cbILProlog) < 0xFFFF) // cannot modify if it's bigger then 2 bytes
                {
                    pSmallEHClause[nIndex].TryOffset += pMethodInfo->m_cbILProlog;
                }
                else
                {
                    FAILURE(L"FixSEHInfo: TryOffset cannot be changed: (pSmallEHClause[nIndex].TryOffset + cbNewOffset) = '" << HEX((pSmallEHClause[nIndex].TryOffset + pMethodInfo->m_cbILProlog)) << L"'\r\n");
                }
                //DISPLAY( L"FixSEHInfo: TryOffset (from beginning of the method IL code): '"<<HEX(pSmallEHClause[nIndex].TryOffset)<<L"'\r\n");
                //DISPLAY( L"FixSEHInfo: TryLength (relative to start of try block): '"<<HEX(pSmallEHClause[nIndex].TryLength)<<L"'\r\n");

                // modify HandlerOffset
                if ((pSmallEHClause[nIndex].HandlerOffset + pMethodInfo->m_cbILProlog) < 0xFFFF) // cannot modify if it's bigger then 2 bytes
                {
                    pSmallEHClause[nIndex].HandlerOffset += pMethodInfo->m_cbILProlog;
                }
                else
                {
                    FAILURE(L"FixSEHInfo: HandlerOffset cannot be changed: (pSmallEHClause[nIndex].HandlerOffset + cbNewOffset) = '" << HEX((pSmallEHClause[nIndex].HandlerOffset + pMethodInfo->m_cbILProlog)) << L"'\r\n");
                }
                //DISPLAY( L"FixSEHInfo: HandlerOffset: '"<<HEX(pSmallEHClause[nIndex].HandlerOffset)<<L"'\r\n");
                //DISPLAY( L"FixSEHInfo: HandlerLength (relative to start of handler block): '"<<HEX(pSmallEHClause[nIndex].HandlerLength)<<L"'\r\n");
            }
        }

        // Find the next EH section
        do
        {
            pCurEHSect = (COR_ILMETHOD_SECT_EH*)pCurEHSect->Next();
        } while (pCurEHSect && (pCurEHSect->Kind() & CorILMethod_Sect_KindMask) != CorILMethod_Sect_EHTable);

    } while (pCurEHSect);

    if (nFatSections > 0)
    {
        _ASSERTE(nSmallSections == 0);
    }
    if (nSmallSections > 0)
    {
        _ASSERTE(nFatSections == 0);
    }
}

BOOL DynamicIL::CreateProlog(CMethodInfo *pMethodInfo)
{
    //Each method gets a prolog that consists of
    //    ldc.i4.1 //CEE_LDC_I4_1
    //    pop      //CEE_POP
    //    nop
    //    ///TODO add generic call
    //    newobj  DynamicILGenericClass<T>::.ctor()
    //    ldc.i4   //CEE_LDC_I4 <functionID>
    //    call void DynamicILGenericClass::DynamicILGeneric

    ModuleID moduleID = pMethodInfo->m_moduleID;
    ModuleInfoMapIterator pIter = m_pModuleInfos->find(moduleID);
    if (pIter == m_pModuleInfos->end())
    {
        FAILURE("can not find moduleinfo " << moduleID);
        return FALSE;
    }
    CModuleInfo *pModuleInfo = pIter->second;

    if (pMethodInfo->m_moduleID != pModuleInfo->m_moduleID)
    {
        FAILURE("pMethodInfo->m_moduleID != pModuleInfo->m_moduleID ");
        return FALSE;
    }

    if (m_eProlog == PROLOG_PINVOKE && IsNilToken(pModuleInfo->m_tkPInvokeMethodDef))
    {
        return FALSE;
    }

    OpcodeInfo Opcode_ldc_i4_1 = g_opcodes[CEE_LDC_I4_1];
    OpcodeInfo Opcode_pop = g_opcodes[CEE_POP];
    OpcodeInfo Opcode_nop = g_opcodes[CEE_NOP];
    OpcodeInfo Opcode_ldc_i4 = g_opcodes[CEE_LDC_I4];
    OpcodeInfo Opcode_ldc_i8 = g_opcodes[CEE_LDC_I8];
    OpcodeInfo Opcode_newobj = g_opcodes[CEE_NEWOBJ];
    OpcodeInfo Opcode_call = g_opcodes[CEE_CALL];

#if 0
    DISPLAY(L"" << HEX(pMethodInfo->m_moduleID) << L" - " << HEX(pMethodInfo->m_functionID));
#endif
    switch (m_eProlog)
    {
    case PROLOG_PUSHPOP:
        //Creates prolog of 5 loads and 5 pops then a nop
        EmitIL(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, Opcode_ldc_i4_1);
        EmitIL(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, Opcode_ldc_i4_1);
        EmitIL(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, Opcode_ldc_i4_1);
        EmitIL(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, Opcode_ldc_i4_1);
        EmitIL(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, Opcode_ldc_i4_1);
        EmitIL(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, Opcode_pop);
        EmitIL(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, Opcode_pop);
        EmitIL(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, Opcode_pop);
        EmitIL(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, Opcode_pop);
        EmitIL(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, Opcode_pop);
        EmitIL(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, Opcode_nop);
        break;

    case PROLOG_PINVOKE:
    case PROLOG_MANAGEDWRAPPERDLL:
        /*
        //This code emits a PInvoke call to m_tkPInvokeMethodDef
        //  Because 64-bit varies from 32-bit we need to ifdef for the platform
        //    ldc.i4   //moduleID
        //    ldc.i4   //functionID
        //    call void [DynamicIL]PInvokeHelloCallback(moduleID, functionID)
        */
#if WIN64 || _AMD64_ || _IA64_ || __ARM_ARCH_ISA_A64 || defined (_M_ARM64)
        EmitIL(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, Opcode_ldc_i8);
        EmitILPTR(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, pMethodInfo->m_moduleID, 8);
        EmitIL(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, Opcode_ldc_i8);
        EmitILPTR(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, pMethodInfo->m_functionID, 8);
#else
        EmitIL(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, Opcode_ldc_i4);
        EmitILPTR(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, pMethodInfo->m_moduleID, 4);
        EmitIL(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, Opcode_ldc_i4);
        EmitILPTR(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, pMethodInfo->m_functionID, 4);
#endif


        EmitIL(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, Opcode_call);
        if (IsMscorlibDLL(pMethodInfo->m_moduleID))
        {
            _ASSERTE(!IsNilToken(m_tkIProbesPInvokeMethodDef));
            EmitILToken(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, m_tkIProbesPInvokeMethodDef);
        }
        else if (m_eProlog == PROLOG_PINVOKE)
        {
            _ASSERTE(!IsNilToken(pModuleInfo->m_tkPInvokeMethodDef));
            EmitILToken(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, pModuleInfo->m_tkPInvokeMethodDef);
        }
        else if (m_eProlog == PROLOG_MANAGEDWRAPPERDLL)
        {
            _ASSERTE(!IsNilToken(m_tkDynamicILManagedFunctionEnter));
            EmitILToken(pMethodInfo->m_ILProlog, &pMethodInfo->m_cbILProlog, m_tkDynamicILManagedFunctionEnter);
        }
        break;

    default:
        _ASSERTE("Invalid PrologType");
    }
    //#endif
    //EmitIL(pMethodInfo->m_ILProlog,&pMethodInfo->m_cbILProlog,Opcode_newobj);
    //EmitILToken(pMethodInfo->m_ILProlog,&pMethodInfo->m_cbILProlog,m_tkDynamicILGenericClass);
    //EmitIL(pMethodInfo->m_ILProlog,&pMethodInfo->m_cbILProlog,Opcode_ldc_i4);
    //EmitILToken(pMethodInfo->m_ILProlog,&pMethodInfo->m_cbILProlog,(ULONG32)pMethodInfo->m_functionID);
    //EmitIL(pMethodInfo->m_ILProlog,&pMethodInfo->m_cbILProlog,Opcode_call);
    //EmitILToken(pMethodInfo->m_ILProlog,&pMethodInfo->m_cbILProlog,m_tkDynamicILGenericMethod);
#if 0
    DISPLAY(L"Prolog " << pMethodInfo->m_cbILProlog);
    DISPLAY(L"**********");
    DumpBytes(pMethodInfo->m_ILProlog, pMethodInfo->m_cbILProlog);
    DISPLAY(L"**********");
#endif

    return TRUE;
}

void DynamicIL::EmitIL(BYTE* pBuffer, ULONG* pCurrentOffset, OpcodeInfo opcodeInfo)
{
    if (opcodeInfo.Byte1 == 0xFF)
    {
        pBuffer[*pCurrentOffset] = opcodeInfo.Byte2;
        (*pCurrentOffset)++;
    }
}

void DynamicIL::EmitILToken(BYTE* pBuffer, ULONG* pCurrentOffset, ULONG32 token)
{
    for (int i = 0; i < 4; i++)
    {
        pBuffer[(*pCurrentOffset)++] = (token >> (i * 8)) & 0xFF;
    }
}
void DynamicIL::EmitILPTR(BYTE* pBuffer, ULONG* pCurrentOffset, UINT_PTR token, int tokenSize)
{
    for (int i = 0; i < tokenSize; i++)
    {
#pragma warning(push)
#pragma warning(disable : 4244)
        pBuffer[(*pCurrentOffset)++] = (token >> (i * 8)) & 0xFF;
#pragma warning(pop)
    }
}


void DynamicIL::DumpBytes(LPCBYTE pBuffer, const ULONG ulBufferSize)
{
    int nCnt = 0;
    wstringstream byteLine;

    for (LPCBYTE pb = pBuffer; pb < (pBuffer + ulBufferSize); ++pb)
    {
        ++nCnt;
        byteLine << L" " << HEX(*pb);
        if ((nCnt % 8) == 0)
        {
            byteLine << L"\r\n";
        }
    }

    DISPLAY(byteLine.str());
}

void DynamicIL::ShowMethod(CMethodInfo *pMethodInfo)
{
#if 0
    _ASSERTE(pMethodInfo != NULL);
    DISPLAY(L"Method: " << pMethodInfo->m_moduleID << " " << pMethodInfo->m_functionID << " " << pMethodInfo->FullSignature());
    //DISPLAY(L"Part of CER: " << (pMethodInfo->m_bIsPartOfCER == TRUE)? "Yes":"No");

    IMAGE_COR_ILMETHOD* pILMethod = (IMAGE_COR_ILMETHOD*)pMethodInfo->m_pMethodHeader;

    if (pMethodInfo->IsFatHeader())
    {
        DISPLAY(L"(Fat) Flags\t\t\t = " << HEX(pILMethod->Fat.Flags) << L"\r\n");
        DISPLAY(L"(Fat) Size\t\t\t = " << HEX(pILMethod->Fat.Size) << L"\r\n");
        DISPLAY(L"(Fat) MaxStack\t\t\t = " << HEX(pILMethod->Fat.MaxStack) << L"\r\n");
        DISPLAY(L"(Fat) CodeSize\t\t\t = " << HEX(pILMethod->Fat.CodeSize) << L"\r\n");
        DISPLAY(L"(Fat) LocalVarSigTok\t\t = " << HEX(pILMethod->Fat.LocalVarSigTok) << L"\r\n");
        ULONG cbMethodSizeWithHeader = pILMethod->Fat.CodeSize + HEADERSIZE_FAT;
        DISPLAY(L"Method size (including header)\t = " << HEX(cbMethodSizeWithHeader) << L"\r\n");
        DISPLAY(L"IL method body: " << (cbMethodSizeWithHeader - HEADERSIZE_FAT) << " Bytes \r\n");

        SEHWalker(pMethodInfo);

        _ASSERTE(cbMethodSizeWithHeader > HEADERSIZE_FAT);

        LPCBYTE pbILStart = (BYTE*)((BYTE*)pMethodInfo->m_pMethodHeader + HEADERSIZE_FAT);
        //Display only the IL Function Body so strip off the header

        DumpBytes(pbILStart, cbMethodSizeWithHeader - HEADERSIZE_FAT);
    }
    else
    {
        _ASSERTE(pMethodInfo->IsTinyHeader());
        DISPLAY(L"(Tiny) CodeSize\t\t\t = " << HEX(pMethodInfo->GetMethodCodeSize()) << L"\r\n");
        ULONG cbMethodSizeWithHeader = pMethodInfo->GetMethodCodeSize() + HEADERSIZE_TINY; //Tiny header is 1 Byte
        DISPLAY(L"Method size (including header)\t = " << HEX(cbMethodSizeWithHeader) << L"\r\n");
        DISPLAY(L"IL method body:\r\n");

        //Method should always be at least bigger than the header
        _ASSERTE(cbMethodSizeWithHeader > HEADERSIZE_TINY);
        //Tiny Method should never be bigger than 64Bytes
        if ((cbMethodSizeWithHeader > HEADERSIZE_TINY_MAX))

            _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_WNDW);
        _ASSERTE(!(cbMethodSizeWithHeader > HEADERSIZE_TINY_MAX));

        LPCBYTE pbILStart = (BYTE*)((BYTE*)pMethodInfo->m_pMethodHeader + HEADERSIZE_TINY);
        //Display only the IL Function Body so strip off the header
        DumpBytes(pbILStart, cbMethodSizeWithHeader - HEADERSIZE_TINY);
    }
#endif
}

mdAssemblyRef DynamicIL::GetMscorlibAssemblyRef(CModuleInfo* pModuleInfo)
{
    if (m_tkMscorlib != 0)
    {
        return m_tkMscorlib;
    }

    HRESULT hr = S_OK;
    COMPtrHolder<IMetaDataAssemblyImport> pMetaDataAssemblyImport;

    GetMetaData(pModuleInfo->m_moduleID,
        ofRead,
        IID_IMetaDataAssemblyImport,
        (IUnknown**)&pMetaDataAssemblyImport);

    mdAssemblyRef rAssemblyRefs[100];
    HCORENUM hEnum = NULL;
    ULONG pcTokens = 0;

    pMetaDataAssemblyImport->EnumAssemblyRefs(&hEnum,
        rAssemblyRefs,
        100,
        &pcTokens);

    PROFILER_WCHAR szName[MAX_PATH];
    PLATFORM_WCHAR szNameTemp[MAX_PATH];
    ULONG cchName = MAX_PATH;
    for (ULONG i = 0; i<pcTokens; i++)
    {
        hr = pMetaDataAssemblyImport->GetAssemblyRefProps(rAssemblyRefs[i],
            NULL,
            NULL,
            szName,
            cchName,
            &cchName,
            NULL,
            NULL,
            NULL,
            NULL);
        if (FAILED(hr))
        {
            FAILURE(L"FAILURE retrieving AssemblyRefProps\n");
        }
        ConvertProfilerWCharToPlatformWChar(szNameTemp, MAX_PATH, szName, MAX_PATH);
        if (_wcsicmp(szNameTemp, L"mscorlib") == 0)
        {
            DISPLAY(L"Found Mscorlib Assembly Ref \n");
            m_tkMscorlib = rAssemblyRefs[i];
            break;
        }
    }

    return m_tkMscorlib;
}


#pragma warning(pop)

#ifdef _MANAGED
#pragma managed(pop)
#endif
