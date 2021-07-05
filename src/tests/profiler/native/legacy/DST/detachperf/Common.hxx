#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500 
#endif // _WIN32_WINNT


#include <cor.h>
#include <corprof.h>
#include <windows.h>
#include <process.h>
#include <Psapi.h>
#include <iostream>
#include <iomanip>
#include "ThreadParams.h"
#include "ThreadMgr.h"
#include "TestHelper.h"

using namespace std;


///////////////////////////////////////////////////////////////////////////////

// Verbosity/Tracing Levels - These flags can be "OR"ed     
// together. Higher-order flags will over-ride lower-order  
// flags if ORed together.                                  
enum VerbosityLevel {
    TRACE_NONE      = 0x00000000,
    TRACE_ERROR     = 0x00000001,
    TRACE_NORMAL    = 0x00000002,
    TRACE_ENTRY     = 0x00000004,
    TRACE_EXIT      = 0x00000008    
};  

///////////////////////////////////////////////////////////////////////////////

//////////////////////////////////
// Global configuration options //
//////////////////////////////////

extern DWORD g_Verbosity; // Global verbosity level 
extern DWORD g_TraceOptions; 

//////////////////////////////////////////////////////////////////////////////////
// Utility Macros.                                                              //
// Note:    Variadic macros could have been used here but they are unsupported  //
//          by  older versions of the C/C++ compilers. Hence not used here.     //
//////////////////////////////////////////////////////////////////////////////////

#define HR_CHECK(_hr_, _exp_, _func_)   if (_exp_ != _hr_) {            \
                                            TRACEERROR(_hr_, _func_)    \
                                            if (S_OK == hr) {           \
                                                hr = E_FAIL;            \
                                            }                           \
                                            goto ErrReturn;             \
                                        }                               \

#define TRACEERROR(_hr_, _msg_)     if (g_Verbosity & TRACE_ERROR) {                            \
                                        wprintf(L"ERROR: %s. Error Code 0x%X\n", _msg_, _hr_);  \
                                    }                                                           \

#define TRACEERROR2(...)            if (g_Verbosity & TRACE_ERROR) {                                \
                                        wprintf(L"ERROR: "); wprintf(__VA_ARGS__); wprintf(L"\n");  \
                                    }                                                               \

#define TRACENORMAL(...)            if (g_Verbosity & TRACE_NORMAL) {                               \
                                        wprintf(L"TRACE: "); wprintf(__VA_ARGS__); wprintf(L"\n");  \
                                    }                                                               \

#define TRACEENTRY(...)             if (g_Verbosity & TRACE_ENTRY) {                                \
                                        wprintf(L"ENTRY: "); wprintf(__VA_ARGS__); wprintf(L"\n");  \
                                    }                                                               \

#define TRACEEXIT(...)              if (g_Verbosity & TRACE_EXIT) {                                 \
                                        wprintf(L"EXIT: "); wprintf(__VA_ARGS__); wprintf(L"\n");   \
                                    }                                                               \


///////////////////////////////////////////////////////////////////////////////