// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// AllCallbacks.cpp
//
// ======================================================================================

#include "../../ProfilerCommon.h"

class AllCallbacks
{
    public:

        AllCallbacks(IPrfCom * pPrfCom);
        ~AllCallbacks();
		HRESULT Verify(IPrfCom * pPrfCom);

    private:

};

/*
 *  Initialize the AllCallbacks class.  
 */
AllCallbacks::AllCallbacks(IPrfCom * /* pPrfCom */)
{
}

/*
 *  Clean up
 */
AllCallbacks::~AllCallbacks()
{
}

/*
 *  Verification
 */
HRESULT AllCallbacks::Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"This stress profiler does not perform verification so it implicitly passed!")
    return S_OK;
}

/*
 *  Verification routine called by TestProfiler
 */
HRESULT AllCallbacks_Verify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(AllCallbacks);
    HRESULT hr = pAllCallbacks->Verify(pPrfCom);

    // Clean up instance of test class
    delete pAllCallbacks;
    pPrfCom->m_pTestClassInstance = NULL;

    return hr;
}

/*
 *  Initialization routine called by TestProfiler
 */
void AllCallbacks_Initialize(IPrfCom * pPrfCom,
                                          PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize AllCallbacks.\n")

    // Create and save an instance of test class
    // pModuleMethodTable->TEST_POINTER is passed into every callback as pPrfCom->m_privatePtr
    SET_CLASS_POINTER(new AllCallbacks(pPrfCom));

    // Turn off callback counting to reduce synchronization/serialization in TestProfiler
    // It isn't needed since we aren't using the counts anyway.
    pPrfCom->m_fEnableCallbackCounting = FALSE;

    // Initialize MethodTable - we want to get all callbacks;
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_ALL & ~COR_PRF_ENABLE_REJIT;

    REGISTER_CALLBACK(VERIFY, AllCallbacks_Verify);
    return;
}

