#include "ProfilerCommon.h"

class VSW_579621
{
    public:

        VSW_579621(IPrfCom *pPrfCom);
        ~VSW_579621();

        static VSW_579621* Instance() { return This; }

        static HRESULT VSW_579621_FunctionLeaveWrapper(IPrfCom *pPrfCom, FunctionID funcId, UINT_PTR clientData, COR_PRF_FRAME_INFO func, COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange) 
            { return Instance()->VSW_579621_FunctionLeave(pPrfCom, funcId, clientData, func, retvalRange); }

        HRESULT Verify(IPrfCom *pPrfCom);

    private:

        HRESULT VSW_579621_FunctionLeave(IPrfCom *pPrfCom,
                                         FunctionID funcId,
                                         UINT_PTR clientData,
                                         COR_PRF_FRAME_INFO func,
                                         COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange);

        /*  This pointer used by static methods to find instance of class */
        static VSW_579621* This;

        /*  Success/Failure counters */
        ULONG m_ulSuccess;
        ULONG m_ulFailure;
};



/*
 *  Static this pointer used by wrappers to get instance of VSW_579621 class
 */
VSW_579621* VSW_579621::This = NULL;

/*
 *  Initialize the VSW_579621 class.  
 */
VSW_579621::VSW_579621(IPrfCom * /*pPrfCom*/)
{
    // Static this pointer
    VSW_579621::This = this;

    // Initialize result counters
    m_ulSuccess = 0;
    m_ulFailure = 0;
}


/*
 *  Clean up the static this pointer and synchronization objects
 */
VSW_579621::~VSW_579621()
{
    // Static this pointer
    VSW_579621::This = NULL;
}

/*
 *  
 */
HRESULT VSW_579621::VSW_579621_FunctionLeave(IPrfCom *pPrfCom,
                                             FunctionID funcId,
                                             UINT_PTR /*clientData*/,
                                             COR_PRF_FRAME_INFO func,
                                             COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange)
{
    HRESULT hr = S_OK;

    WCHAR_STR( functionName );
    hr = PPRFCOM->GetFunctionIDName(funcId, functionName, func);
    if (FAILED(hr))
    {
        FAILURE(L"GetFunctionIDName Failed.");
        m_ulFailure++;
        return hr;
    }

    // Here is our target function, it will return 0x12345678 so we can easily check if retvalRange is correct
    if (functionName.find(L"TestBad") != string::npos)
    {
        // Grab the return value
        UINT testValue = *(PUINT)(retvalRange->startAddress + 0x8); // I'm cheating here

        DISPLAY(L"COR_PRF_FUNCTION_ARGUMENT_RANGE->StartAddress : " << HEX(retvalRange->startAddress));
        DISPLAY(L"Return Value Should Be 0x12345678 and is : " << HEX(testValue));

        // And finally, the actual check in the test
        if (0x12345678 != testValue)
        {
            FAILURE(L"0x12345678 != " << HEX(testValue));
            m_ulFailure++;
        }
        else
        {
            m_ulSuccess++;
        }
    }

    return hr;
}


/*
 *  Verify the results of this run.
 */
HRESULT VSW_579621::Verify(IPrfCom *pPrfCom)
{
    DISPLAY(L"VSW_579621 Callback Verification\n");

    if ((m_ulFailure == 0) && (m_ulSuccess > 0))
    {
        DISPLAY(L"Test passed!")
        return S_OK;
    }
    else
    {
        FAILURE(L"Either some checks failed, or no successful checks were completed.")
        return E_FAIL;
    }
}


/*
 *  Global pointer to test class object
 */
VSW_579621* global_VSW_579621;


/*
 *  Verification routine called by TestProfiler
 */
HRESULT VSW_579621_Verify(IPrfCom *pPrfCom)
{
    HRESULT hr = VSW_579621::Instance()->Verify(pPrfCom);

    // Clean up instance of test class
    delete global_VSW_579621;

    return hr;
}


/*
 *  Initialization routine called by TestProfiler
 */
void VSW_579621_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize VSW_579621.\n")

    // Create instance of test class
    global_VSW_579621 = new VSW_579621(pPrfCom);

    // Initialize MethodTable
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_ENTERLEAVE | COR_PRF_ENABLE_FUNCTION_RETVAL;
    pModuleMethodTable->FUNCTIONLEAVE2 = (FC_FUNCTIONLEAVE2) &VSW_579621::VSW_579621_FunctionLeaveWrapper;
    pModuleMethodTable->VERIFY = (FC_VERIFY) &VSW_579621_Verify;

    return;
}

