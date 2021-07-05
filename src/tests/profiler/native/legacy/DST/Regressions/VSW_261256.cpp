#include "../../ProfilerCommon.h"
#include "../../LegacyCompat.h" // Whidbey Framework Support

INT VSW261256_fail;
INT VSW261256_pass;

HRESULT STDMETHODCALLTYPE VSW261256_SnapShotCallback( FunctionID funcId,
                                                    UINT_PTR ip,
                                                    COR_PRF_FRAME_INFO finfo,
                                                    ULONG32 contextSize,
                                                    BYTE context[],
                                                    void * clientData)
{
    IPrfCom *pPrfCom = (IPrfCom *)clientData;

    HRESULT hr = S_OK;
    COMPtrHolder<IMetaDataImport> pImport;
    mdToken token;

    hr = pPrfCom->m_pInfo->GetTokenAndMetaDataFromFunction(funcId,
                                                        IID_IMetaDataImport,
                                                        (IUnknown**)&pImport,
                                                        &token);
    if (FAILED(hr))
    {
        return S_OK;
    }

    DWORD implFlags;
    DWORD length;
    WCHAR funcName[STRING_LENGTH];

    pImport->GetMethodProps(token,
                        NULL,
                        funcName,
                        STRING_LENGTH,
                        &length,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &implFlags);
    if (FAILED(hr))
    {
		DISPLAY(L"GetMethodProps failed. Returned HR 0x" << HEX(hr) << L"\n");
        return S_OK;
    }

    if (implFlags & miInternalCall)
    {
        VSW261256_fail++;
        #ifdef UNICODE
            DISPLAY(funcName);
        #else
            DISPLAY(funcName);
        #endif
            FAILURE(L"SnapShotCallback received for ECall!\n");
        return hr;
    }
    else
    {
        VSW261256_pass++;
    }

    return S_OK;
}


HRESULT VSW261256_FunctionEnter2(IPrfCom * pPrfCom,
                 FunctionID mappedFuncId,
                 UINT_PTR clientData,
                 COR_PRF_FRAME_INFO func,
                 COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo)
{
    pPrfCom->m_pInfo->DoStackSnapshot(NULL,
                                        (StackSnapshotCallback *)&VSW261256_SnapShotCallback,
                                        COR_PRF_SNAPSHOT_DEFAULT,
                                        pPrfCom,
                                        NULL,
                                        NULL);
    return S_OK;
}


HRESULT VSW261256_ModuleLoadFinished(IPrfCom * pPrfCom,
                                    ModuleID moduleId,
                                    HRESULT hrStatus)
{
    DISPLAY( L"VSW261256_ModuleLoadFinished \n" );

    HRESULT hr = S_OK;
    DWORD length;
    PLATFORM_WCHAR modName[STRING_LENGTH];
    PROFILER_WCHAR modNameTemp[STRING_LENGTH];

    hr = pPrfCom->m_pInfo->GetModuleInfo(moduleId,
                                    NULL,
                                    STRING_LENGTH,
                                    &length,
                                    modNameTemp,
                                    NULL);

    ConvertProfilerWCharToPlatformWChar(modName, STRING_LENGTH, modNameTemp, STRING_LENGTH);
    if (SUCCEEDED(hr))
    {
        DISPLAY(modName);
    }

    hr = pPrfCom->m_pInfo->DoStackSnapshot(NULL,
                                    (StackSnapshotCallback *)&VSW261256_SnapShotCallback,
                                    COR_PRF_SNAPSHOT_DEFAULT,
                                    pPrfCom,
                                    NULL,
                                    NULL);
    // Don't check return value.
    return S_OK;
}

HRESULT VSW261256_Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"Verify VSW261256 - Profiling: DoStackSnapshot issues callbacks for ECalls\n");
    DISPLAY(L"ECall's Received " << VSW261256_fail << "\nSuccessfull DoStackSnpashot checks " << VSW261256_pass << L"\n");

    if( (VSW261256_fail != 0) || (VSW261256_pass == 0))
    {
        DISPLAY( L"TEST FAILED!\n" );
        return E_FAIL;
    }

    return S_OK;
}

void VSW_261256_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize VSW261256 extension\n") ;

    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_MODULE_LOADS |
                               COR_PRF_ENABLE_STACK_SNAPSHOT |
                               COR_PRF_MONITOR_ENTERLEAVE;

    pModuleMethodTable->VERIFY = (FC_VERIFY) &VSW261256_Verify;
    pModuleMethodTable->MODULELOADFINISHED = (FC_MODULELOADFINISHED) &VSW261256_ModuleLoadFinished;
    pModuleMethodTable->FUNCTIONENTER2 = (FC_FUNCTIONENTER2) &VSW261256_FunctionEnter2;

    VSW261256_fail = 0;
    VSW261256_pass = 0;

    return;
}
