#include "JITCommon.h"

// Legacy support
#include "../LegacyCompat.h"
// Legacy support


                                    

atomic<LONG> gReJIT_nOccurences;
atomic<LONG> gReJIT_nJITCompile;
FunctionID gReJIT_FuncID;

HRESULT ReJIT_JITCachedFunctionSearchStarted(IPrfCom * pPrfCom, FunctionID functionID, BOOL * pbUseCachedFunction)
{
    ThreadID thread;
    pPrfCom->m_pInfo->GetCurrentThreadID(&thread);
    DISPLAY(L"0x" << thread << L"JITCachedFunctionSearchStarted: 0x" << functionID << L"\n");
    *pbUseCachedFunction = FALSE;

    WCHAR_STR( funcName );
    pPrfCom->GetFunctionIDName(functionID, funcName);

    DISPLAY(funcName << L" - 0x" << functionID << L"\n");

    return S_OK;
}

HRESULT ReJIT_JITCompilationStarted_Reg(IPrfCom * pPrfCom, FunctionID functionID, BOOL /*fIsSafeToBlock*/ )
{
    ThreadID thread;
    pPrfCom->m_pInfo->GetCurrentThreadID(&thread);
    DISPLAY(L"0x" << thread << L" JITCompilationStarted: 0x" << functionID << L"\n");

    HRESULT hr;

    WCHAR_STR( funcName );
    pPrfCom->GetFunctionIDName(functionID, funcName);

    DISPLAY(funcName << L" - 0x" << functionID << L"\n");

    if (gReJIT_FuncID == 0)
    {
        gReJIT_FuncID = functionID;
    }
    else
    {
        hr = pPrfCom->m_pInfo->SetFunctionReJIT(gReJIT_FuncID);
        if (hr != E_NOTIMPL)
        {
            FAILURE(L"SetFunctionReJIT returned 0x" << hr << L" for FunctionID 0x" << gReJIT_FuncID << L"\n");
            return hr;
        }

        gReJIT_FuncID = functionID;
    }

    return S_OK;
}

HRESULT ReJIT_JITCompilationFinished_reg(IPrfCom * pPrfCom, FunctionID functionID, HRESULT hrStatus, BOOL /*fIsSafeToBlock*/ )
{
    ThreadID thread;
    pPrfCom->m_pInfo->GetCurrentThreadID(&thread);
    DISPLAY(L"0x" << thread << L" ReJIT_JITCompilationFinished - 0x" << functionID << L"\n");

    if (FAILED(hrStatus))
    {
        FAILURE(L"Function FAILED to JIT: FUNCTIONID ID: 0x" << functionID << L"  HRESULT: 0x" << hrStatus << L"\n");
        return S_OK;
    }

    return S_OK;

}

HRESULT ReJIT_JITCompilationStarted(IPrfCom * pPrfCom, FunctionID functionID, BOOL /*fIsSafeToBlock*/ )
{
    DISPLAY(L"ReJIT_JITCompilationStarted\n");

    HRESULT hr;

    WCHAR_STR( funcName );
    hr = pPrfCom->GetFunctionIDName(functionID, funcName);
	if (FAILED(hr))
    {
        FAILURE(L"GetFunctionNameFromFunctionID returned 0x" << hr << L" from ReJIT_JITCompilationStarted\n");
        return hr;
    }

    if (funcName.find(L"add3") != string::npos)
    {
        ++gReJIT_nOccurences;
        gReJIT_FuncID = functionID;

        // Negative test case.  Can not call Re-JIT on own FuncID in JITCompliationStarted
        hr = pPrfCom->m_pInfo->SetFunctionReJIT(functionID);
        if (hr != E_NOTIMPL)
        {
            FAILURE(L"IProfilerInfo::SetFunctionReJIT() FAILED - E_NOTIMPL Was Expected.  Received 0x" << hr << L"\n");
            return hr;
        }
    }

    ++gReJIT_nJITCompile;

    return S_OK;
}

HRESULT ReJIT_JITCompilationFinished(IPrfCom * pPrfCom, FunctionID functionID, HRESULT hrStatus, BOOL /*fIsSafeToBlock*/ )
{
    DISPLAY(L"ReJIT_JITCompilationFinished\n");

    if (FAILED(hrStatus))
    {
        FAILURE(L"Function FAILED to JIT: FUNCTIONID ID: 0x" << functionID << L"  HRESULT: 0x" << hrStatus << L"\n");
        return S_OK;
    }

    // If we've JIT-ed less than 3 times, JIT again
    if (gReJIT_nOccurences < 3)
    {
        // If this function is add3() ...
        if (gReJIT_FuncID == functionID)
        {
            HRESULT hr;
            // ... call ReJIT on it
            hr = pPrfCom->m_pInfo->SetFunctionReJIT(functionID);
            if (hr != E_NOTIMPL)
            {
                FAILURE(L"ICorProfilerInfo::SetFunctionReJIT() FAILED.  Returned 0x" << hr << L"\n");
                return hr;
            }

            // ReJIT succeeded.  Do some negative testing.
            hr = pPrfCom->m_pInfo->SetFunctionReJIT(NULL);
            if (hr != E_NOTIMPL)
            {
                FAILURE(L"ICorProfilerInfo::SetFunctionReJIT() Negative Testing FAILED.  Returned 0x" << hr << L"\n");
                return hr;
            }
        }
    }

    --gReJIT_nJITCompile;

    return S_OK;

}

HRESULT ReJIT_Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"Verify ReJIT callbacks\n");

    if (gReJIT_nOccurences == 0)
    {
        FAILURE(L"FAILED: no JitCompilation callbacks encountered\n");
        return E_FAIL;
    }

    if (gReJIT_nOccurences != 1)
    {
        FAILURE(L"Observed Function Has Been Jitted an Unexpected Number of times,  " << gReJIT_nOccurences << L"\n");
        return E_FAIL;
    }

    if (gReJIT_nJITCompile != 0)
    {
            FAILURE(L"Unexpected JIT Method Count, " << gReJIT_nJITCompile << L"\n");
    }

    return S_OK;
}

void ReJIT_Initialize (IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const PLATFORM_WCHAR * /*TestName*/)
{
    DISPLAY(L"Initialize ReJIT extension\n");

    gReJIT_nOccurences = 0;
    gReJIT_nJITCompile = 0;
    gReJIT_FuncID = 0;

    pModuleMethodTable->FLAGS =     COR_PRF_ENABLE_REJIT |
                                COR_PRF_MONITOR_CACHE_SEARCHES |
                                COR_PRF_MONITOR_JIT_COMPILATION;

    pModuleMethodTable->VERIFY = (FC_VERIFY) &ReJIT_Verify;
    pModuleMethodTable->JITCOMPILATIONSTARTED = (FC_JITCOMPILATIONSTARTED) &ReJIT_JITCompilationStarted;
    pModuleMethodTable->JITCOMPILATIONFINISHED = (FC_JITCOMPILATIONFINISHED) &ReJIT_JITCompilationFinished;

    wstring env = ReadEnvironmentVariable(L"PROFILER_TEST_REJIT_REGRESSION_271186");
    if(env.size() > 0)
    {
        if (env == L"TRUE")
        {
            pModuleMethodTable->JITCACHEDFUNCTIONSEARCHSTARTED = (FC_JITCACHEDFUNCTIONSEARCHSTARTED) &ReJIT_JITCachedFunctionSearchStarted;
            pModuleMethodTable->JITCOMPILATIONSTARTED = (FC_JITCOMPILATIONSTARTED) &ReJIT_JITCompilationStarted_Reg;
            pModuleMethodTable->JITCOMPILATIONFINISHED = (FC_JITCOMPILATIONFINISHED) &ReJIT_JITCompilationFinished_reg;
            pModuleMethodTable->VERIFY = NULL;
        }
    }

    return;
}
