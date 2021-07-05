// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// FinalizerQueue.cpp
//
// Template for creating new profiler tests using the RunProf/TestProfiler framework.
// 
// ======================================================================================

#include "../../ProfilerCommon.h"


//---------------------------------------------------------------------------------------
//
// FinalizerQueue illustrates the basics of writing new profiler tests.  
//
// Assumptions:
//
// Notes:
//    Uses initialize and verify functions called by TestProfiler.
//    Uses test macros to manage class pointers.
//    Registers for FunctionEnter2 callbacks and displays the names of functions.
//    
//    To compile /w4 /wx (warning level 4 && treat warnings as errors) unreferenced 
//    function parameters are commented out.  Simply uncomment to use.
//

class FinalizerQueue
{
public:

    //
    // Constructors & Destructors
    //

    FinalizerQueue(IPrfCom * pPrfCom);
    ~FinalizerQueue();

    // Called at the end of the tests.  Returning S_OK signifies the test passed and E_FAIL that it failed.
    HRESULT Verify(IPrfCom * pPrfCom);
    static HRESULT FinalizeableObjectQueuedWrapper(IPrfCom * pPrfCom,
                                                   DWORD finalizerFlags,
                                                   ObjectID objectID)
    {
        return STATIC_CLASS_CALL(FinalizerQueue)->FinalizeableObjectQueued(pPrfCom, finalizerFlags, objectID);
    }
private:

    //
    // Callback instance methods.  These should match the static wrappers above.
    //
    HRESULT FinalizeableObjectQueued(IPrfCom *  pPrfCom ,
                                               DWORD  finalizerFlags ,
                                               ObjectID  objectID );
    
    //
    // Class variables
    //

    // Success/Failure counters
    ULONG m_ulSuccess;
    ULONG m_ulFailure;
};


//
// Initialize the FinalizerQueue class.  
// 
FinalizerQueue::FinalizerQueue(IPrfCom * /*pPrfCom*/)
  : m_ulSuccess(0),
    m_ulFailure(0)
{
    // Initialize class state
}


//
// Clean up
// 
FinalizerQueue::~FinalizerQueue()
{
}


//----------------------------------------------FinalizerQueue-----------------------------------------
//
//
HRESULT FinalizerQueue::FinalizeableObjectQueued(IPrfCom * pPrfCom ,
                                           DWORD finalizerFlags,
                                           ObjectID objectID)
{
   
    ClassID classID;
    HRESULT hr = PINFO->GetClassFromObject(objectID, &classID);
    if (FAILED(hr))
    {
        m_ulFailure++;
        FAILURE(L"Failed to get class for object : " << HEX(finalizerFlags));   
    }

     if (classID&(3))//GC_MARKED|GC_PINNED
    {
        m_ulFailure++;
        FAILURE(L"Invalid classID in FinalizeableObjectQueued: " << HEX(finalizerFlags));
    }

    m_ulSuccess++;

    return S_OK;
}



//---------------------------------------------------------------------------------------
//
// FinalizerQueue::Verify determines and reports result of the test run
//
// Arguments:
//    pPrfCom – ProfilerCommon utility class pointer
//
// Return Value:
//    S_OK   - Test Passed
//    E_FAIL – Test Failed
//
// Assumptions:
//    
// Notes:
//    Can be called while other threads are still executing in your class.  
//    Be thread safe.
//

HRESULT FinalizerQueue::Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"FinalizerQueue Callback Verification...");
    
    if ((m_ulFailure == 0) && (m_ulSuccess > 0))
    {
        
        DISPLAY(L"Test passed! "<< m_ulSuccess << L" objects FQueued!" );
        return S_OK;
    }
    else
    {
        FAILURE(L"Either some checks failed, or no successful checks were completed.\n" <<
                L"\tm_ulFailure = " << m_ulFailure << L" m_ulSuccess = " << m_ulSuccess);
        return E_FAIL;
    }
}


//---------------------------------------------------------------------------------------
//
// FinalizerQueue_Verify calls the test class verification function
//
// Arguments:
//    pPrfCom – ProfilerCommon utility class pointer
//
// Return Value:
//    S_OK   - Test Passed
//    E_FAIL – Test Failed
//
// Assumptions:
//    
// Notes:
//    Uses LOCAL_CLASS_POINTER and FREE_CLASS_POINTER macros to manage class instance.  
//    See ProfilerCommon.h for macro definitions
//

// static
HRESULT FinalizerQueue_Verify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(FinalizerQueue);
    HRESULT hr = pFinalizerQueue->Verify(pPrfCom);
    FREE_CLASS_POINTER(FinalizerQueue);
    
    return hr;
}


//---------------------------------------------------------------------------------------
//
// FinalizerQueue_Initialize informs TestProfiler how to deliver callbacks to test class
//
// Arguments:
//    pPrfCom            – ProfilerCommon utility class pointer
//    pModuleMethodTable - Struct of function pointers defining callbacks and class info
//
// Return Value:
//
// Assumptions:
//    
// Notes:
//    Uses macros to handles the management of the class instance
//    See ProfilerCommon.h for macro definitions
//
//    Sets the requested COR_PRF_* flags and ICorProfilerInfo callbacks  
//    See ProfilerCallbackTypedefs.h for function pointer definitions and struct layout
//    See CORProf.idl for flag and interface info
//

// static
void FinalizerQueue_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize FinalizerQueue.")

    // Create and save an instance of test class
    SET_CLASS_POINTER(new FinalizerQueue(pPrfCom));
    
    // Initialize MethodTable
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_GC;
    
    REGISTER_CALLBACK(FINALIZEABLEOBJECTQUEUED,  FinalizerQueue::FinalizeableObjectQueuedWrapper);
    REGISTER_CALLBACK(VERIFY, FinalizerQueue_Verify);

    return;
}
