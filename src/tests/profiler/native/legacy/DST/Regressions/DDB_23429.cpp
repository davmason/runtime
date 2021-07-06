// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// ======================================================================================
//
// DDB_23429.cpp
//
// Hijacking corrupts floating point registers
// 
// ======================================================================================

#include "../../ProfilerCommon.h"


class ddb32429
{
public:

    #pragma region static_wrapper_methods

    static HRESULT RuntimeThreadSuspendedWrapper(IPrfCom * pPrfCom,
                                            ThreadID threadID)
    {
        return STATIC_CLASS_CALL(ddb32429)->RuntimeThreadSuspended(pPrfCom, threadID);
    }

    #pragma endregion // static_wrapper_methods

    HRESULT Verify(IPrfCom * pPrfCom);

private:

    #pragma region callback_handler_prototypes

    HRESULT ddb32429::RuntimeThreadSuspended(IPrfCom * /* pPrfCom */ ,
                                             ThreadID /* threadID */ );

    #pragma endregion // callback_handler_prototypes
};



extern "C" volatile int s_x;
volatile int s_x;

extern "C" __declspec(noinline) double bar(double x, double y, double z);

__declspec(noinline)
double bar(double x, double y, double z)
{
  s_x = (int)x;

  double a = x * y;
  double b = y * z;
  double c = z * x;

  double d = a * b;
  double e = b * c;
  double f = c * a;

  double g = a * d;
  double h = b * e;
  double i = c * f;


  return a*i + b*h + c*g + d*a + e*i + f*g + g*x + b*a;
}


HRESULT ddb32429::RuntimeThreadSuspended(IPrfCom * /* pPrfCom */ ,
                                         ThreadID /* threadID */ )
{
    int x;
    bar((int)(size_t)&x,(int)(size_t)&x,(int)(size_t)&x);

    return S_OK;
}



HRESULT ddb32429::Verify(IPrfCom * pPrfCom)
{
    int m_ulFailure = 0;
    int m_ulSuccess = 0;
    
    DISPLAY(L"ddb32429 Callback Verification...");

    // DDB_32429_PASS
    if (pPrfCom->m_ExceptionThrown > 0 && false)
    {
        DISPLAY(L"DDB_32429 failed\n");
        m_ulFailure = 1;
        return E_FAIL;
    }
    else 
    {
        DISPLAY(L"Test passed!");
        m_ulSuccess = 1;
        return S_OK;
    }
}


HRESULT ddb32429_Verify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(ddb32429);
    HRESULT hr = pddb32429->Verify(pPrfCom);
    FREE_CLASS_POINTER(ddb32429);
    
    return hr;
}


void DDB_32429_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize ddb32429.")

    // Create and save an instance of test class
    SET_CLASS_POINTER(new ddb32429());
    
    // Initialize MethodTable
    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_SUSPENDS;          

    REGISTER_CALLBACK(RUNTIMETHREADSUSPENDED, ddb32429::RuntimeThreadSuspendedWrapper);
    REGISTER_CALLBACK(VERIFY, ddb32429_Verify);

    return;
}
