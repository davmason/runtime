#include "ProfilerCommon.h"

// Legacy support
#include "LegacyCompat.h"
// Legacy support

INT gGetClassRoundTrip_error;
INT gGetClassRoundTrip_incomplete;
INT gGetClassRoundTrip_success;

HRESULT GetClassRoundTrip_RoundTrip(IPrfCom * pPrfCom, ClassID classId)
{
    HRESULT hr;
    ClassID newClassId;
    ClassID parentClassId;
    ModuleID moduleId;
    mdTypeDef typeDefToken;
    UINT numTypeArgs;
    ClassID typeArgs[SHORT_LENGTH];

    WCHAR_STR( className );

    hr = pPrfCom->m_pInfo->GetClassIDInfo2(classId,
                                      &moduleId,
                                       &typeDefToken,
                                       &parentClassId,
                                      SHORT_LENGTH,
                                      &numTypeArgs,
                                      typeArgs);

    if (hr == CORPROF_E_DATAINCOMPLETE)
    {
        gGetClassRoundTrip_incomplete++;
		DISPLAY(L"GetClassIDInfo2 returned CORPROF_E_DATAINCOMPLETE\n");
        return S_OK;
    }
    else if (hr == E_INVALIDARG)
    {
        if (PINFO->IsArrayClass(classId,
                                            NULL,
                                            NULL,
                                            NULL) == S_FALSE)
        {
            gGetClassRoundTrip_error ++;
			FAILURE(L"GetClassIDInfo2 for ClassID 0x" << HEX(classId) << L" returned E_INVALIDARG and IsArrayClass returned S_FALSE\n");
            return S_FALSE;
        }
        else
        {
            // we have an array.  can't round trip
            return S_OK;
        }
    }
    else if (hr != S_OK)
    {
        gGetClassRoundTrip_error ++;
		FAILURE(L"GetClassIDInfo2 for ClassID 0x" << HEX(classId) << L" returned _FAILURE_ HR 0x" << HEX(hr) << L"\n");
        return S_FALSE;
    }

    hr = pPrfCom->m_pInfo->GetClassFromTokenAndTypeArgs(moduleId,
                                                         typeDefToken,
                                                         numTypeArgs,
                                                        typeArgs,
                                                         &newClassId);

    if (hr == CORPROF_E_DATAINCOMPLETE)
    {
        gGetClassRoundTrip_incomplete++;
        DISPLAY( L"GetClassFromTokenAndTypeArgs returned CORPROF_E_DATAINCOMPLETE\n" );
        return S_OK;
    }
    if (hr != S_OK)
    {
        gGetClassRoundTrip_error++;
		FAILURE(L"GetClassFromTokenAndTypeArgs returned _FAILURE_ HR 0x" << HEX(hr) << L"\n");
        return S_FALSE;
    }

    if (classId != newClassId)
    {
        gGetClassRoundTrip_error++;
        PPRFCOM->GetClassIDName(classId, className);
		DISPLAY(L"Origional Class " << className << L" - 0x" << HEX(classId) << L"\n");

        PPRFCOM->GetClassIDName(newClassId, className);
		DISPLAY(L"New Class " << className << L" - 0x" << HEX(newClassId) << L"\n");

		FAILURE(L"Round trip failed.  classId != newClassId.  0x << " << HEX(classId) << L" != 0x " << HEX(newClassId) << L"\n");
        return S_FALSE;
    }

    gGetClassRoundTrip_success++;

    // Round trip the Parent ClassID
    if (parentClassId != 0)
    {
		if (GetClassRoundTrip_RoundTrip(pPrfCom, parentClassId) != S_OK)
			FAILURE(L"_FAILURE_ in ClassLoadFinished for ClassID 0x" << HEX(classId) << L" Parent ClassID 0x" << HEX(parentClassId) << L"\n");
    }

    // Round trip all type args
    for (ULONG32 i = 0; i < numTypeArgs; i++)
    {
		if (GetClassRoundTrip_RoundTrip(pPrfCom, typeArgs[i]) != S_OK)
			FAILURE(L"_FAILURE_ in ClassLoadFinished for ClassID 0x" << HEX(classId) << " Type Arg ClassID 0x" << HEX(typeArgs[i]) << "\n");
    }

    return S_OK;

}



HRESULT GetClassRoundTrip_ClassLoadStarted(IPrfCom * pPrfCom,
                        ClassID classId)
{
	if (GetClassRoundTrip_RoundTrip(pPrfCom, classId) != S_OK)
		FAILURE(L"_FAILURE_ in ClassLoadStarted for ClassID 0x" << HEX(classId) << L"\n");
    return S_OK;
}

HRESULT GetClassRoundTrip_ClassLoadFinished(IPrfCom * pPrfCom,
                        ClassID classId,
                        HRESULT /*hrStatus*/)
{
	if (GetClassRoundTrip_RoundTrip(pPrfCom, classId) != S_OK)
		FAILURE(L"_FAILURE_ in ClassLoadFinished for ClassID 0x" << HEX(classId) << L"\n");
    return S_OK;
}


HRESULT GetClassRoundTrip_ClassUnloadStarted(IPrfCom * pPrfCom,
                        ClassID classId)
{
	if (GetClassRoundTrip_RoundTrip(pPrfCom, classId) != S_OK)
		FAILURE(L"_FAILURE_ in ClassUnloadStarted for ClassID 0x" << HEX(classId) << L"\n");

    return S_OK;
}

HRESULT GetClassRoundTrip_ClassUnloadFinished(IPrfCom * pPrfCom,
                        ClassID classId,
                        HRESULT /*hrStatus*/)
{
	if (GetClassRoundTrip_RoundTrip(pPrfCom, classId) != S_OK)
		FAILURE(L"_FAILURE_ in ClassUnloadFinished for ClassID 0x" << HEX(classId) << L"\n");

    return S_OK;
}


HRESULT GetClassRoundTrip_Verify(IPrfCom * pPrfCom)
{
	DISPLAY(L"Verify GetClassRoundTrip callbacks\n");
	DISPLAY(L"Success " << gGetClassRoundTrip_success << L", Incomplete " << gGetClassRoundTrip_incomplete << L", _FAILURE_ " << gGetClassRoundTrip_error);

    if (gGetClassRoundTrip_error != 0)
    {
		FAILURE(L"Errors encountered in run! " << gGetClassRoundTrip_error << L"\n");
        return E_FAIL;
    }
    return S_OK;
}

void GetClassRoundTrip_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY( L"Initialize GetClassRoundTrip module\n" );

    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_CLASS_LOADS |
                                                    COR_PRF_USE_PROFILE_IMAGES;

    pModuleMethodTable->VERIFY = (FC_VERIFY) &GetClassRoundTrip_Verify;
    pModuleMethodTable->CLASSLOADSTARTED = (FC_CLASSLOADSTARTED) &GetClassRoundTrip_ClassLoadStarted;
    pModuleMethodTable->CLASSLOADFINISHED = (FC_CLASSLOADFINISHED) &GetClassRoundTrip_ClassLoadFinished;
    pModuleMethodTable->CLASSUNLOADSTARTED = (FC_CLASSUNLOADSTARTED) &GetClassRoundTrip_ClassUnloadStarted;
    pModuleMethodTable->CLASSUNLOADFINISHED = (FC_CLASSUNLOADFINISHED) &GetClassRoundTrip_ClassUnloadFinished;

    gGetClassRoundTrip_error = 0;
    gGetClassRoundTrip_incomplete = 0;
    gGetClassRoundTrip_success = 0;

    return;
}




