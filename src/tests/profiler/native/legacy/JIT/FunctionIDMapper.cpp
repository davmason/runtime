#include "JITCommon.h"

class CLRFunctionInfo
{
 public:
    CLRFunctionInfo(FunctionID funcID);
    ~CLRFunctionInfo();

    FunctionID GetFunctionID();

    // called whenever FuncIDMapper is called on the function
    VOID SetFunctionHooked(BOOL hooked);
    BOOL IsHooked() {return isHooked;}

 private:

    INT onStack;
    BOOL isNgened;
    BOOL isHooked;
    FunctionID funcID;
};

CLRFunctionInfo::CLRFunctionInfo(FunctionID funcID) :
      onStack(0),
      isNgened(false),
      isHooked(false),
      funcID(funcID)
{
}

CLRFunctionInfo::~CLRFunctionInfo()
{
}

FunctionID CLRFunctionInfo::GetFunctionID()
{
    return this->funcID;
}

void CLRFunctionInfo::SetFunctionHooked(BOOL hooked)
{
    this->isHooked = hooked;
}

class FuncIDMapper
{
    public:

    FuncIDMapper(IPrfCom * pPrfCom);
    ~FuncIDMapper();

    HRESULT FuncEnter2(IPrfCom * pPrfCom,
                    FunctionID mappedFuncId,
                    UINT_PTR clientData,
                    COR_PRF_FRAME_INFO func,
                    COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo);

    static HRESULT FuncEnter2Wrapper(IPrfCom * pPrfCom,
                    FunctionID mappedFuncId,
                    UINT_PTR clientData,
                    COR_PRF_FRAME_INFO func,
                    COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo)
    {
        return Instance()->FuncEnter2(pPrfCom, mappedFuncId, clientData, func, argumentInfo);
    }

    HRESULT FuncTailcall2(IPrfCom * pPrfCom,
                    FunctionID mappedFuncId,
                    UINT_PTR clientData,
                    COR_PRF_FRAME_INFO func);

    static HRESULT FuncTailcall2Wrapper(IPrfCom * pPrfCom,
                    FunctionID mappedFuncId,
                    UINT_PTR clientData,
                    COR_PRF_FRAME_INFO func)
    {
        return Instance()->FuncTailcall2(pPrfCom, mappedFuncId, clientData, func);
    }

    HRESULT FuncLeave2(IPrfCom * pPrfCom,
                    FunctionID mappedFuncId,
                    UINT_PTR clientData,
                    COR_PRF_FRAME_INFO func,
                    COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange);

    static HRESULT FuncLeave2Wrapper(IPrfCom * pPrfCom,
                    FunctionID mappedFuncId,
                    UINT_PTR clientData,
                    COR_PRF_FRAME_INFO func,
                    COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange)
    {
        return Instance()->FuncLeave2(pPrfCom, mappedFuncId, clientData, func, retvalRange);
    }

	static HRESULT FunctionEnter3Wrapper(IPrfCom * pPrfCom,
                    FunctionIDOrClientID functionIdOrClientID)

    {
        return Instance()->FunctionEnter3(pPrfCom, functionIdOrClientID);
    }
	
	static HRESULT FunctionLeave3Wrapper(IPrfCom * pPrfCom,
                    FunctionIDOrClientID functionIdOrClientID)

    {
        return Instance()->FunctionLeave3(pPrfCom, functionIdOrClientID);
    }

	static HRESULT FunctionTailCall3Wrapper(IPrfCom * pPrfCom,
                    FunctionIDOrClientID functionIdOrClientID)

    {
        return Instance()->FunctionTailCall3(pPrfCom, functionIdOrClientID);
    }

	static HRESULT FunctionEnter3WithInfoWrapper(IPrfCom * pPrfCom,
                    FunctionIDOrClientID functionIdOrClientID, COR_PRF_ELT_INFO eltInfo)

    {
        return Instance()->FunctionEnter3WithInfo(pPrfCom, functionIdOrClientID, eltInfo);
    }

	static HRESULT FunctionLeave3WithInfoWrapper(IPrfCom * pPrfCom,
                    FunctionIDOrClientID functionIdOrClientID, COR_PRF_ELT_INFO eltInfo)

    {
        return Instance()->FunctionLeave3WithInfo(pPrfCom, functionIdOrClientID, eltInfo);
    }

	static HRESULT FunctionTailCall3WithInfoWrapper(IPrfCom * pPrfCom,
                    FunctionIDOrClientID functionIdOrClientID, COR_PRF_ELT_INFO eltInfo)

    {
        return Instance()->FunctionTailCall3WithInfo(pPrfCom, functionIdOrClientID, eltInfo);
    }

    UINT_PTR FunctionMapper(IPrfCom * pPrfCom,
                    FunctionID funcId,
                    BOOL *pbHookFunction );

	UINT_PTR FunctionIDMapper2(IPrfCom * pPrfCom,
		FunctionID funcId,
		void * clientData,
		BOOL *pbHookFunction );

	static UINT_PTR __stdcall FunctionMapperWrapper( FunctionID funcId, BOOL *pbHookFunction )
	{
		return Instance()->FunctionMapper(Instance()->m_pPrfCom, funcId, pbHookFunction);
	}

	static UINT_PTR __stdcall FunctionIDMapper2Wrapper( FunctionID funcId, void *clientData, BOOL *pbHookFunction)
	{
		FuncIDMapper *  pFuncIdmapper = reinterpret_cast<FuncIDMapper *>(clientData);
		return pFuncIdmapper->FunctionMapper(pFuncIdmapper->m_pPrfCom, funcId, pbHookFunction);
	}

	HRESULT FunctionEnter3(IPrfCom * pPrfCom,
		FunctionIDOrClientID functionIdOrClientID)
	{
		return VerifyValidClientID(pPrfCom, functionIdOrClientID);
	}

	HRESULT FunctionLeave3(IPrfCom * pPrfCom,
		FunctionIDOrClientID functionIdOrClientID)
	{
		return VerifyValidClientID(pPrfCom, functionIdOrClientID);
	}

	HRESULT FunctionTailCall3(IPrfCom * pPrfCom,
		FunctionIDOrClientID functionIdOrClientID)
	{
		return VerifyValidClientID(pPrfCom, functionIdOrClientID);
	}

	HRESULT FunctionEnter3WithInfo(IPrfCom * pPrfCom,
		FunctionIDOrClientID functionIdOrClientID, COR_PRF_ELT_INFO /*eltInfo*/)
	{
		return VerifyValidClientID(pPrfCom, functionIdOrClientID);
	}

	HRESULT FunctionLeave3WithInfo(IPrfCom * pPrfCom,
		FunctionIDOrClientID functionIdOrClientID, COR_PRF_ELT_INFO /*eltInfo*/)
	{
		return VerifyValidClientID(pPrfCom, functionIdOrClientID);
	}

	HRESULT FunctionTailCall3WithInfo(IPrfCom * pPrfCom,
		FunctionIDOrClientID functionIdOrClientID, COR_PRF_ELT_INFO /*eltInfo*/)
	{
		return VerifyValidClientID(pPrfCom, functionIdOrClientID);
	}

    HRESULT VerifyFunctionIDMapping(IPrfCom * pPrfCom, FunctionID funcId, UINT_PTR clientData);

	HRESULT VerifyValidClientID(IPrfCom * pPrfCom, FunctionIDOrClientID functionIdOrClientID);

    HRESULT Verify(IPrfCom * pPrfCom);

    static FuncIDMapper* Instance()
    {
        return This;
    }

    static FuncIDMapper* This;

    std::mutex m_criticalSection;

    typedef std::map<FunctionID,CLRFunctionInfo *> FuncInfos;
    FuncInfos m_funcInfos;

    IPrfCom *m_pPrfCom;

    ULONG m_nSuccess;

};





FuncIDMapper* FuncIDMapper::This = NULL;

FuncIDMapper::FuncIDMapper(IPrfCom * pPrfCom)
{
    // Static this pointer
    FuncIDMapper::This = this;

    m_pPrfCom = pPrfCom;

    m_nSuccess = 0;
}

FuncIDMapper::~FuncIDMapper()
{
    // Static this pointer
    FuncIDMapper::This = NULL;
}

HRESULT FuncIDMapper::FuncEnter2(IPrfCom * pPrfCom,
                    FunctionID mappedFuncId,
                    UINT_PTR clientData,
                    COR_PRF_FRAME_INFO /*func*/,
                    COR_PRF_FUNCTION_ARGUMENT_INFO * /*argumentInfo*/)
{
    return VerifyFunctionIDMapping(pPrfCom, mappedFuncId, clientData);
}



HRESULT FuncIDMapper::FuncTailcall2(IPrfCom * pPrfCom,
                    FunctionID mappedFuncId,
                    UINT_PTR clientData,
                    COR_PRF_FRAME_INFO /*func*/)
{
    return VerifyFunctionIDMapping(pPrfCom, mappedFuncId, clientData);
}


HRESULT FuncIDMapper::FuncLeave2(IPrfCom * pPrfCom,
                    FunctionID mappedFuncId,
                    UINT_PTR clientData,
                    COR_PRF_FRAME_INFO /*func*/,
                    COR_PRF_FUNCTION_ARGUMENT_RANGE * /*retvalRange*/)
{
    return VerifyFunctionIDMapping(pPrfCom, mappedFuncId, clientData);
}



UINT_PTR FuncIDMapper::FunctionMapper(IPrfCom * pPrfCom,
                    FunctionID funcId,
                    BOOL *pbHookFunction )
{
    lock_guard<mutex> guard(m_criticalSection);

    *pbHookFunction = TRUE;

    FuncInfos::iterator i = this->m_funcInfos.find(funcId);
    CLRFunctionInfo * fi;
    if( i==this->m_funcInfos.end() )
    {
        // function not found!
        fi = new CLRFunctionInfo(funcId);
        this->m_funcInfos.insert( make_pair(funcId,fi));
    }
    else
    {
        fi = (i->second);
    }

    fi->SetFunctionHooked(*pbHookFunction);

    DISPLAY(L"\nmapping " << HEX(funcId) << L"-->" << HEX(fi));

    return reinterpret_cast<UINT_PTR>(fi);
}

HRESULT FuncIDMapper::VerifyFunctionIDMapping(IPrfCom * pPrfCom, FunctionID funcId, UINT_PTR clientData)
{
    lock_guard<mutex> guard(m_criticalSection);

    HRESULT hr = S_OK;

    // Make sure we have good data...
    if ((funcId == 0) || (clientData == 0))
    {
        FAILURE(L"\nNULL Arg passed to VerifyFunctionIDMapping: FunctionID = " << HEX(funcId) << L" clientData = " << HEX(clientData));
        hr = E_FAIL;
    }

    // Mapped ID should be address of CLRFunctionInfo structure
    CLRFunctionInfo * fi = reinterpret_cast<CLRFunctionInfo*>(clientData);

    // Make sure Origional ID in mapping == ID passed to FunctionEnter/Leave/Tailcall events
    if (fi->GetFunctionID() != funcId)
    {
        FAILURE(L"\nCLRFunctionInfo->FunctionID " << HEX(fi->GetFunctionID()) << L" != Mapped FunctionID " << HEX(funcId));
        hr = E_FAIL;
    }

    // Look up CLRFunctionInfo in map of Info to make sure it == Mapped ID
    FuncInfos::iterator i = this->m_funcInfos.find(funcId);
    if( i==this->m_funcInfos.end() )
    {
        // function not found!
        FAILURE(L"FuncEnter2 original FunctionID not found in FunctionInfo table\n");
        hr = E_FAIL;
    }
    else
    {
        if(i->second != fi)
        {
            FAILURE(L"FunctionInfo in map does not equal mapped funcion id\n");
            hr = E_FAIL;
        }
    }

    m_nSuccess++;

    return hr;
}

HRESULT FuncIDMapper::VerifyValidClientID(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID)
{
	HRESULT hr = S_OK;
	UINT_PTR clientData = functionIDOrClientID.clientID;
	if (0 == clientData)
	{
		FAILURE(L"\nNULL Arg passed to VerifyValidClientID");
		return E_FAIL;
	}
	CLRFunctionInfo * fi = reinterpret_cast<CLRFunctionInfo*>(clientData);
	FunctionID funcID = fi->GetFunctionID();
	FuncInfos::iterator i = this->m_funcInfos.find(funcID);
	if( i==this->m_funcInfos.end() )
	{
		FAILURE(L"\nClientID passed to VerifyValidClientID not found in  FunctionMapper");
		return E_FAIL;
	}
	else
	{
		m_nSuccess++;
		// successfuly found client ID FunctionID mapping.
	}
	return hr;
}

HRESULT FuncIDMapper::Verify(IPrfCom * pPrfCom)
{
    DISPLAY( L"Verify FuncIDMapper::Verify callbacks\n" );

    {
        lock_guard<mutex> guard(m_criticalSection);
        FreeAndEraseMap(m_funcInfos);
    }

    if (m_nSuccess == 0)
    {
        FAILURE(L"No successfull mappings took place\n");
        return E_FAIL;
    }
    else
    {
        DISPLAY(m_nSuccess << L" Successfully verified mappings\n");
    }

    return S_OK;
}

/*
 *  Straight function callback used by TestProfiler
 */

FuncIDMapper* global_FuncIDMapper;

HRESULT FuncIDMapper_Verify(IPrfCom * pPrfCom)
{
    HRESULT hr = FuncIDMapper::Instance()->Verify(pPrfCom);

    delete global_FuncIDMapper;

    return hr;
}

HRESULT FuncIDMapper2_Verify(IPrfCom * pPrfCom)
{
	FuncIDMapper * funcIdMapper = reinterpret_cast<FuncIDMapper * >(pPrfCom->m_pTestClassInstance);
	HRESULT hr = funcIdMapper->Verify(pPrfCom);
	delete funcIdMapper;
	return hr;
}

void FuncIDMapper_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Profiler API Test: FuncIDMapper with EnterLeave2 Function Events\n");

    global_FuncIDMapper = new FuncIDMapper(pPrfCom);
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_ENTERLEAVE;
    pModuleMethodTable->VERIFY = (FC_VERIFY) &FuncIDMapper_Verify;
    pModuleMethodTable->FUNCTIONIDMAPPER = (FC_FUNCTIONIDMAPPER) &FuncIDMapper::FunctionMapperWrapper;
    pModuleMethodTable->FUNCTIONENTER2 = (FC_FUNCTIONENTER2) &FuncIDMapper::FuncEnter2Wrapper;
    pModuleMethodTable->FUNCTIONTAILCALL2 = (FC_FUNCTIONTAILCALL2) &FuncIDMapper::FuncTailcall2Wrapper;
    pModuleMethodTable->FUNCTIONLEAVE2 = (FC_FUNCTIONLEAVE2) &FuncIDMapper::FuncLeave2Wrapper;
}

void FuncIDMapperELT3_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, bool fastPath)
{
	DISPLAY(L"Profiler API Test: FunctionIDMapper with EnterLeave3 Function Events\n");

	global_FuncIDMapper = new FuncIDMapper(pPrfCom);

	pModuleMethodTable->VERIFY = (FC_VERIFY) &FuncIDMapper_Verify;
	pModuleMethodTable->FUNCTIONIDMAPPER = (FC_FUNCTIONIDMAPPER) &FuncIDMapper::FunctionMapperWrapper;
	if(fastPath)
	{
		pModuleMethodTable->FLAGS = COR_PRF_MONITOR_ENTERLEAVE;
		pModuleMethodTable->FUNCTIONENTER3 = (FC_FUNCTIONENTER3) &FuncIDMapper::FunctionEnter3Wrapper;
		pModuleMethodTable->FUNCTIONTAILCALL3 = (FC_FUNCTIONTAILCALL3) &FuncIDMapper::FunctionTailCall3Wrapper;
		pModuleMethodTable->FUNCTIONLEAVE3 = (FC_FUNCTIONLEAVE3) &FuncIDMapper::FunctionLeave3Wrapper;
	}
	else
	{
		pModuleMethodTable->FLAGS = COR_PRF_MONITOR_ENTERLEAVE | COR_PRF_ENABLE_FUNCTION_ARGS | COR_PRF_ENABLE_FUNCTION_RETVAL | COR_PRF_ENABLE_FRAME_INFO; 
		pModuleMethodTable->FUNCTIONENTER3WITHINFO = (FC_FUNCTIONENTER3WITHINFO) &FuncIDMapper::FunctionEnter3WithInfoWrapper;
		pModuleMethodTable->FUNCTIONLEAVE3WITHINFO = (FC_FUNCTIONLEAVE3WITHINFO) &FuncIDMapper::FunctionLeave3WithInfoWrapper;
		pModuleMethodTable->FUNCTIONTAILCALL3WITHINFO = (FC_FUNCTIONTAILCALL3WITHINFO) &FuncIDMapper::FunctionTailCall3WithInfoWrapper;
	}
}

void FuncIDMapper2_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, bool fastPath)
{
	DISPLAY(L"Profiler API Test: FuncIDMapper2 with EnterLeave3 Function Events\n");
	global_FuncIDMapper = new FuncIDMapper(pPrfCom);

	pModuleMethodTable->TEST_POINTER = reinterpret_cast<void *>(global_FuncIDMapper);
	pModuleMethodTable->VERIFY = (FC_VERIFY) &FuncIDMapper2_Verify;
	pModuleMethodTable->FUNCTIONIDMAPPER2 = (FC_FUNCTIONIDMAPPER2) &FuncIDMapper::FunctionIDMapper2Wrapper;
	if(fastPath)
	{
		pModuleMethodTable->FLAGS = COR_PRF_MONITOR_ENTERLEAVE;// set event mask for fast path
		pModuleMethodTable->FUNCTIONENTER3 = (FC_FUNCTIONENTER3) &FuncIDMapper::FunctionEnter3Wrapper;
		pModuleMethodTable->FUNCTIONLEAVE3 = (FC_FUNCTIONLEAVE3) &FuncIDMapper::FunctionLeave3Wrapper;
		pModuleMethodTable->FUNCTIONTAILCALL3 = (FC_FUNCTIONTAILCALL3) &FuncIDMapper::FunctionTailCall3Wrapper;
	}
	else
	{
		// set event mask for slow path
		pModuleMethodTable->FLAGS = COR_PRF_MONITOR_ENTERLEAVE | COR_PRF_ENABLE_FUNCTION_ARGS | COR_PRF_ENABLE_FUNCTION_RETVAL | COR_PRF_ENABLE_FRAME_INFO; 
		pModuleMethodTable->FUNCTIONENTER3WITHINFO = (FC_FUNCTIONENTER3WITHINFO) &FuncIDMapper::FunctionEnter3WithInfoWrapper;
		pModuleMethodTable->FUNCTIONLEAVE3WITHINFO = (FC_FUNCTIONLEAVE3WITHINFO) &FuncIDMapper::FunctionLeave3WithInfoWrapper;
		pModuleMethodTable->FUNCTIONTAILCALL3WITHINFO = (FC_FUNCTIONTAILCALL3WITHINFO) &FuncIDMapper::FunctionTailCall3WithInfoWrapper;
	}
}