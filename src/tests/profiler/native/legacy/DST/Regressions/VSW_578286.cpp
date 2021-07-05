#include "../../ProfilerCommon.h"
#include "../../LegacyCompat.h" // Whidbey Framework Support

class VSW578286
{
    public:

        static HRESULT Verify(IPrfCom * pPrfCom);
        static HRESULT TransitionCallback(IPrfCom * pPrfCom, FunctionID functionId, COR_PRF_TRANSITION_REASON reason);

    private:

        static ULONG m_ulFailure;
};

ULONG VSW578286::m_ulFailure = 0;

/*
 *  We do not want to receive any U2M or M2U callbacks for qcalls.
 */
HRESULT VSW578286::TransitionCallback(IPrfCom * pPrfCom, FunctionID functionId, COR_PRF_TRANSITION_REASON /*reason*/)
{
    WCHAR_STR( funcName );
    HRESULT hr = S_OK;

    // Get the name of the current function
    hr = PPRFCOM->GetFunctionIDName(functionId, funcName);
    if (FAILED(hr))
    {
        FAILURE(L"GetFunctionNameFromFunctionID in TransitionCallback failed for FunctionID 0x" << HEX(functionId));
        VSW578286::m_ulFailure++;
        return E_FAIL;
    }

    // Known QCalls exercised by qcalls test input.
    if ( (funcName.find(L"GetFullName")  != string::npos) ||
         (funcName.find(L"GetResource")  != string::npos) ||
         (funcName.find(L"GetPublicKey") != string::npos) )
    {
        FAILURE(L"Transition Callback recieved for QCall " << funcName.c_str() << "!");
        VSW578286::m_ulFailure++;
        return E_FAIL;
    }

    return S_OK;
}

/*
 *  Verify the results of this run.
 */
HRESULT VSW578286::Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"VSW578286 Callback Verification\n");

    if (VSW578286::m_ulFailure == 0)
    {
        DISPLAY(L"Test passed!");
        return S_OK;
    }
    else
    {
        DISPLAY(L"Teat failed.  QCall transition callback received.");
        return E_FAIL;
    }
}

/*
 *  Verification routine called by TestProfiler
 */
HRESULT VSW578286_Verify(IPrfCom * pPrfCom)
{
    return VSW578286::Verify(pPrfCom);
}

/*
 *  Initialization routine called by TestProfiler
 */
void VSW_578286_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize VSW578286.\n");

    // Initialize MethodTable
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_CODE_TRANSITIONS;
    pModuleMethodTable->MANAGEDTOUNMANAGEDTRANSITION = (FC_MANAGEDTOUNMANAGEDTRANSITION) &VSW578286::TransitionCallback;
    pModuleMethodTable->UNMANAGEDTOMANAGEDTRANSITION = (FC_UNMANAGEDTOMANAGEDTRANSITION) &VSW578286::TransitionCallback;
    pModuleMethodTable->VERIFY = (FC_VERIFY) &VSW578286_Verify;

    return;
}