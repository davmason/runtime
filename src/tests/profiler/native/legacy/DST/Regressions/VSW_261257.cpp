#include "ProfilerCommon.h"
#include "LegacyCompat.h" // Whidbey Framework Support

INT VSW261257_fail;

HRESULT VSW261257_ModuleLoadFinished(IPrfCom * pPrfCom,
                                    ModuleID moduleId,
                                    HRESULT hrStatus)
{
    DISPLAY( L"VSW261257_ModuleLoadFinished " );

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

    if(FAILED(hr))
    {
        FAILURE(L"Failed to get GetModuleInfo: " << HEX(hr));
        return S_OK;
    }
    DISPLAY(modName);
    wstring name(modName);
    if (name.find(L"mscorlib.dll") == string::npos)
    {
        // We're only interested in mscorlib for this test.  Return if we are not at that module load.
        return S_OK;
    }

    DISPLAY( L"MSCorLib found.\n" );
    COMPtrHolder<IMetaDataImport> pImport;

    hr = pPrfCom->m_pInfo->GetModuleMetaData(moduleId,
                                            0,
                                            IID_IMetaDataImport,
                                            (IUnknown**)&pImport);
    if(FAILED(hr))
    {
        FAILURE(L"Failed to get GetModuleMetaData: " << HEX(hr));
        return S_OK;
    }
      HCORENUM typeEnum = 0;
       mdToken typeDef;
       ULONG numTypeDefs;

          while (true)
          {
              hr = pImport->EnumTypeDefs(&typeEnum,
                                &typeDef,
                                1,
                                &numTypeDefs);
              if (numTypeDefs != 1)
                  break;

               HCORENUM methodEnum = 0;
              while (true)
              {
                  mdToken methodDef;
                     ULONG numMethodDefs;
                     hr = pImport->EnumMethods(&methodEnum,typeDef,&methodDef,1,&numMethodDefs);
                     if (numMethodDefs != 1)
                         break;

            DWORD implFlags = 0;
                     hr = pImport->GetMethodProps(methodDef,NULL,NULL,0,NULL,NULL,NULL,NULL,NULL,&implFlags);
                     if (implFlags & miInternalCall)
                     {
                         FunctionID id;
                            LPCBYTE start;
                            ULONG size;
                            hr = pPrfCom->m_pInfo->GetFunctionFromToken(moduleId, methodDef, &id);
                            if (SUCCEEDED(hr))
                            {
                                hr = pPrfCom->m_pInfo->GetCodeInfo(id,&start,&size);
                                    //printf("GetCodeInfo returned 0x%x\n",hr);
                            }
                  }
              }
    }

    return S_OK;
}

HRESULT VSW261257_Verify(IPrfCom * pPrfCom)
{
    DISPLAY( L"Verify VSW261257\n" );

    if (VSW261257_fail != 0)
    {
        FAILURE( L"VSW261257_fail != 0" );
        return E_FAIL;
    }

    return S_OK;

}

void VSW_261257_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize VSW261257 extension\n" );

    pModuleMethodTable->FLAGS =     COR_PRF_MONITOR_MODULE_LOADS;

    pModuleMethodTable->VERIFY = (FC_VERIFY) &VSW261257_Verify;
    pModuleMethodTable->MODULELOADFINISHED = (FC_MODULELOADFINISHED) &VSW261257_ModuleLoadFinished;

    VSW261257_fail = 0;

    return;
}
