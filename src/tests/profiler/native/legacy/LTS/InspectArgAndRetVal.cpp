
#include "InspectArgAndRetVal.h"

static const mdToken g_tkCorEncodeToken[4] ={mdtTypeDef, mdtTypeRef, mdtTypeSpec, mdtBaseType};

InspectArgAndRetVal::InspectArgAndRetVal(IPrfCom * pPrfCom, 
                                         InspectionTestFlags testFlags)
{
    m_pPrfCom = pPrfCom;
    m_testFlags = testFlags;
    m_bMSCorLibFound = FALSE;
    mscorelibMduelId = NULL;

    m_retValIntSuccess              = 0;
    m_retValClassSuccess            = 0;
    m_retValClassStaticSuccess      = 0;
    m_retValStructSuccess           = 0;
    m_retValStructStaticSuccess     = 0;
    m_retValComplextStaticSuccess   = 0;
    m_retValGenericClassIntSuccess  = 0;
    m_retValGenericMethodIntSuccess = 0;
    m_argIntSuccess                 = 0;
    m_argClassSuccess               = 0;
    m_argClassStaticSuccess         = 0;
    m_argStructSuccess              = 0;
    m_argStructStaticSuccess        = 0;
    m_argComplexStaticSuccess       = 0;
    m_argGenericClassIntSuccess     = 0;
    m_argGenericMethodIntSuccess    = 0;
    m_allSuccess                    = 0;

    m_ulInspectedThisPointers       = 0;

    m_bInFunctionEnter = FALSE;
    m_bInFunctionLeave = FALSE;
    m_bBeginMethod     = FALSE;
    m_bBeginParams     = FALSE;
    m_bBeginRetType    = FALSE;

    systemObjectTypeDef = NULL;
    systemValueTypeTypeDef = NULL;

    //InitializeCriticalSectionAndSpinCount(&m_csParse, 0x1000);
}

InspectArgAndRetVal::~InspectArgAndRetVal()
{
}


HRESULT InspectArgAndRetVal::FunctionEnter3WithInfo(IPrfCom * pPrfCom, FunctionIDOrClientID functionIdOrClientID, COR_PRF_ELT_INFO eltInfo)
{
	lock_guard<mutex> guard(m_csParse);
	
	COR_PRF_FRAME_INFO frameInfo;
	ULONG pcbArgumentInfo = sizeof(COR_PRF_FUNCTION_ARGUMENT_INFO);
	COR_PRF_FUNCTION_ARGUMENT_INFO * pArgumentInfo = new COR_PRF_FUNCTION_ARGUMENT_INFO;
	HRESULT hr = pPrfCom->m_pInfo->GetFunctionEnter3Info(functionIdOrClientID.functionID, eltInfo, &frameInfo, &pcbArgumentInfo, pArgumentInfo);
	if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) && hr != S_OK)
	{
		delete pArgumentInfo;

		FAILURE("\nGetFunctionEnter3Info failed inside InspectArgAndRetVal::FunctionEnter3WithInfo");
		return hr;
	}
	else if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
	{
		delete pArgumentInfo;
		pArgumentInfo = reinterpret_cast <COR_PRF_FUNCTION_ARGUMENT_INFO *>  (new BYTE[pcbArgumentInfo]); 
		hr = pPrfCom->m_pInfo->GetFunctionEnter3Info(functionIdOrClientID.functionID, eltInfo, &frameInfo, &pcbArgumentInfo, pArgumentInfo);
		if(hr != S_OK)
		{
			delete (BYTE *)pArgumentInfo;
			FAILURE("\nGetFunctionEnter3Info failed inside InspectArgAndRetVal::FunctionEnter3WithInfo after setting the correct value of pcbArgumentInfo to" << pcbArgumentInfo);
			return E_FAIL;
		}
	}

	m_bInFunctionEnter = TRUE;

	DERIVED_COMMON_POINTER(ILTSCom);

	PLATFORM_WCHAR  wszMethodName[STRING_LENGTH];
	PROFILER_WCHAR  wszMethodNameTemp[STRING_LENGTH];
	ULONG           cwName = NULL;
	PCCOR_SIGNATURE pSig   = NULL;
	ULONG           cbSig  = NULL;

	ModuleID moduleId = NULL;
	mdToken  token    = NULL;
	ClassID  classId  = NULL;

	// Get the method signature blob
	COMPtrHolder<IMetaDataImport2> pIMDImport;
	MUST_PASS(PINFO->GetFunctionInfo2(functionIdOrClientID.functionID, frameInfo, &classId, &moduleId, &token, 0, NULL, NULL));
	MUST_PASS(PINFO->GetModuleMetaData(moduleId, ofRead, IID_IMetaDataImport2, (IUnknown **)&pIMDImport));
	MUST_PASS(pIMDImport->GetMethodProps(token, NULL, wszMethodNameTemp, STRING_LENGTH, &cwName, NULL, &pSig, &cbSig, NULL, NULL));

	// Setup variables for parse callbacks to use
	m_functionID = functionIdOrClientID.functionID;
	m_func = frameInfo;
	m_pArgumentInfo = pArgumentInfo;

	ConvertProfilerWCharToPlatformWChar(wszMethodName, STRING_LENGTH, wszMethodNameTemp, STRING_LENGTH);

	// Parse and inspect
	if ((m_testFlags & UNIT_TEST_ARG_INT) && STRING_EQUAL(L"IntValueTest", wszMethodName))
	{
		DISPLAY(L"\nUNIT_TEST_ARG_INT FunctionEnter3WithInfo testing " << wszMethodName);
		MUST_RETURN_VALUE(Parse((sig_byte *)pSig, cbSig), true);
	}
	if ((m_testFlags & UNIT_TEST_ARG_CLASS) && STRING_EQUAL(L"SimpleClassTest", wszMethodName))
	{
		DISPLAY(L"\nUNIT_TEST_ARG_CLASS FunctionEnter3WithInfo testing " << wszMethodName);
		MUST_RETURN_VALUE(Parse((sig_byte *)pSig, cbSig), true);
	}
	if ((m_testFlags & UNIT_TEST_ARG_CLASS_STATIC) && STRING_EQUAL(L"SimpleClassStaticsTest", wszMethodName))
	{
		DISPLAY(L"\nUNIT_TEST_ARG_CLASS_STATIC FunctionEnter3WithInfo testing " << wszMethodName);
		MUST_RETURN_VALUE(Parse((sig_byte *)pSig, cbSig), true);
	}
	if ((m_testFlags & UNIT_TEST_ARG_STRUCT) && STRING_EQUAL(L"SimpleStructTest", wszMethodName))
	{
		DISPLAY(L"\nUNIT_TEST_ARG_STRUCT FunctionEnter3WithInfo testing " << wszMethodName);
		MUST_RETURN_VALUE(Parse((sig_byte *)pSig, cbSig), true);
	}
	if ((m_testFlags & UNIT_TEST_ARG_STRUCT_STATIC) && STRING_EQUAL(L"SimpleStructStaticsTest", wszMethodName))
	{
		DISPLAY(L"\nUNIT_TEST_ARG_STRUCT_STATIC FunctionEnter3WithInfo testing " << wszMethodName);
		MUST_RETURN_VALUE(Parse((sig_byte *)pSig, cbSig), true);
	}
	if ((m_testFlags & UNIT_TEST_ARG_COMPLEX_STATIC) && STRING_EQUAL(L"ClassWithComplexStaticsTest", wszMethodName))
	{
		DISPLAY(L"\nUNIT_TEST_ARG_COMPLEX_STATIC FunctionEnter3WithInfo testing " << wszMethodName);
		MUST_RETURN_VALUE(Parse((sig_byte *)pSig, cbSig), true);
	}
	if ((m_testFlags & UNIT_TEST_ARG_GENERIC_CLASS_INT) && STRING_EQUAL(L"Foo", wszMethodName))
	{
		WCHAR_STR( fullName );
		PPRFCOM->GetClassIDName(classId, fullName, false);
		fullName.append(L"::").append(wszMethodName);
		if (fullName == L"GenericClass`1<System.Int32>::Foo")
		{
			DISPLAY(L"\nUNIT_TEST_ARG_GENERIC_CLASS_INT FunctionEnter3WithInfo testing " << fullName);
			MUST_RETURN_VALUE(Parse((sig_byte *)pSig, cbSig), true);
		}
	}
	else if ((m_testFlags & UNIT_TEST_ARG_GENERIC_METHOD_INT) && STRING_EQUAL(L"GenericFoo", wszMethodName))
	{
		WCHAR_STR( fullName );
		PPRFCOM->GetFunctionIDName(functionIdOrClientID.functionID, fullName, frameInfo, false);
		if (fullName == L"GenericFoo<System.Int32>")
		{
			DISPLAY(L"\nUNIT_TEST_ARG_GENERIC_METHOD_INT FunctionEnter3WithInfo testing " << fullName);
			MUST_RETURN_VALUE(Parse((sig_byte *)pSig, cbSig), true);
		}
	}

	m_bInFunctionEnter = FALSE;
	delete[] (BYTE *) pArgumentInfo;
	return S_OK;

}
                                        



HRESULT InspectArgAndRetVal::FunctionLeave3WithInfo(IPrfCom * pPrfCom, FunctionIDOrClientID functionIdOrClientID, COR_PRF_ELT_INFO eltInfo)
{
	lock_guard<mutex> guard(m_csParse);
	COR_PRF_FRAME_INFO frameInfo;
	COR_PRF_FUNCTION_ARGUMENT_RANGE * pRetvalRange = new COR_PRF_FUNCTION_ARGUMENT_RANGE;

	HRESULT hr = pPrfCom->m_pInfo->GetFunctionLeave3Info(functionIdOrClientID.functionID, eltInfo, &frameInfo, pRetvalRange);
	if (hr != HRESULT_FROM_WIN32(E_INVALIDARG) && hr != S_OK)
	{
		delete pRetvalRange;

		FAILURE("\nGetFunctionLeave3Info failed inside InspectArgAndRetVal::FunctionLeave3WithInfo");
		return hr;
	}
	else if(hr == HRESULT_FROM_WIN32(E_INVALIDARG))
	{
		delete pRetvalRange;

		FAILURE("\nInvalid functionId received in FunctionLeave3WithInfo");
		return hr;
	}

	
	m_bInFunctionLeave = TRUE;

	DERIVED_COMMON_POINTER(ILTSCom);

	PLATFORM_WCHAR  wszMethodName[STRING_LENGTH];
	PROFILER_WCHAR  wszMethodNameTemp[STRING_LENGTH];
	ULONG           cwName = NULL;
	PCCOR_SIGNATURE pSig   = NULL;
	ULONG           cbSig  = NULL;

	ModuleID moduleId = NULL;
	mdToken  token    = NULL;
	ClassID  classId  = NULL;

	// Get the method signature blob
	COMPtrHolder<IMetaDataImport2> pIMDImport;
	MUST_PASS(PINFO->GetFunctionInfo2(functionIdOrClientID.functionID, frameInfo, &classId, &moduleId, &token, 0, NULL, NULL));
	MUST_PASS(PINFO->GetModuleMetaData(moduleId, ofRead, IID_IMetaDataImport2, (IUnknown **)&pIMDImport));
	MUST_PASS(pIMDImport->GetMethodProps(token, NULL, wszMethodNameTemp, STRING_LENGTH, &cwName, NULL, &pSig, &cbSig, NULL, NULL));

	ConvertProfilerWCharToPlatformWChar(wszMethodName, STRING_LENGTH, wszMethodNameTemp, STRING_LENGTH);

	// Setup variables for parse callbacks to use
	m_functionID = functionIdOrClientID.functionID;
	m_func = frameInfo;
	m_pRetvalRange = pRetvalRange;

	// Parse and inspect
	if ((m_testFlags & UNIT_TEST_RETVAL_INT) && STRING_EQUAL(L"IntValueTest", wszMethodName))
	{
		DISPLAY(L"\nUNIT_TEST_RETVAL_INT FunctionLeave3WithInfo testing " << wszMethodName);
		MUST_RETURN_VALUE(Parse((sig_byte *)pSig, cbSig), true);
	}
	if ((m_testFlags & UNIT_TEST_RETVAL_CLASS) && STRING_EQUAL(L"SimpleClassTest", wszMethodName))
	{
		DISPLAY(L"\nUNIT_TEST_RETVAL_CLASS FunctionLeave3WithInfo testing " << wszMethodName);
		MUST_RETURN_VALUE(Parse((sig_byte *)pSig, cbSig), true);
	}
	if ((m_testFlags & UNIT_TEST_RETVAL_CLASS_STATIC) && STRING_EQUAL(L"SimpleClassStaticsTest", wszMethodName))
	{
		DISPLAY(L"\nUNIT_TEST_RETVAL_CLASS_STATIC FunctionLeave3WithInfo testing " << wszMethodName);
		MUST_RETURN_VALUE(Parse((sig_byte *)pSig, cbSig), true);
	}
	if ((m_testFlags & UNIT_TEST_RETVAL_STRUCT) && STRING_EQUAL(L"SimpleStructTest", wszMethodName))
	{
		DISPLAY(L"\nUNIT_TEST_RETVAL_STRUCT FunctionLeave3WithInfo testing " << wszMethodName);
		MUST_RETURN_VALUE(Parse((sig_byte *)pSig, cbSig), true);
	}
	if ((m_testFlags & UNIT_TEST_RETVAL_STRUCT_STATIC) && STRING_EQUAL(L"SimpleStructStaticsTest", wszMethodName))
	{
		DISPLAY(L"\nUNIT_TEST_RETVAL_STRUCT_STATIC FunctionLeave3WithInfo testing " << wszMethodName);
		MUST_RETURN_VALUE(Parse((sig_byte *)pSig, cbSig), true);
	}
	if ((m_testFlags & UNIT_TEST_RETVAL_COMPLEX_STATIC) && STRING_EQUAL(L"ClassWithComplexStaticsTest", wszMethodName))
	{
		DISPLAY(L"\nUNIT_TEST_RETVAL_COMPLEX_STATIC FunctionLeave3WithInfo testing " << wszMethodName);
		MUST_RETURN_VALUE(Parse((sig_byte *)pSig, cbSig), true);
	}
	if ((m_testFlags & UNIT_TEST_RETVAL_GENERIC_CLASS_INT) && STRING_EQUAL(L"Bar", wszMethodName))
	{
		WCHAR_STR( fullName );
		PPRFCOM->GetClassIDName(classId, fullName, false);
		fullName.append(L"::").append(wszMethodName);
		if (fullName == L"GenericClass`1<System.Int32>::Bar")
		{
			DISPLAY(L"\nUNIT_TEST_RETVAL_GENERIC_CLASS_INT FunctionLeave3WithInfo testing " << fullName);
			MUST_RETURN_VALUE(Parse((sig_byte *)pSig, cbSig), true);
		}
	}
	if ((m_testFlags & UNIT_TEST_RETVAL_GENERIC_METHOD_INT) && STRING_EQUAL(L"GenericBar", wszMethodName))
	{
		WCHAR_STR( fullName );
		PPRFCOM->GetFunctionIDName(functionIdOrClientID.functionID, fullName, frameInfo, false);
		if (fullName == L"GenericBar<System.Int32>")
		{
			DISPLAY(L"\nUNIT_TEST_RETVAL_GENERIC_METHOD_INT FunctionLeave3WithInfo testing " << fullName);
			MUST_RETURN_VALUE(Parse((sig_byte *)pSig, cbSig), true);
		}
	}

	m_bInFunctionLeave = FALSE;
	delete pRetvalRange;
	return S_OK;
}


HRESULT InspectArgAndRetVal::ModuleLoadFinished(IPrfCom * pPrfCom,
                                                ModuleID moduleId,
                                                HRESULT hrStatus)
{
    HRESULT hr     = S_OK;
    ULONG   cbName = NULL;
    PLATFORM_WCHAR moduleName[STRING_LENGTH];
    PROFILER_WCHAR moduleNameTemp[STRING_LENGTH];

    COMPtrHolder<IMetaDataImport> pIMDImport;

    hr = PINFO->GetModuleMetaData(moduleId,
                                  ofRead,
                                  IID_IMetaDataImport,
                                  (IUnknown **)&pIMDImport);
    if (FAILED(hr))
    {
        FAILURE(L"GetModuleMetaData returned hr=" << HEX(hr) << L" for ModuleID " << HEX(moduleId));
        return hr;
    }
    if (hr != S_OK)
    {
        return hr;
    }

    mdModule moduleToken;
    MUST_PASS(pIMDImport->GetModuleFromScope(&moduleToken));

    m_defToIdMap.insert(make_pair(moduleToken, moduleId));

    MUST_PASS(PINFO->GetModuleInfo(moduleId, 0, STRING_LENGTH, &cbName, moduleNameTemp, 0));
	ConvertProfilerWCharToPlatformWChar(moduleName, STRING_LENGTH, moduleNameTemp, STRING_LENGTH);
    //PLATFORM_WCHAR pwszShortName = *(wcsrchr(moduleName, '\\'));
    DISPLAY(L"\nModuleLoadFinished( Name=" << moduleName << L", ModuleID=" << HEX(moduleId ) << ", HR=" << HEX(hrStatus) << L" )");

    if ((FAILED(hrStatus)) || (m_bMSCorLibFound == TRUE))
    {
        return hrStatus;
    }

    if (IsMscorlib(moduleName))
    {
        DISPLAY(L"\tMSCorLib.dll found.");

        mscorelibMduelId = moduleId;

        mdTypeDef typeDefToken = NULL;

        MUST_PASS(pIMDImport->FindTypeDefByName(PROFILER_STRING("System.Boolean"), NULL, &typeDefToken));
        corTypeDefTokens[ELEMENT_TYPE_BOOLEAN] = typeDefToken;

        MUST_PASS(pIMDImport->FindTypeDefByName(PROFILER_STRING("System.Char"), NULL, &typeDefToken));
        corTypeDefTokens[ELEMENT_TYPE_CHAR] = typeDefToken;

        MUST_PASS(pIMDImport->FindTypeDefByName(PROFILER_STRING("System.SByte"), NULL, &typeDefToken));
        corTypeDefTokens[ELEMENT_TYPE_I1] = typeDefToken;

        MUST_PASS(pIMDImport->FindTypeDefByName(PROFILER_STRING("System.Byte"), NULL, &typeDefToken));
        corTypeDefTokens[ELEMENT_TYPE_U1] = typeDefToken;

        MUST_PASS(pIMDImport->FindTypeDefByName(PROFILER_STRING("System.Int16"), NULL, &typeDefToken));
        corTypeDefTokens[ELEMENT_TYPE_I2] = typeDefToken;

        MUST_PASS(pIMDImport->FindTypeDefByName(PROFILER_STRING("System.UInt16"), NULL, &typeDefToken));
        corTypeDefTokens[ELEMENT_TYPE_U2] = typeDefToken;

        MUST_PASS(pIMDImport->FindTypeDefByName(PROFILER_STRING("System.Int32"), NULL, &typeDefToken));
        corTypeDefTokens[ELEMENT_TYPE_I4] = typeDefToken;

        MUST_PASS(pIMDImport->FindTypeDefByName(PROFILER_STRING("System.UInt32"), NULL, &typeDefToken));
        corTypeDefTokens[ELEMENT_TYPE_U4] = typeDefToken;

        MUST_PASS(pIMDImport->FindTypeDefByName(PROFILER_STRING("System.Int64"), NULL, &typeDefToken));
        corTypeDefTokens[ELEMENT_TYPE_I8] = typeDefToken;

        MUST_PASS(pIMDImport->FindTypeDefByName(PROFILER_STRING("System.UInt64"), NULL, &typeDefToken));
        corTypeDefTokens[ELEMENT_TYPE_U8] = typeDefToken;

        MUST_PASS(pIMDImport->FindTypeDefByName(PROFILER_STRING("System.Single"), NULL, &typeDefToken));
        corTypeDefTokens[ELEMENT_TYPE_R4] = typeDefToken;

        MUST_PASS(pIMDImport->FindTypeDefByName(PROFILER_STRING("System.Double"), NULL, &typeDefToken));
        corTypeDefTokens[ELEMENT_TYPE_R8] = typeDefToken;

        MUST_PASS(pIMDImport->FindTypeDefByName(PROFILER_STRING("System.String"), NULL, &typeDefToken));
        corTypeDefTokens[ELEMENT_TYPE_STRING] = typeDefToken;

        MUST_PASS(pIMDImport->FindTypeDefByName(PROFILER_STRING("System.Object"), NULL, &typeDefToken));
        systemObjectTypeDef = typeDefToken;

        MUST_PASS(pIMDImport->FindTypeDefByName(PROFILER_STRING("System.ValueType"), NULL, &typeDefToken));
        systemValueTypeTypeDef = typeDefToken;
    
        m_bMSCorLibFound = TRUE;
        DISPLAY(L"\tFinished collecting type tokens.");

        // Disable module load events from now on.
        MUST_PASS(PPRFCOM->RemoveEvent(COR_PRF_MONITOR_MODULE_LOADS));
    }
    return hr;
}



void InspectArgAndRetVal::NotifyHasThis()
{
    PARSE_CALLBACK_VARIABLES;
    DISPLAY(L"\tNotifyHasThis");

    ClassID   classId       = NULL;
    ClassID   parentClassId = NULL;
    mdTypeDef classToken    = NULL;

    if (m_bInFunctionEnter == FALSE)
    {
        return;
    }
    else if (systemObjectTypeDef == 0 || systemValueTypeTypeDef == 0)
    {
        m_ulCurrentParam++;
        m_ulInspectedThisPointers++;
        return;
    }


    MUST_PASS(PINFO->GetFunctionInfo2(m_functionID, m_func, &classId, NULL, NULL, NULL, NULL, NULL));
    ModuleID moduleId = NULL;
    while(classId != 0)
    {
		
        MUST_PASS(PINFO->GetClassIDInfo2(classId, &moduleId, &classToken, &parentClassId, NULL, NULL, NULL));

        classId = parentClassId;
        if (moduleId == mscorelibMduelId && classToken == systemValueTypeTypeDef)
        {
            break;
        }
    }

    if (moduleId == mscorelibMduelId && classToken == systemObjectTypeDef)
    {
        ObjectID thisPointerObjectId = *(ObjectID *)m_pArgumentInfo->ranges[0].startAddress;
        PLTSCOM->InspectObjectID(thisPointerObjectId, classId, corTypeDefTokens, FALSE);
        m_ulInspectedThisPointers++;
    }
    else if (moduleId == mscorelibMduelId && classToken == systemValueTypeTypeDef)
    {
        // TODO: inspect the value type
        DISPLAY(L"\t this=systemValueTypeTypeDef");
    }
    else
    {
        FAILURE(L"Parent reported not System.Object or System.ValueType!");
    }

    // This is first param.  
    m_ulCurrentParam++;
}

void InspectArgAndRetVal::NotifyParamCount(sig_count count)
{ 
    PARSE_CALLBACK_VARIABLES; 
    DISPLAY(L"\tNotifyParamCount " << HEX(count)); 

    if (m_bInFunctionEnter && ((count + m_ulCurrentParam) < m_pArgumentInfo->numRanges))
    {
        FAILURE(L"Function has more ArgInfo->Ranges [" << HEX(m_pArgumentInfo->numRanges) << L"] than Parameters [" << HEX(count) << L"]");
    }
}


void InspectArgAndRetVal::NotifyTypeSimple(sig_elem_type  elem_type) 
{ 
    PARSE_CALLBACK_VARIABLES; 
    DISPLAY(L"\tNotifyTypeSimple " << HEX(elem_type)); 
    
    INT intParam = 0;
    PVOID offset = NULL;
    
    if (m_bBeginParams && m_bInFunctionEnter)
    {
        offset= (PVOID) m_pArgumentInfo->ranges[m_ulCurrentParam].startAddress;
        DISPLAY(L"\t\tCurrentParam=" << HEX(m_ulCurrentParam) << L", StartAddress=" << HEX(offset));
    }
    else if (m_bBeginRetType && m_bInFunctionLeave)
    {
        offset= (PVOID) m_pRetvalRange->startAddress;
        DISPLAY(L"\t\tRetVal StartAddress=" << HEX(offset));
    }
    else
    {
        return;
    }

    // Helper macros for extracting our values from raw data.   
    #define EXTRACT_INT(type)    (int)         ((type*)offset)[0]
    #define EXTRACT_UINT(type)   (unsigned int)((type*)offset)[0]
    #define EXTRACT_FLOAT(type)  (float)       ((type*)offset)[0]
    #define EXTRACT_DOUBLE(type) (double)      ((type*)offset)[0]

    switch(elem_type)
    {
        case ELEMENT_TYPE_VOID:                                       
            break;
        case ELEMENT_TYPE_BOOLEAN:                                    
            break;
        case ELEMENT_TYPE_CHAR:
            MUST_RETURN_VALUE(ValidatePtr(offset, sizeof(WCHAR)), true);
            DISPLAY(L"\t\tVal=" << HEX(EXTRACT_INT(WCHAR)));           
            break;
        case ELEMENT_TYPE_I1:      
            MUST_RETURN_VALUE(ValidatePtr(offset, sizeof(BYTE)), true);
            DISPLAY(L"\t\tVal=" << HEX(EXTRACT_INT(BYTE)));            
            break;
        case ELEMENT_TYPE_U1:      
            MUST_RETURN_VALUE(ValidatePtr(offset, sizeof(BYTE)), true);
            DISPLAY(L"\t\tVal=" << HEX(EXTRACT_UINT(BYTE)));           
            break;
        case ELEMENT_TYPE_I2:      
            MUST_RETURN_VALUE(ValidatePtr(offset, sizeof(WORD)), true);
            DISPLAY(L"\t\tVal=" << HEX(EXTRACT_INT(WORD)));            
            break;
        case ELEMENT_TYPE_U2:      
            MUST_RETURN_VALUE(ValidatePtr(offset, sizeof(WORD)), true);
            DISPLAY(L"\t\tVal=" << HEX(EXTRACT_UINT(WORD)));           
            break;
        case ELEMENT_TYPE_I4:      
            MUST_RETURN_VALUE(ValidatePtr(offset, sizeof(DWORD)), true);
            intParam = EXTRACT_INT(DWORD);
            DISPLAY(L"\t\tVal=" << HEX(intParam)); 

            if ((m_testFlags & UNIT_TEST_ARG_INT) && (m_bInFunctionEnter))
            {
                MUST_PASS(VerifyFieldValue(pPrfCom, L"Param", intParam, m_ulCurrentParam));
            }
            
            if ((m_testFlags & UNIT_TEST_RETVAL_INT) && (m_bInFunctionLeave))
            {
                MUST_PASS(VerifyFieldValue(pPrfCom, L"Param", intParam, 12345));
            }
            break;
        case ELEMENT_TYPE_U4:      
            MUST_RETURN_VALUE(ValidatePtr(offset, sizeof(DWORD)), true);
            DISPLAY(L"\t\tVal=" << HEX(EXTRACT_UINT(DWORD)));          
            break;
        case ELEMENT_TYPE_I8:      
            MUST_RETURN_VALUE(ValidatePtr(offset, sizeof(DWORD64)), true);
            DISPLAY(L"\t\tVal=" << HEX(EXTRACT_INT(DWORD64)));         
            break;
        case ELEMENT_TYPE_U8:      
            MUST_RETURN_VALUE(ValidatePtr(offset, sizeof(DWORD64)), true);
            DISPLAY(L"\t\tVal=" << HEX(EXTRACT_UINT(DWORD64)));        
            break;
        case ELEMENT_TYPE_R4:      
            MUST_RETURN_VALUE(ValidatePtr(offset, sizeof(DWORD)), true);
            DISPLAY(L"\t\tVal=" << HEX(EXTRACT_FLOAT(DWORD))); 
            break;
        case ELEMENT_TYPE_R8:      
            MUST_RETURN_VALUE(ValidatePtr(offset, sizeof(DWORD64)), true);
            DISPLAY(L"\t\tVal=" << HEX(EXTRACT_DOUBLE(DWORD64)));      
            break;
        default:
            FAILURE(L"Unknown primitive type " << HEX(elem_type));
            break;
    }

    if (m_bBeginParams && m_bInFunctionEnter)
    {
        m_ulCurrentParam++; 
    }
}

void InspectArgAndRetVal::NotifyTypeDefOrRef(sig_index_type  indexType, int index) 
{ 
    PARSE_CALLBACK_VARIABLES; 
    FieldValMap variables;

    mdToken token = TokenFromRid(index, g_tkCorEncodeToken[indexType]);
    DISPLAY(L"\tNotifyTypeDefOrRef : Type=" << HEX(indexType) << L", Index=" << HEX(index) << L", Token=" << HEX(token));

    if (m_bBeginParams && m_bInFunctionEnter)
    {
        if (m_testFlags & UNIT_TEST_ARG_CLASS)
        {
            variables.insert(make_pair(static_cast<wstring>(L"i"), 11));
            variables.insert(make_pair(static_cast<wstring>(L"j"), 22));
            variables.insert(make_pair(static_cast<wstring>(L"k"), 0));
            variables.insert(make_pair(static_cast<wstring>(L"l"), 0));
        }
        else if (m_testFlags & UNIT_TEST_ARG_CLASS_STATIC)
        {
            variables.insert(make_pair(static_cast<wstring>(L"i"), 11));
            variables.insert(make_pair(static_cast<wstring>(L"j"), 22));
            variables.insert(make_pair(static_cast<wstring>(L"k"), 33));
            variables.insert(make_pair(static_cast<wstring>(L"l"), 44));
        }
        else if (m_testFlags & UNIT_TEST_ARG_STRUCT)
        {
            variables.insert(make_pair(static_cast<wstring>(L"i"), 11));
            variables.insert(make_pair(static_cast<wstring>(L"j"), 22));
            variables.insert(make_pair(static_cast<wstring>(L"k"), 0));
            variables.insert(make_pair(static_cast<wstring>(L"l"), 0));
        }
        else if (m_testFlags & UNIT_TEST_ARG_STRUCT_STATIC)
        {
            variables.insert(make_pair(static_cast<wstring>(L"i"), 11));
            variables.insert(make_pair(static_cast<wstring>(L"j"), 22));
            variables.insert(make_pair(static_cast<wstring>(L"k"), 33));
            variables.insert(make_pair(static_cast<wstring>(L"l"), 44));
        }
        else if (m_testFlags & UNIT_TEST_ARG_COMPLEX_STATIC)
        {
            variables.insert(make_pair(static_cast<wstring>(L"local_i"), 100));
            variables.insert(make_pair(static_cast<wstring>(L"i"), 200));
            variables.insert(make_pair(static_cast<wstring>(L"i_thread_static"), 300));
            variables.insert(make_pair(static_cast<wstring>(L"i_context_static"), 400));
        }
    }
    else if (m_bBeginRetType && m_bInFunctionLeave)
    {
        if (m_testFlags & UNIT_TEST_RETVAL_CLASS)
        {
            variables.insert(make_pair(static_cast<wstring>(L"i"), 22));
            variables.insert(make_pair(static_cast<wstring>(L"j"), 44));
            variables.insert(make_pair(static_cast<wstring>(L"k"), 0));
            variables.insert(make_pair(static_cast<wstring>(L"l"), 0));
        }
        else if (m_testFlags & UNIT_TEST_RETVAL_CLASS_STATIC)
        {
            variables.insert(make_pair(static_cast<wstring>(L"i"), 22));
            variables.insert(make_pair(static_cast<wstring>(L"j"), 44));
            variables.insert(make_pair(static_cast<wstring>(L"k"), 66));
            variables.insert(make_pair(static_cast<wstring>(L"l"), 88));
        }
        else if (m_testFlags & UNIT_TEST_RETVAL_STRUCT)
        {
            variables.insert(make_pair(static_cast<wstring>(L"i"), 22));
            variables.insert(make_pair(static_cast<wstring>(L"j"), 44));
            variables.insert(make_pair(static_cast<wstring>(L"k"), 0));
            variables.insert(make_pair(static_cast<wstring>(L"l"), 0));
        }
        else if (m_testFlags & UNIT_TEST_RETVAL_STRUCT_STATIC)
        {
            variables.insert(make_pair(static_cast<wstring>(L"i"), 22));
            variables.insert(make_pair(static_cast<wstring>(L"j"), 44));
            variables.insert(make_pair(static_cast<wstring>(L"k"), 66));
            variables.insert(make_pair(static_cast<wstring>(L"l"), 88));
        }
        else if (m_testFlags & UNIT_TEST_RETVAL_COMPLEX_STATIC)
        {
            variables.insert(make_pair(static_cast<wstring>(L"local_i"), 1));
            variables.insert(make_pair(static_cast<wstring>(L"i"), 2));
            variables.insert(make_pair(static_cast<wstring>(L"i_thread_static"), 4));
            variables.insert(make_pair(static_cast<wstring>(L"i_context_static"), 8));
        }
    }

    if (variables.size() != 0)
    {
        if ((m_testFlags & TEST_WITH_CLASS) && (m_bIsClassType))
        {
            MUST_PASS(VerifyClass(m_pPrfCom, token, variables));
        }
        else if ((m_testFlags & TEST_WITH_STRUCT) && (m_bIsValueType))
        {
            MUST_PASS(VerifyStruct(m_pPrfCom, token, variables));
        }
    }
}

void InspectArgAndRetVal::NotifyTypeGenericTypeVariable(sig_mem_number number) 
{ 
    PARSE_CALLBACK_VARIABLES; 
    DISPLAY(L"\tNotifyTypeGenericTypeVariable [GenClass] " << HEX(number)); 
    
    if (m_bInFunctionEnter)
    {
        MUST_PASS(VerifyFieldValue(pPrfCom, L"GenericParam", *reinterpret_cast<INT *>(m_pArgumentInfo->ranges[m_ulCurrentParam++].startAddress), 100))
    }
    else if (m_bInFunctionLeave)
    {
        MUST_PASS(VerifyFieldValue(pPrfCom, L"GenericParam", *reinterpret_cast<INT *>(m_pRetvalRange->startAddress), 200))
    }
}


void InspectArgAndRetVal::NotifyGenericParamCount(sig_count count) 
{ 
    PARSE_CALLBACK_VARIABLES; 
    DISPLAY(L"\tNotifyGenericParamCount [GenMeth] " << HEX(count)); 

    if (m_bInFunctionEnter)
    {
        MUST_PASS(VerifyFieldValue(pPrfCom, L"GenericParam", *reinterpret_cast<INT *>(m_pArgumentInfo->ranges[m_ulCurrentParam++].startAddress), 600))
    }
    else if (m_bInFunctionLeave)
    {
        MUST_PASS(VerifyFieldValue(pPrfCom, L"GenericParam", *reinterpret_cast<INT *>(m_pRetvalRange->startAddress), 700))
    }
}


HRESULT InspectArgAndRetVal::VerifyStruct(IPrfCom * pPrfCom,
                                          mdToken token,
                                          FieldValMap& varList)
{
    ModuleID moduleId = NULL;
    COMPtrHolder<IMetaDataImport> pIMDImport;

    MUST_PASS(PINFO->GetFunctionInfo2(m_functionID, NULL, NULL, &moduleId, NULL, NULL, NULL, NULL));
    MUST_PASS(PINFO->GetModuleMetaData(moduleId, ofRead, IID_IMetaDataImport, reinterpret_cast<IUnknown **>(&pIMDImport)));

    if (TypeFromToken(token) == mdtTypeRef)
    {
        COMPtrHolder<IMetaDataImport> pNewIMDImport;
        mdTypeDef newToken = NULL;
        MUST_PASS(pIMDImport->ResolveTypeRef(token, IID_IMetaDataImport, (IUnknown**)&pNewIMDImport, &newToken));

        //pIMDImport = pNewIMDImport;

        mdModule moduleToken;
        MUST_PASS(pNewIMDImport->GetModuleFromScope(&moduleToken));

        ModuleDefToIDMap::iterator tokIter = m_defToIdMap.find(moduleToken);
        if(tokIter == m_defToIdMap.end())
        {
            FAILURE(L"Unknown metadata import: " << moduleToken);
            return E_FAIL;
        }
        
        moduleId = (*tokIter).second;
        token = newToken;
    }

    ClassID classId = NULL;
    MUST_PASS(PINFO->GetClassFromToken(moduleId, token, &classId));

    ObjectID objectId = GetObjectID();

    return InspectObjectFields(pPrfCom, token, moduleId, classId, objectId, varList);
}

HRESULT InspectArgAndRetVal::VerifyClass(IPrfCom * pPrfCom,
                                         mdToken token,
                                         FieldValMap& varList)
{
    ObjectID objectId = GetObjectID();

    ClassID classId = NULL;
    MUST_PASS(PINFO->GetClassFromObject(objectId, &classId));

    ModuleID moduleId = NULL;
    MUST_PASS(PINFO->GetFunctionInfo2(m_functionID, NULL, NULL, &moduleId, NULL, NULL, NULL, NULL));

    ModuleID classModule = NULL;
    mdToken classToken = NULL;
    MUST_PASS(PINFO->GetClassIDInfo2(classId, &classModule, &classToken, NULL, NULL, NULL, NULL));

    if (classModule != moduleId)
    {
        FAILURE(L"Modules do not match");
    }
    if (classToken != token)
    {
        FAILURE(L"Tokens do not match.  classToken=" << HEX(classToken) << L", token=" << HEX(token));
    }

    return InspectObjectFields(pPrfCom, classToken, classModule, classId, objectId, varList);
}

ObjectID InspectArgAndRetVal::GetObjectID()
{
    if (m_bInFunctionEnter)
    {
        if (m_bIsClassType == TRUE)
        {
            return *reinterpret_cast<ObjectID *>(m_pArgumentInfo->ranges[m_ulCurrentParam++].startAddress);
        }
        else if (m_bIsValueType == TRUE)
        {
            return m_pArgumentInfo->ranges[m_ulCurrentParam++].startAddress;
        }
    }
    else if (m_bInFunctionLeave)
    {
        if (m_bIsClassType == TRUE)
        {
            return *reinterpret_cast<ObjectID *>(m_pRetvalRange->startAddress);
        }
        else if (m_bIsValueType == TRUE)
        {
            return m_pRetvalRange->startAddress;
        }
    }

    return NULL;
}

HRESULT InspectArgAndRetVal::InspectObjectFields(IPrfCom * pPrfCom,
                                                 mdToken token,
                                                 ModuleID moduleId,
                                                 ClassID classId,
                                                 ObjectID objectId,
                                                 FieldValMap& varList)
{
    HRESULT hr = S_OK;
    DERIVED_COMMON_POINTER(ILTSCom);

    COMPtrHolder<IMetaDataImport> pIMDImport;
    MUST_PASS(PINFO->GetModuleMetaData(moduleId, ofRead, IID_IMetaDataImport, reinterpret_cast<IUnknown **>(&pIMDImport)));

    COR_FIELD_OFFSET rFieldOffset[SHORT_LENGTH];
    ULONG cFieldOffset = NULL;
    ULONG ulClassSize = NULL;
    MUST_PASS(PINFO->GetClassLayout(classId, rFieldOffset, SHORT_LENGTH, &cFieldOffset, &ulClassSize));

    HCORENUM hEnum = NULL;
    mdFieldDef fieldTokens[SHORT_LENGTH];
    ULONG cTokens = NULL;
    MUST_PASS(pIMDImport->EnumFields(&hEnum, token, fieldTokens, SHORT_LENGTH, &cTokens));

    FieldValMap::iterator iVars;
    ThreadID threadId = NULL;
    AppDomainID appdomainId = NULL;
    ContextID contextId = NULL;
    PVOID staticAddress = NULL;

    for (ULONG i = 0; i < cTokens; i++)
    {
        if (TypeFromToken(fieldTokens[i]) != mdtFieldDef)
        {
            FAILURE(L"IMetaDataImport::EnumFields returned a bad ridOfField.");
        }

        PLATFORM_WCHAR fieldName[SHORT_LENGTH];
        PROFILER_WCHAR fieldNameTemp[SHORT_LENGTH];
        ULONG ulNameLength = NULL;
        DWORD fieldAttribs = NULL;
        MUST_PASS(pIMDImport->GetFieldProps(fieldTokens[i], NULL, fieldNameTemp, SHORT_LENGTH, &ulNameLength, &fieldAttribs, NULL, NULL, NULL, NULL, NULL));

		ConvertProfilerWCharToPlatformWChar(fieldName, SHORT_LENGTH, fieldNameTemp, SHORT_LENGTH);

        UINT layoutIndex = 0;
        BOOL found = false;
        for (layoutIndex = 0; layoutIndex < cFieldOffset; layoutIndex++)
        {
            if (rFieldOffset[layoutIndex].ridOfField == fieldTokens[i])
            {
                found = true;
                break;
            }
        }

        INT data = NULL;
        if (found == TRUE)
        {
            if (IsFdStatic(fieldAttribs) == TRUE)
            {
                FAILURE(L"Static field reported by IMetaDataImport::EnumFields also reported by ICorProfilerInfo2::GetClassLayout");
                return E_FAIL;
            }

            data = *reinterpret_cast<PINT>(objectId + rFieldOffset[layoutIndex].ulOffset);
        }
        else
        {
            if (IsFdStatic(fieldAttribs) == FALSE)
            {
                FAILURE(L"Non-Static field reported by IMetaDataImport::EnumFields but not by ICorProfilerInfo2::GetClassLayout");
                return E_FAIL;
            }

            staticAddress = NULL;
            hr = PLTSCOM->GetStaticFieldAddress(pIMDImport, classId, fieldTokens[i], fieldAttribs, &threadId, &contextId, &appdomainId, &staticAddress);
            if (hr == CORPROF_E_DATAINCOMPLETE || hr == CORPROF_E_LITERALS_HAVE_NO_ADDRESS)
            {
                DISPLAY(L"\t" << fieldName << L":HR=" << HEX(hr));
                continue;
            }
            else
            {
                data = *reinterpret_cast<PINT>(staticAddress);
            }
        }
        DISPLAY(L"\t" << fieldName << L"=" << data);

        iVars = varList.find(fieldName);
        if (iVars != varList.end())
        {
            MUST_PASS(VerifyFieldValue(pPrfCom, fieldName, data, iVars->second));
            varList.erase(iVars);
        }
        else
        {
            DISPLAY(L"\t" << fieldName << L"=" << data << L" - Not on verification list");
        }
    }

    if (varList.size() != 0)
    {
        for (iVars = varList.begin(); iVars != varList.end(); iVars++)
        {
            DISPLAY(L"Unchecked Variable - " << iVars->first << L" = " << iVars->second);
        }
        FAILURE(L"Unchecked variables, look at output for list.");
        return E_FAIL;
    }

    return S_OK;
}


HRESULT InspectArgAndRetVal::VerifyFieldValue(IPrfCom * pPrfCom,
                                              const PLATFORM_WCHAR * fieldName,
                                              INT fieldVal,
                                              INT expectedVal)
{
    if (fieldVal == expectedVal)
    {
        DISPLAY(L"\t" << fieldName << L"=" << fieldVal << L" - Verified");

        if (m_testFlags & UNIT_TEST_RETVAL_INT)                 m_retValIntSuccess++;
        if (m_testFlags & UNIT_TEST_RETVAL_CLASS)               m_retValClassSuccess++;
        if (m_testFlags & UNIT_TEST_RETVAL_CLASS_STATIC)        m_retValClassStaticSuccess++;
        if (m_testFlags & UNIT_TEST_RETVAL_STRUCT)              m_retValStructSuccess++;
        if (m_testFlags & UNIT_TEST_RETVAL_STRUCT_STATIC)       m_retValStructStaticSuccess++;
        if (m_testFlags & UNIT_TEST_RETVAL_COMPLEX_STATIC)      m_retValComplextStaticSuccess++;
        if (m_testFlags & UNIT_TEST_RETVAL_GENERIC_CLASS_INT)   m_retValGenericClassIntSuccess++;
        if (m_testFlags & UNIT_TEST_RETVAL_GENERIC_METHOD_INT)  m_retValGenericMethodIntSuccess++;
        if (m_testFlags & UNIT_TEST_ARG_INT)                    m_argIntSuccess++;
        if (m_testFlags & UNIT_TEST_ARG_CLASS)                  m_argClassSuccess++;
        if (m_testFlags & UNIT_TEST_ARG_CLASS_STATIC)           m_argClassStaticSuccess++;
        if (m_testFlags & UNIT_TEST_ARG_STRUCT)                 m_argStructSuccess++;
        if (m_testFlags & UNIT_TEST_ARG_STRUCT_STATIC)          m_argStructStaticSuccess++;
        if (m_testFlags & UNIT_TEST_ARG_COMPLEX_STATIC)         m_argComplexStaticSuccess++;
        if (m_testFlags & UNIT_TEST_ARG_GENERIC_CLASS_INT)      m_argGenericClassIntSuccess++;
        if (m_testFlags & UNIT_TEST_ARG_GENERIC_METHOD_INT)     m_argGenericMethodIntSuccess++;

        return S_OK;
    }
    else
    {
        FAILURE(L"Expected " <<  fieldName << L" = " << expectedVal << L", not " << fieldVal);
        return E_FAIL;
    }
}

//
// Verify the different unit and scenario test cases this extension is used for
//
HRESULT InspectArgAndRetVal::Verify(IPrfCom * pPrfCom)
{
    // Verify different unit and scenario test cases tested by this extension.

#pragma region Lots of Verification

    DISPLAY(L"\nVerify InspectArgAndRetVal");

    if (m_testFlags & UNIT_TEST_ARG_INT)
    {
        if ((m_argIntSuccess != 3) || (m_ulInspectedThisPointers != 1))
        {
            FAILURE(L"UNIT_TEST_ARG_INT Failed.\n\n\tTestVariable[Expected|Actual]\n"
                    << L"\n\tm_argIntSuccess[3|" << m_argIntSuccess << "]"
                    << L"\n\tm_ulInspectedThisPointers[1|" << m_ulInspectedThisPointers << "]");
        }
        else
        {
            DISPLAY(L"UNIT_TEST_ARG_INT Succeeded.");
        }
    }

    if (m_testFlags & UNIT_TEST_ARG_STRUCT)
    {
        if ((m_argStructSuccess != 4) || (m_ulInspectedThisPointers != 0))
        {
            FAILURE(L"UNIT_TEST_ARG_STRUCT Failed.\n\n\tTestVariable[Expected|Actual]\n"
                    << L"\n\tm_argStructSuccess[4|" << m_argStructSuccess << "]"
                    << L"\n\tm_ulInspectedThisPointers[0|" << m_ulInspectedThisPointers << "]");
        }
        else
        {
            DISPLAY(L"UNIT_TEST_ARG_STRUCT Succeeded.");
        }
    }

    if (m_testFlags & UNIT_TEST_ARG_STRUCT_STATIC)
    {
        if ((m_argStructStaticSuccess != 4) || (m_ulInspectedThisPointers != 0))
        {
            FAILURE(L"UNIT_TEST_ARG_STRUCT_STATIC Failed.\n\n\tTestVariable[Expected|Actual]\n"
                    << L"\n\tm_argStructStaticSuccess[4|" << m_argStructStaticSuccess << "]"
                    << L"\n\tm_ulInspectedThisPointers[0|" << m_ulInspectedThisPointers << "]");
        }
        else
        {
            DISPLAY(L"UNIT_TEST_ARG_STRUCT_STATIC Succeeded.");
        }
    }

    if (m_testFlags & UNIT_TEST_ARG_CLASS)
    {
        if ((m_argClassSuccess != 4) || (m_ulInspectedThisPointers != 0))
        {
            FAILURE(L"UNIT_TEST_ARG_CLASS Failed.\n\n\tTestVariable[Expected|Actual]\n"
                    << L"\n\tm_argClassSuccess[2|" << m_argClassSuccess << "]"
                    << L"\n\tm_ulInspectedThisPointers[0|" << m_ulInspectedThisPointers << "]");
        }
        else
        {
            DISPLAY(L"UNIT_TEST_ARG_CLASS Succeeded.");
        }
    }

    if (m_testFlags & UNIT_TEST_ARG_CLASS_STATIC)
    {
        if ((m_argClassStaticSuccess != 4)|| (m_ulInspectedThisPointers != 0))
        {
            FAILURE(L"UNIT_TEST_ARG_COMPLEX_STATIC Failed.\n\n\tTestVariable[Expected|Actual]\n"
                    << L"\n\tm_argClassStaticSuccess[4|" << m_argClassStaticSuccess << "]"
                    << L"\n\tm_ulInspectedThisPointers[0|" << m_ulInspectedThisPointers << "]");
        }
        else
        {
            DISPLAY(L"UNIT_TEST_ARG_CLASS_STATIC Succeeded.");
        }
    }

    if (m_testFlags & UNIT_TEST_ARG_COMPLEX_STATIC)
    {
        if ((m_argComplexStaticSuccess != 4) || (m_ulInspectedThisPointers != 1))
        {
            FAILURE(L"UNIT_TEST_ARG_COMPLEX_STATIC Failed.\n\n\tTestVariable[Expected|Actual]\n"
                    << L"\n\tm_argComplexStaticSuccess[4|" << m_argComplexStaticSuccess << "]"
                    << L"\n\tm_ulInspectedThisPointers[1|" << m_ulInspectedThisPointers << "]");
        }
        else
        {
            DISPLAY(L"UNIT_TEST_ARG_COMPLEX_STATIC Succeeded!");
        }
    }

    if (m_testFlags & UNIT_TEST_RETVAL_INT)
    {
        if ((m_retValIntSuccess != 1) || (m_ulInspectedThisPointers != 0))
        {
            FAILURE(L"UNIT_TEST_RETVAL_INT Failed.\n\n\tTestVariable[Expected|Actual]\n"
                    << L"\n\tm_retValIntSuccess[1|" << m_retValIntSuccess << "]"
                    << L"\n\tm_ulInspectedThisPointers[0|" << m_ulInspectedThisPointers << "]");
        }
        else
        {
            DISPLAY(L"UNIT_TEST_RETVAL_INT Succeeded.");
        }
    }

    if (m_testFlags & UNIT_TEST_RETVAL_STRUCT)
    {
        if ((m_retValStructSuccess != 4) || (m_ulInspectedThisPointers != 0))
        {
            FAILURE(L"UNIT_TEST_RETVAL_STRUCT Failed.\n\n\tTestVariable[Expected|Actual]\n"
                    << L"\n\tm_retValStructSuccess[4|" << m_retValStructSuccess << "]"
                    << L"\n\tm_ulInspectedThisPointers[0|" << m_ulInspectedThisPointers << "]");
        }
        else
        {
            DISPLAY(L"UNIT_TEST_RETVAL_STRUCT Succeeded.");
        }
    }

    if (m_testFlags & UNIT_TEST_RETVAL_STRUCT_STATIC)
    {
        if ((m_retValStructStaticSuccess != 4) || (m_ulInspectedThisPointers != 0))
        {
            FAILURE(L"UNIT_TEST_RETVAL_STRUCT_STATIC Failed.\n\n\tTestVariable[Expected|Actual]\n"
                    << L"\n\tm_retValStructStaticSuccess[4|" << m_retValStructStaticSuccess << "]"
                    << L"\n\tm_ulInspectedThisPointers[0|" << m_ulInspectedThisPointers << "]");
        }
        else
        {
            DISPLAY(L"UNIT_TEST_RETVAL_STRUCT_STATIC Succeeded.");
        }
    }

    if (m_testFlags & UNIT_TEST_RETVAL_CLASS)
    {
        if ((m_retValClassSuccess != 4) || (m_ulInspectedThisPointers != 0))
        {
            FAILURE(L"UNIT_TEST_RETVAL_CLASS Failed.\n\n\tTestVariable[Expected|Actual]\n"
                    << L"\n\tm_retValClassSuccess[4|" << m_retValClassSuccess << "]"
                    << L"\n\tm_ulInspectedThisPointers[0|" << m_ulInspectedThisPointers << "]");
        }
        else
        {
            DISPLAY(L"UNIT_TEST_RETVAL_CLASS Succeeded.");
        }
    }

    if (m_testFlags & UNIT_TEST_RETVAL_CLASS_STATIC)
    {
        if ((m_retValClassStaticSuccess != 4) || (m_ulInspectedThisPointers != 0))
        {
            FAILURE(L"UNIT_TEST_RETVAL_CLASS_STATIC Failed.\n\n\tTestVariable[Expected|Actual]\n"
                    << L"\n\tm_retValClassStaticSuccess[2|" << m_retValClassStaticSuccess << "]"
                    << L"\n\tm_ulInspectedThisPointers[0|" << m_ulInspectedThisPointers << "]");
        }
        else
        {
            DISPLAY(L"UNIT_TEST_RETVAL_CLASS_STATIC Succeeded.");
        }
    }

    if (m_testFlags & UNIT_TEST_RETVAL_COMPLEX_STATIC)
    {
        if ((m_retValComplextStaticSuccess != 4) || (m_ulInspectedThisPointers != 0))
        {
            FAILURE(L"UNIT_TEST_RETVAL_COMPLEX_STATIC Failed.\n\n\tTestVariable[Expected|Actual]\n"
                    << L"\n\tm_retValComplextStaticSuccess[4|" << m_retValComplextStaticSuccess << "]"
                    << L"\n\tm_ulInspectedThisPointers[0|" << m_ulInspectedThisPointers << "]");
        }
        else
        {
            DISPLAY(L"UNIT_TEST_RETVAL_COMPLEX_STATIC Succeeded.");
        }
    }

    if (m_testFlags & UNIT_TEST_ARG_GENERIC_CLASS_INT)
    {
        if ((m_argGenericClassIntSuccess != 1) || (m_ulInspectedThisPointers != 1))
        {
            FAILURE(L"UNIT_TEST_ARG_GENERIC_CLASS_INT Failed.\n\n\tTestVariable[Expected|Actual]\n"
                    << L"\n\tm_argGenericClassIntSuccess[1|" << m_argGenericClassIntSuccess << "]"
                    << L"\n\tm_ulInspectedThisPointers[1|" << m_ulInspectedThisPointers << "]");
        }
        else
        {
            DISPLAY(L"UNIT_TEST_ARG_GENERIC_CLASS_INT Succeeded.");
        }
    }

    if (m_testFlags & UNIT_TEST_RETVAL_GENERIC_CLASS_INT)
    {
        if ((m_retValGenericClassIntSuccess != 2) || (m_ulInspectedThisPointers != 0))
        {
            FAILURE(L"UNIT_TEST_RETVAL_GENERIC_CLASS_INT Failed.\n\n\tTestVariable[Expected|Actual]\n"
                    << L"\n\tm_retValGenericClassIntSuccess[2|" << m_retValGenericClassIntSuccess << "]"
                    << L"\n\tm_ulInspectedThisPointers[0|" << m_ulInspectedThisPointers << "]");
        }
        else
        {
            DISPLAY(L"UNIT_TEST_RETVAL_GENERIC_CLASS_INT Succeeded.");
        }
    }

    if (m_testFlags & UNIT_TEST_ARG_GENERIC_METHOD_INT)
    {
        if ((m_argGenericMethodIntSuccess != 1) || (m_ulInspectedThisPointers != 0))
        {
            FAILURE(L"UNIT_TEST_ARG_GENERIC_METHOD_INT Failed.\n\n\tTestVariable[Expected|Actual]\n"
                    << L"\n\tm_argGenericMethodIntSuccess[1|" << m_argGenericMethodIntSuccess << "]"
                    << L"\n\tm_ulInspectedThisPointers[0|" << m_ulInspectedThisPointers << "]");
        }
        else
        {
            DISPLAY(L"UNIT_TEST_ARG_GENERIC_METHOD_INT Succeeded!");
        }
    }

    if (m_testFlags & UNIT_TEST_RETVAL_GENERIC_METHOD_INT)
    {
        if ((m_retValGenericMethodIntSuccess != 1) || (m_ulInspectedThisPointers != 0))
        {
            FAILURE(L"UNIT_TEST_ARG_GENERIC_METHOD_INT Failed.\n\n\tTestVariable[Expected|Actual]\n"
                    << L"\n\tm_retValGenericMethodIntSuccess[1|" << m_retValGenericMethodIntSuccess << "]"
                    << L"\n\tm_ulInspectedThisPointers[0|" << m_ulInspectedThisPointers << "]");
        }
        else
        {
            DISPLAY(L"UNIT_TEST_RETVAL_GENERIC_METHOD_INT Succeeded!");
        }
    }

    DISPLAY(L"Verification complete.");

#pragma endregion // Lots of Verification

    return S_OK;
}


/*
 *  Straight function callback used by TestProfiler
 */
HRESULT InspectArgAndRetVal_Verify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(InspectArgAndRetVal);
    HRESULT hr = pInspectArgAndRetVal->Verify(pPrfCom);

    // Clean up instance of test class
    delete pInspectArgAndRetVal;
    pPrfCom->m_pTestClassInstance = NULL;

    return hr;
}


void InspectArgAndRetVal_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, InspectionTestFlags testFlags)
{
    DISPLAY(L"Initialize InspectArgAndRetVal extension with InspectionTestFlags=" << HEX(testFlags));

    // Create and save an instance of test class
    SET_CLASS_POINTER(new InspectArgAndRetVal(pPrfCom, testFlags));

    // Set the COR_PRF flags for this run.  We want funtion enter/leave/tail with full info for inspection
    static_cast<InspectArgAndRetVal *>(pModuleMethodTable->TEST_POINTER)->m_dwCorPrfFlags = 
                                                                                      COR_PRF_MONITOR_ENTERLEAVE     |
                                                                                      COR_PRF_MONITOR_MODULE_LOADS   |
                                                                                      COR_PRF_ENABLE_FUNCTION_ARGS   |
                                                                                      COR_PRF_ENABLE_FUNCTION_RETVAL |
                                                                                      COR_PRF_ENABLE_FRAME_INFO      |
                                                                                      COR_PRF_MONITOR_MODULE_LOADS   |
                                                                                      COR_PRF_USE_PROFILE_IMAGES     |
                                                                                      COR_PRF_ENABLE_FRAME_INFO      |
                                                                                      COR_PRF_DISABLE_INLINING       |
                                                                                      COR_PRF_DISABLE_OPTIMIZATIONS  ;

    pModuleMethodTable->FLAGS = static_cast<InspectArgAndRetVal *>(pModuleMethodTable->TEST_POINTER)->m_dwCorPrfFlags;

    // Set up callback for TestProfiler to use to hook into our test
    REGISTER_CALLBACK(VERIFY, InspectArgAndRetVal_Verify);
    REGISTER_CALLBACK(MODULELOADFINISHED, InspectArgAndRetVal::ModuleLoadFinishedWrapper);

    // Only hook FunctionEnter3WithInfo for tests that look for function argument values
    if (testFlags & TEST_ARGS)
    {
        REGISTER_CALLBACK(FUNCTIONENTER3WITHINFO, InspectArgAndRetVal::FunctionEnter3WithInfoWrapper);
    }

    // Only hook FunctionLeave3WithInfo for tests that look for function return values
    if (testFlags & TEST_RETVAL)
    {
        REGISTER_CALLBACK(FUNCTIONLEAVE3WITHINFO, InspectArgAndRetVal::FunctionLeave3WithInfoWrapper);
    }

    return;
}
