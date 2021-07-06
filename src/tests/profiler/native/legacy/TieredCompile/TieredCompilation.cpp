// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "../ProfilerCommon.h"

using std::mutex;
using std::lock_guard;
using std::vector;
using std::tuple;

class TieredCompilationVerification
{
public:
	TieredCompilationVerification(IPrfCom * pPrfCom, BOOL reJit)
	{
        m_pPrfCom = pPrfCom;
        m_jitCount = 0;
        m_verifySuccessfulCount = 0;
        m_verifyFailed = FALSE;
        m_targetFuncName = L"MethodThatWillBeVersioned";
    }
	~TieredCompilationVerification()
	{
		TieredCompilationVerification::This = NULL;
	}

    static const int NumStartAddresses = 2;

    IPrfCom *m_pPrfCom;
    mutex m_criticalSection;
    atomic<LONG> m_jitCount;
    atomic<LONG> m_verifySuccessfulCount;
    BOOL m_verifyFailed;
    wstring m_targetFuncName;

    HRESULT JITCompilationFinished(IPrfCom * pPrfCom, FunctionID functionID, HRESULT hrStatus, BOOL fIsSafeToBlock)
    {
        wstring funcName;
        HRESULT hr = pPrfCom->GetFunctionIDName(functionID, funcName);
        if (FAILED(hr))
        {
            FAILURE(L"GetFunctionIDName failed with hr=0x" << hex << hr);
            return hr;
        }

        if (funcName == m_targetFuncName)
        {
            ++m_jitCount;

            CallVerification(pPrfCom, functionID, NULL);
        }

        return S_OK;
    }

    /*
     * Called on every JIT event. Validates that the number of code addresses returned is
     * the same as the number of jit events received for the method. This is an implementation
     * detail and will require revisiting if we ever start collecting dead code.
     *
     * Also calls GetIlToNativeMapping3 and GetCodeInfo4 for each native code address, and
     * verifys that the calls are successful.
     *
     */
    HRESULT CallVerification(IPrfCom * pPrfCom, FunctionID functionID, ReJITID rejitID)
    {
        COMPtrHolder<ICorProfilerInfo9> pInfo;
        HRESULT hr = pPrfCom->m_pInfo->QueryInterface(IID_ICorProfilerInfo9, (void**)&pInfo);
        if (FAILED(hr))
        {
            FAILURE(L"QueryInterface() for ICorProfilerInfo9 failed.");
            return E_FAIL;
        }

        ULONG32 cCodeStartAddresses;
        ULONG32 cCountReturned;
        UINT_PTR codeStartAddresses[NumStartAddresses];

        hr = pInfo->GetNativeCodeStartAddresses(functionID, 
                                                        rejitID,
                                                        0,
                                                        &cCountReturned,
                                                        NULL);
        if (FAILED(hr))
        {
            FAILURE(L"GetNativeCodeStartAddresses failed with hr=0x" << hex << hr);
            m_verifyFailed = TRUE;
            return E_FAIL;
        }

        hr = pInfo->GetNativeCodeStartAddresses(functionID, 
                                                        rejitID,
                                                        NumStartAddresses,
                                                        &cCodeStartAddresses,
                                                        codeStartAddresses);
        if (FAILED(hr))
        {
            FAILURE(L"GetNativeCodeStartAddresses failed with hr=0x" << hex << hr);
            m_verifyFailed = TRUE;
            return E_FAIL;
        }

        if (cCountReturned != cCodeStartAddresses)
        {
            FAILURE(L"Count returned by first call is not equal to number returned the second time.");
            m_verifyFailed = TRUE;
            return E_FAIL;
        }

        if (cCodeStartAddresses != m_jitCount)
        {
            FAILURE(L"GetNativeCodeStartAddresses returned " << cCodeStartAddresses 
                        << L" after " << m_jitCount << "jit events. These should be the same.");
            m_verifyFailed = TRUE;
            return E_FAIL;
        }

        for(ULONG32 i = 0; i < cCodeStartAddresses; ++i)
        {
            UINT_PTR pNativeCodeStartAddress = codeStartAddresses[i];

            ULONG32 cMap;
            COR_DEBUG_IL_TO_NATIVE_MAP map[NumStartAddresses];
            hr = pInfo->GetILToNativeMapping3(pNativeCodeStartAddress,
                                              NumStartAddresses,
                                              &cMap,
                                              map);
            if (FAILED(hr))
            {
                FAILURE(L"GetILToNativeMapping3 returned hr=0x" << hex << hr << L" for code address=0x" << hex << pNativeCodeStartAddress);
                return E_FAIL;
            }

            ULONG32 cCodeInfos;
            COR_PRF_CODE_INFO codeInfos[NumStartAddresses];
            hr = pInfo->GetCodeInfo4(pNativeCodeStartAddress, 
                              NumStartAddresses,
                              &cCodeInfos,
                              codeInfos);
            if (FAILED(hr))
            {
                FAILURE(L"GetCodeInfo4 returned hr=0x" << hex << hr << L" for code address=0x" << hex << pNativeCodeStartAddress);
                return E_FAIL;
            }
        }

        ++m_verifySuccessfulCount;
        return S_OK;
    }
    
	HRESULT Verify(IPrfCom * pPrfCom)
	{
        HRESULT hr = S_OK;

        DISPLAY(L"m_verifySuccessfulCount=" << m_verifySuccessfulCount);
        DISPLAY(L"m_jitCount=" << m_jitCount);

		if (m_verifyFailed) 
		{
            DISPLAY(L"Tiered compliation verification did not pass.");
            hr = E_FAIL;
		}

        if (m_verifySuccessfulCount != m_jitCount)
        {
            DISPLAY(L"Number of successful verifications did not match number of jit events.");
            hr = E_FAIL;
        }

        if (m_verifySuccessfulCount == 0 || m_jitCount == 0)
        {
            DISPLAY(L"Test did not run.");
            hr = E_FAIL;
        }

		return hr;
	}
	static TieredCompilationVerification* This;
	static TieredCompilationVerification* Instance()
	{
		return This;
	}
};

TieredCompilationVerification* TieredCompilationVerification::This = new TieredCompilationVerification(NULL, FALSE);

HRESULT TieredCompilation_JITCompilationFinished(IPrfCom * pPrfCom, FunctionID functionID, HRESULT hrStatus, BOOL fIsSafeToBlock)
{
    HRESULT hr = TieredCompilationVerification::Instance()->JITCompilationFinished(pPrfCom, functionID, hrStatus, fIsSafeToBlock);
    return hr;
}

HRESULT TieredCompilation_Verify(IPrfCom * pPrfCom)
{
    HRESULT hr = TieredCompilationVerification::Instance()->Verify(pPrfCom);
	DISPLAY(L"\nProfiler API Test: TieredCompilation_Verify hr = 0x" << hex << hr << L"\n");

	return hr;
}

void TieredCompilation_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
	DISPLAY(L"\nProfiler API Test: TieredCompilation_Initialize");

    pModuleMethodTable->FLAGS = 
            COR_PRF_MONITOR_JIT_COMPILATION   |
            COR_PRF_MONITOR_MODULE_LOADS      |
            COR_PRF_DISABLE_ALL_NGEN_IMAGES
            ;

    REGISTER_CALLBACK(JITCOMPILATIONFINISHED,       TieredCompilation_JITCompilationFinished);
    REGISTER_CALLBACK(VERIFY,                       TieredCompilation_Verify);


	pModuleMethodTable->VERIFY = (FC_VERIFY) &TieredCompilation_Verify;
}