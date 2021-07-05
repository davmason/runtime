#include "../../ProfilerCommon.h"
#include "../../LegacyCompat.h" // Whidbey Framework Support

#include "../../../holder.h"

INT VSW261255_fail;

HRESULT STDMETHODCALLTYPE VSW261255_SnapShotCallback( FunctionID funcId,
                                                    UINT_PTR ip,
                                                    COR_PRF_FRAME_INFO finfo,
                                                    ULONG32 contextSize,
                                                    BYTE context[],
                                                    void * clientData)
{
    IPrfCom *pPrfCom = (IPrfCom *)clientData;

    DISPLAY( L"\tSnapShotCallback :" );

    HRESULT hr = S_OK;
    ClassID classId;
    ModuleID moduleId;
    mdToken token;
    ULONG32 nTypeArgs;
    ClassID typeArgs[SHORT_LENGTH];

    hr = pPrfCom->m_pInfo->GetFunctionInfo2(funcId,
                                        finfo,
                                            &classId,
                                            &moduleId,
                                            &token,
                                            SHORT_LENGTH,
                                            &nTypeArgs,
                                            typeArgs);
    if (hr != S_OK)
    {
        DISPLAY( L" could not get func info\n" );
        return S_OK;
    }

    COMPtrHolder<IMetaDataImport> pIMDImport;

    hr = pPrfCom->m_pInfo->GetModuleMetaData(moduleId,
                                ofRead,
                                IID_IMetaDataImport,
                                (IUnknown **)&pIMDImport);
    if (hr != S_OK)
    {
        DISPLAY( L" could not get func info\n" );
        return S_OK;
    }

    mdTypeDef classToken;

    hr = pPrfCom->m_pInfo->GetClassIDInfo(classId, NULL, &classToken);

    if (hr != S_OK)
    {
        DISPLAY(L" could not get func info\n");
        return S_OK;
    }

    ULONG cchName;
    PLATFORM_WCHAR name[STRING_LENGTH];
    PROFILER_WCHAR nameTemp[STRING_LENGTH];

    hr = pIMDImport->GetTypeDefProps(classToken,
                                     nameTemp,
                                     STRING_LENGTH,
                                     &cchName,
                                     NULL,
                                     NULL);
    
    ConvertProfilerWCharToPlatformWChar(name, STRING_LENGTH, nameTemp, STRING_LENGTH);
    if (hr != S_OK)
    {
        DISPLAY(L" could not get func info\n");
        return S_OK;
    }

    DISPLAY(name);

    hr = pIMDImport->GetMethodProps(token,
                                    NULL,
                                    nameTemp,
                                    STRING_LENGTH,
                                    &cchName,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);

    ConvertProfilerWCharToPlatformWChar(name, STRING_LENGTH, nameTemp, STRING_LENGTH);
    if (hr != S_OK)
    {
        FAILURE(L"GetMethodProps returned 0x" << HEX(hr) << "\n");
        return S_OK;
    }

    DISPLAY(name);

    return S_OK;
}

HRESULT VSW261255_FuncEnter2Search(IPrfCom *pPrfCom,
                         FunctionID mappedFuncId,
                         UINT_PTR clientData,
                         COR_PRF_FRAME_INFO func,
                         COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo)
{
    HRESULT hr = S_OK;

    DISPLAY( L"VSW261255 FuncEnter2 : " );

    ClassID classId;
    ModuleID moduleId;
    mdToken token;
    ULONG32 nTypeArgs;
    ClassID typeArgs[SHORT_LENGTH];

    hr = pPrfCom->m_pInfo->GetFunctionInfo2(mappedFuncId,
                                    func,
                                        &classId,
                                        &moduleId,
                                        &token,
                                        SHORT_LENGTH,
                                        &nTypeArgs,
                                        typeArgs);
    if (hr != S_OK)
    {
        FAILURE(L"GetFunctionInfo2 returned 0x" << HEX(hr) << L"\n");
        return hr;
    }

    COMPtrHolder<IMetaDataImport> pIMDImport;

    hr = pPrfCom->m_pInfo->GetModuleMetaData(moduleId,
                                        ofRead,
                                        IID_IMetaDataImport,
                                        (IUnknown **)&pIMDImport);
    if (hr != S_OK)
    {
        FAILURE(L"GetModuleMetaData returned 0x" << HEX(hr) << L"\n");
        return hr;
    }

    mdTypeDef classToken;

    hr = pPrfCom->m_pInfo->GetClassIDInfo(classId, NULL, &classToken);

    if (hr != S_OK)
    {
        FAILURE(L"GetClassIDInfo returned 0x" << HEX(hr) << "\n");
        return hr;
    }

    ULONG cchName;
    PROFILER_WCHAR nameTemp[STRING_LENGTH];
    PLATFORM_WCHAR name[STRING_LENGTH];

    hr = pIMDImport->GetTypeDefProps(classToken,
                                     nameTemp,
                                     STRING_LENGTH,
                                     &cchName,
                                     NULL,
                                     NULL);

    ConvertProfilerWCharToPlatformWChar(name, STRING_LENGTH, nameTemp, STRING_LENGTH);

    if (hr != S_OK)
    {
        FAILURE(L"GetTypeDefProps returned 0x" << HEX(hr) << "\n");
        return hr;
    }

    DISPLAY(name);

    hr = pIMDImport->GetMethodProps(token,
                                    NULL,
                                    nameTemp,
                                    STRING_LENGTH,
                                    &cchName,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);
    ConvertProfilerWCharToPlatformWChar(name, STRING_LENGTH, nameTemp, STRING_LENGTH);

    if (hr != S_OK)
    {
        FAILURE(L"GetMethodProps returned 0x" << HEX(hr) << "\n");
        return hr;
    }

    DISPLAY(name);


    hr = pPrfCom->m_pInfo->DoStackSnapshot(NULL,
                                    (StackSnapshotCallback *)&VSW261255_SnapShotCallback,
                                    COR_PRF_SNAPSHOT_REGISTER_CONTEXT,
                                    pPrfCom,
                                    NULL,
                                    NULL);
    if (FAILED(hr))
    {
        FAILURE(L"DoStackSnapshot failed.  Returned HR 0x" << HEX(hr) << "\n");
        return hr;
    }


    return hr;
}

HRESULT VSW261255_Verify(IPrfCom * pPrfCom)
{
    DISPLAY( L"Verify VSW261255\n" );

    if (VSW261255_fail != 0)
    {
        FAILURE(L"");
        return E_FAIL;
    }

    return S_OK;

}

void VSW_261255_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize VSW261255 extension\n" );

    pModuleMethodTable->FLAGS =    COR_PRF_MONITOR_ENTERLEAVE |
                                COR_PRF_ENABLE_FUNCTION_ARGS |
                                COR_PRF_ENABLE_FUNCTION_RETVAL;

    pModuleMethodTable->VERIFY = (FC_VERIFY) &VSW261255_Verify;
    pModuleMethodTable->FUNCTIONENTER2 = (FC_FUNCTIONENTER2) &VSW261255_FuncEnter2Search;

    VSW261255_fail = 0;

    return;
}
