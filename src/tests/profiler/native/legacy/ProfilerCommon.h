// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// ProfilerCommon.h
//
// Defines the interface and default implementation of the ProfilerCommon class.  This 
// class contains common routines used by all profiler tests such display and IT to name
// routines.
// 
// ======================================================================================

#ifndef __PROFILER_DRIVER_COMMON__
#define __PROFILER_DRIVER_COMMON__

// Windows includes
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#include <windows.h>
#endif

#ifdef WIN32
#ifndef DECLSPEC_EXPORT
#define DECLSPEC_EXPORT __declspec(dllexport)
#endif//DECLSPEC_EXPORT

#ifndef DECLSPEC_IMPORT
#define DECLSPEC_IMPORT __declspec(dllimport)
#endif//DECLSPEC_IMPORT

#ifndef NAKED 
#define NAKED __declspec(naked)
#endif//NAKED
#else // WIN32
#define DECLSPEC_EXPORT
#define DECLSPEC_IMPORT
#define NAKED
#endif // WIN32

#ifndef EXTERN_C
#define EXTERN_C extern "C"
#endif//EXTERN_C

// Runtime includes
#include "cor.h"
#include "corhdr.h"     // Needs to be included before corprof

#ifndef WINCORESYS
#include "corsym.h"     // ISymUnmanagedReader
#endif

#include "corprof.h"    // Definitions for ICorProfilerCallback and friends
#include "corerror.h"   // For error code names

#define PLATFORM_WCHAR wchar_t
#define PROFILER_WCHAR WCHAR

// C++ Standard Library
#include <iostream>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <climits>
#include <thread>
#include <chrono>
#include <deque>
#include <list>
#include <vector>
#include <stack>
#include <map>
#include <algorithm>

using namespace std;
using std::ios;
    
//Profiler Helper
#include "ProfilerCommonHelper.h"

#include "../holder.h"
#include "../event.h"

// Standard array and string lengths
#define SHORT_LENGTH    32
#define STRING_LENGTH  256
#define LONG_LENGTH   1024

// Macros for satellites to call ICorProfilerInfo & IPrfCom interfaces.
#define PINFO pPrfCom->m_pInfo
#define PPRFCOM pPrfCom

// C string comparison made easy
#ifdef WINDOWS
#define STRING_EQUAL(inputString, testString) _wcsicmp(inputString, testString)==0 ?TRUE:FALSE
#else
#define STRING_EQUAL(inputString, testString) wcscmp(inputString, testString)==0 ?TRUE:FALSE
#endif

// Use in Test_Initialize(): REGISTER_CALLBACK(FUNCTIONENTER2, CTestClass::FuncEnter2Wrapper);
#define REGISTER_CALLBACK(callback, function) pModuleMethodTable->callback = (FC_##callback) &function

// Use in Test_Initialize(): SET_CLASS_POINTER(new CNewTest(pPrfCom));
// Created new test class instance and stores it so it will be passed into each Callback function.
#define SET_CLASS_POINTER( _initString_ ) pModuleMethodTable->TEST_POINTER = reinterpret_cast<VOID *>( _initString_ )

// Use in CallbackFuncion(): LOCAL_CLASS_POINTER(CNewTest);
// Creates variable named pCNewTest for use in that callback
#define LOCAL_CLASS_POINTER( _testClass_ ) _testClass_ * p##_testClass_ = reinterpret_cast< _testClass_ * >( pPrfCom->m_pTestClassInstance )

// Use in Test_Verify(): FREE_CLASS_POINTER(CNewTest);
// Deletes variable named pCNewTest and sets IPrfCom class pointer to NULL.
#define FREE_CLASS_POINTER( _testClass_ ) {     \
    delete p##_testClass_ ;                     \
    pPrfCom->m_pTestClassInstance = NULL ;      \
}                                               \

// Use in static callback wrappers: STATIC_CLASS_CALL(CNewTest)->NewTestProfilerCallback(pPrfCom, classId);
// Makes writing wrappers easier and cleaner.
#define STATIC_CLASS_CALL( _testClass_ ) reinterpret_cast< _testClass_ * >( pPrfCom->m_pTestClassInstance )

// Use to inform TestProfiler of the derived IPrfCom to use for the rest of the test run.
//
//  Test_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& testName)
//  {
//      INewCommon * pINewCommon = new INewCommon(pPrfCom->m_fProfilingASPorService);
//      SET_DERIVED_POINTER(pINewCommon);
//      ...
//
#define SET_DERIVED_POINTER( _newPrfCom_ ) pModuleMethodTable->IPPRFCOM = dynamic_cast<IPrfCom *>( _newPrfCom_ );

// Use in CallbackFuncions(): DERIVED_COMMON_POINTER(INewCom);
// When a derived class is used instead of PrfCom, this will create local pINewCom pointer for use in the function.
#define DERIVED_COMMON_POINTER( _newComm_ ) _newComm_ * p##_newComm_ = dynamic_cast< _newComm_ *>(pPrfCom);

// MUST_PASS will assert if the call returns failing HR.
#define MUST_PASS(call) {                                                                           \
                            HRESULT __hr = (call);                                                  \
                            if( FAILED(__hr) )                                                      \
                            {                                                                       \
                                FAILURE(L"\nCall '" << #call << L"' failed with HR=" << HEX(__hr)   \
                                        << L"\nIn " << __FILE__ << L" at line " << __LINE__);       \
                            }                                                                       \
                        } 

// MUST_RETURN_VALUE will assert if the call does not return value
#define MUST_RETURN_VALUE(call, value) {                                                            \
                            HRESULT __hr = static_cast<HRESULT>(call);                              \
                            if( __hr != static_cast<HRESULT>(value) )                               \
                            {                                                                       \
                                FAILURE(L"\nCall '" << #call << L"' returned " << HEX(__hr)           \
                                        << L"Instead of expected " << HEX(value)                    \
                                        << L"\nIn " << __FILE__ << L" at line " << __LINE__);       \
                            }                                                                       \
                        } 

// MUST_NOT_RETURN_VALUE will assert if the call returns value
#define MUST_NOT_RETURN_VALUE(call, value) {                                                        \
                            HRESULT __hr = static_cast<HRESULT>(call);                              \
                            if( __hr == static_cast<HRESULT>(value) )                               \
                            {                                                                       \
                                FAILURE(L"\nCall '" << #call << L"' returned " << HEX(__hr)           \
                                        << L"\nIn " << __FILE__ << L" at line " << __LINE__);       \
                            }                                                                       \
                        } 
#define WCHAR_STR(name) wstring name;                                            \
                        name.reserve(STRING_LENGTH);

// Print macros used throughout framework
#define DEC std::dec
// #define HEX(__num) L"0x" << std::hex << std::uppercase << __num << std::dec

#define HEX
#define FLT std::fixed

// Prints log file
// Assert helps locate where these macros are used with a null m_pPrfCom pointer. 
#define DISPLAY( message )  {                                                                       \
                                assert(pPrfCom != NULL);                                            \
                                std::wstringstream temp;                                            \
                                temp << message;                                                    \
                                pPrfCom->Display(temp);                                             \
                            }

// Writes failure to log file
#define FAILURE( message )  {                                                                       \
                                assert(pPrfCom != NULL);                                            \
                                std::wstringstream temp;                                            \
                                temp << message;                                                    \
                                pPrfCom->Error(temp) ;                                              \
                            }    // Prints to log file when logging is enabled. (set PRF_DBG=1)


// Environment variables which are used by TestProfiler for test initialization
#define ENVVAR_BPD_SATELLITEMODULE  L"bpd_satellitemodule"
#define ENVVAR_BPD_TESTNAME         L"bpd_testname"
#define ENVVAR_BPD_OUTPUTFILE       L"bpd_outputfile"
#define ENVVAR_BPD_ENABLEASYNC      L"bpd_enableasync"
#define ENVVAR_BPD_ATTACHMODE       L"bpd_attachmode"
#define ENVVAR_BPD_USEDSSWITHEBP    L"bpd_usedsswithebp"

/*
ValidatePtr will try to see if the pointer to argument is a 'valid' pointer.
Originally the test was using IsBadReadPtr to test if reading from the pointer doesn't raise any exception.
However, this won't necessarily mean that the pointer is valid, since it could be pointing to a sane address
but not an address within the stack, making the check too lenient.
On Linux, we can check this by using the malloc_usable_size function to see if it is a stack-allocated address.
*/
#if !defined(WINDOWS) && !defined(OSX)
#include <malloc.h>
bool IsStackPtr(void * ptr);
#endif

bool ValidatePtr(void* ptr, size_t size);

#ifdef __clang__
#define PROFILER_STRING(str) u##str
template<typename T, typename V>
inline void ConvertCharArray(T *dst, size_t dstLen, const V *src, size_t srcLen)
{
    if(dst == NULL || src == NULL)
    {
        return;
    }

    for(int i = 0; i < dstLen && i < srcLen; ++i)
    {
        dst[i] = static_cast<T>(src[i]);

        if(src[i] == 0)
        {
            break;
        }
    }
}

inline void ConvertProfilerWCharToPlatformWChar(PLATFORM_WCHAR *dst, size_t dstLen, const PROFILER_WCHAR *src, size_t srcLen)
{
    return ConvertCharArray<PLATFORM_WCHAR, PROFILER_WCHAR>(dst, dstLen, src, srcLen);
}

inline void ConvertPlatformWCharToProfilerWChar(PROFILER_WCHAR *dst, size_t dstLen, const PLATFORM_WCHAR *src, size_t srcLen)
{
    return ConvertCharArray<PROFILER_WCHAR, PLATFORM_WCHAR>(dst, dstLen, src, srcLen);
}
#else // __clang__ 
#define PROFILER_STRING(str) L##str
inline void ConvertProfilerWCharToPlatformWChar(PLATFORM_WCHAR *dst, size_t dstLen, const PROFILER_WCHAR *src, size_t srcLen)
{
    if(dst == NULL || src == NULL)
    {
        return;
    }
    
    memcpy_s(dst, dstLen * sizeof(PLATFORM_WCHAR), src, srcLen * sizeof(PROFILER_WCHAR));
}

inline void ConvertPlatformWCharToProfilerWChar(PROFILER_WCHAR *dst, size_t dstLen, const PLATFORM_WCHAR *src, size_t srcLen)
{
    if(dst == NULL || src == NULL)
    {
        return;
    }
    
    memcpy_s(dst, dstLen * sizeof(PROFILER_WCHAR), src, srcLen * sizeof(PLATFORM_WCHAR));
}
#endif // __clang__ 

template<typename T>
BOOL IsNullTerminated(T string, size_t stringLength)
{
    for (size_t i = 0; i < stringLength; ++i)
    {
        if (string[i] == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

// Platform independent case insensitive comparison
BOOL StrCmpIns(const PLATFORM_WCHAR *lhs, const PLATFORM_WCHAR *rhs);
std::string ToNarrow(std::wstring str);
std::wstring ToWide(std::string str);
std::wstring ReadEnvironmentVariable(std::wstring name);

inline void ConvertWStringToLower(PLATFORM_WCHAR * wstr, int strLen)
{
#ifdef WINDOWS
    _wcslwr_s(wstr, strLen);
#else
    int i = 0;
    while (i < strLen && wstr[i])
    {
        wstr[i] = towlower(wstr[i]);
        ++i;
    }
#endif
}

INT CompareCaseInsensitiveWString(const PLATFORM_WCHAR * wstr1, const PLATFORM_WCHAR * wstr2);

// Same as Sleep(), but cross platform
VOID YieldProc();
VOID YieldProc(DWORD dwSleepMs);

#ifdef __clang__ 
inline void wcscat_s(wchar_t * dest, size_t destsz,
                 const wchar_t * src)
{
    wcscat(dest, src);
}

#define swprintf_s swprintf
#endif // __clang__ 

BOOL ContainsAtEnd(const PLATFORM_WCHAR *wszContainer, const PLATFORM_WCHAR *wszProspectiveEnding);

template<typename T>
bool IsWithinFudgeFactor(T value, T comparand, double lowerFudgeFactor, double upperFudgeFactor = 1.0)
{
    T lowerBound = (T)(comparand * lowerFudgeFactor);
    T upperBound = (T)(comparand * upperFudgeFactor);

    return value >= lowerBound && value <= upperBound;
}

class IPrfCom
{
    public:
        virtual ~IPrfCom() = default;

        // Common ICorProfilerInfo3 pointer
        ICorProfilerInfo3 *m_pInfo;

        // Common ICorProfilerInfo4 pointer
        // I'm using a separate variable to avoid updating existing testcases
        ICorProfilerInfo4 *m_pInfo4;

        // Critical Sections 
        std::recursive_mutex m_criticalSection;             // Public for satellites to use.  Do not use in PrfCommon.
        std::recursive_mutex m_asynchronousCriticalSection; // For synchronization in callbacks.
        
        // Output critical sections to protect output string streams
        std::mutex m_outputMutex;

        // Test class instance pointer.
        PVOID m_pTestClassInstance;

        // Running in startup mode or attach mode?
        BOOL m_bAttachMode;

        // old DSS implementation or new EBP-based one?
        BOOL m_bUseDSSWithEBP;

        // Event and Methods for synchronizing Sampling Hard Suspends DO NOT USE! Only used by
        // DoStackSnapshot hijacking tests.
        std::shared_ptr<ManualEvent> m_asynchEvent;
        BOOL   m_fEnableAsynch;
        virtual HRESULT InitializeForAsynch() = 0;
        virtual HRESULT RequestHardSuspend()  = 0;
        virtual HRESULT ReleaseHardSuspend()  = 0;

        // Output functions
        virtual VOID Display(std::wstringstream& sstrm) = 0;
        virtual VOID Error(std::wstringstream& sstrm) = 0;
        
        // Common utility routines
        virtual HRESULT GetAppDomainIDName(AppDomainID appdomainId, wstring &name, const BOOL full = FALSE) = 0;
        virtual HRESULT GetAssemblyIDName(AssemblyID assemblyId, wstring &name, const BOOL full = FALSE) = 0;
        virtual HRESULT GetModuleIDName(ModuleID funcId, wstring &name, const BOOL full = FALSE) = 0;
        virtual HRESULT GetClassIDName(ClassID classId, wstring &name, const BOOL full = FALSE) = 0;
        virtual HRESULT GetFunctionIDName(FunctionID funcId, wstring &name, const COR_PRF_FRAME_INFO frameInfo = 0, const BOOL full = FALSE) = 0;
        virtual BOOL IsFunctionStatic(FunctionID funcId, const COR_PRF_FRAME_INFO frameInfo = 0) = 0;

        // Unregister for Profiler Callbacks
        virtual HRESULT RemoveEvent(DWORD event) = 0;

#pragma region Callback Counters
        
        // startup - shutdown
        std::atomic<LONG> m_Startup;
        std::atomic<LONG> m_Shutdown;
        std::atomic<LONG> m_ProfilerAttachComplete;
        std::atomic<LONG> m_ProfilerDetachSucceeded;

        // threads
        std::atomic<LONG> m_ThreadCreated;
        std::atomic<LONG> m_ThreadDestroyed;
        std::atomic<LONG> m_ThreadAssignedToOSThread;
        std::atomic<LONG> m_ThreadNameChanged;

        // appdomains
        std::atomic<LONG> m_AppDomainCreationStarted;
        std::atomic<LONG> m_AppDomainCreationFinished;
        std::atomic<LONG> m_AppDomainShutdownStarted;
        std::atomic<LONG> m_AppDomainShutdownFinished;

        // assemblies
        std::atomic<LONG> m_AssemblyLoadStarted;
        std::atomic<LONG> m_AssemblyLoadFinished;
        std::atomic<LONG> m_AssemblyUnloadStarted;
        std::atomic<LONG> m_AssemblyUnloadFinished;

        // modules
        std::atomic<LONG> m_ModuleLoadStarted;
        std::atomic<LONG> m_ModuleLoadFinished;
        std::atomic<LONG> m_ModuleUnloadStarted;
        std::atomic<LONG> m_ModuleUnloadFinished;
        std::atomic<LONG> m_ModuleAttachedToAssembly;

        // classes
        std::atomic<LONG> m_ClassLoadStarted;
        std::atomic<LONG> m_ClassLoadFinished;
        std::atomic<LONG> m_ClassUnloadStarted;
        std::atomic<LONG> m_ClassUnloadFinished;
        std::atomic<LONG> m_FunctionUnloadStarted;

        // JIT
        std::atomic<LONG> m_JITCompilationStarted;
        std::atomic<LONG> m_JITCompilationFinished;
        std::atomic<LONG> m_JITCachedFunctionSearchStarted;
        std::atomic<LONG> m_JITCachedFunctionSearchFinished;
        std::atomic<LONG> m_JITFunctionPitched;
        std::atomic<LONG> m_JITInlining;

        // exceptions
        std::atomic<LONG> m_ExceptionThrown;

        std::atomic<LONG> m_ExceptionSearchFunctionEnter;
        std::atomic<LONG> m_ExceptionSearchFunctionLeave;
        std::atomic<LONG> m_ExceptionSearchFilterEnter;
        std::atomic<LONG> m_ExceptionSearchFilterLeave;

        std::atomic<LONG> m_ExceptionSearchCatcherFound;
        std::atomic<LONG> m_ExceptionCLRCatcherFound;
        std::atomic<LONG> m_ExceptionCLRCatcherExecute;

        std::atomic<LONG> m_ExceptionOSHandlerEnter;
        std::atomic<LONG> m_ExceptionOSHandlerLeave;

        std::atomic<LONG> m_ExceptionUnwindFunctionEnter;
        std::atomic<LONG> m_ExceptionUnwindFunctionLeave;
        std::atomic<LONG> m_ExceptionUnwindFinallyEnter;
        std::atomic<LONG> m_ExceptionUnwindFinallyLeave;
        std::atomic<LONG> m_ExceptionCatcherEnter;
        std::atomic<LONG> m_ExceptionCatcherLeave;

         // transitions
        std::atomic<LONG> m_ManagedToUnmanagedTransition;
        std::atomic<LONG> m_UnmanagedToManagedTransition;

        // ccw
        std::atomic<LONG> m_COMClassicVTableCreated;
        std::atomic<LONG> m_COMClassicVTableDestroyed;

            // suspend
        std::atomic<LONG> m_RuntimeSuspendStarted;
        std::atomic<LONG> m_RuntimeSuspendFinished;
        std::atomic<LONG> m_RuntimeSuspendAborted;
        std::atomic<LONG> m_RuntimeResumeStarted;
        std::atomic<LONG> m_RuntimeResumeFinished;
        std::atomic<LONG> m_RuntimeThreadSuspended;
        std::atomic<LONG> m_RuntimeThreadResumed;

        // gc
        std::atomic<LONG> m_MovedReferences;
        std::atomic<LONG> m_MovedReferences2;
        std::atomic<LONG> m_ObjectAllocated;
        std::atomic<LONG> m_ObjectReferences;
        std::atomic<LONG> m_RootReferences;
        std::atomic<LONG> m_ObjectsAllocatedByClass;

        // remoting
        std::atomic<LONG> m_RemotingClientInvocationStarted;
        std::atomic<LONG> m_RemotingClientInvocationFinished;
        std::atomic<LONG> m_RemotingClientSendingMessage;
        std::atomic<LONG> m_RemotingClientReceivingReply;

        std::atomic<LONG> m_RemotingServerInvocationStarted;
        std::atomic<LONG> m_RemotingServerInvocationReturned;
        std::atomic<LONG> m_RemotingServerSendingReply;
        std::atomic<LONG> m_RemotingServerReceivingMessage;

        // suspension counter array
        //DWORD m_dwSuspensionCounters[(DWORD)COR_PRF_SUSPEND_FOR_GC_PREP+1];
        std::atomic<LONG>  m_ForceGCEventCounter;
        std::atomic<DWORD> m_dwForceGCSucceeded;

        // enter-leave counters
        std::atomic<LONG> m_FunctionEnter;
        std::atomic<LONG> m_FunctionLeave;
        std::atomic<LONG> m_FunctionTailcall;

        std::atomic<LONG> m_FunctionEnter2;
        std::atomic<LONG> m_FunctionLeave2;
        std::atomic<LONG> m_FunctionTailcall2;

        std::atomic<LONG> m_FunctionEnter3WithInfo;     // counter for m_FunctionEnter3WithInfo (slowpath)
        std::atomic<LONG> m_FunctionLeave3WithInfo;     // counter for m_FunctionLeave3WithInfo (slowpath)
        std::atomic<LONG> m_FunctionTailCall3WithInfo;  // counter for m_FunctionTailcall3WithInfo (slowpath)

        std::atomic<LONG> m_FunctionEnter3;             // counter for m_FunctionEnter3 (fastpath)
        std::atomic<LONG> m_FunctionLeave3;             // counter for m_FunctionLeave3 (fastpath)
        std::atomic<LONG> m_FunctionTailCall3;          // counter for m_FunctionTailcall3 (fastpath)

        std::atomic<LONG> m_FunctionIDMapper;           // TODO we dont track this/use this. Do we need it?
        std::atomic<LONG> m_FunctionIDMapper2;          // TODO do we need a counter for this. See m_FunctionMapper for relevance.

        std::atomic<LONG> m_GarbageCollectionStarted;
        std::atomic<LONG> m_GarbageCollectionFinished;
        std::atomic<LONG> m_FinalizeableObjectQueued;
        std::atomic<LONG> m_RootReferences2;
        std::atomic<LONG> m_HandleCreated;
        std::atomic<LONG> m_HandleDestroyed;
        std::atomic<LONG> m_SurvivingReferences;
        std::atomic<LONG> m_SurvivingReferences2;

        // Keep track of active callbacks to prepare for Hard Suspend
        std::atomic<LONG> m_ActiveCallbacks;

#pragma endregion // Callback Counters
        ULONG m_ulError;
};


class PrfCommon: public IPrfCom
{
    public:

        PrfCommon(ICorProfilerInfo3 *pInfo3);
        virtual ~PrfCommon();

        virtual HRESULT InitializeForAsynch();
        virtual HRESULT RequestHardSuspend();
        virtual HRESULT ReleaseHardSuspend();

        virtual VOID Display(std::wstringstream& sstrm);
        virtual VOID Error(std::wstringstream& sstrm);

        virtual HRESULT GetAppDomainIDName(AppDomainID appdomainId, wstring &name, const BOOL full = FALSE);
        virtual HRESULT GetAssemblyIDName(AssemblyID assemblyId, wstring &name, const BOOL full = FALSE);
        virtual HRESULT GetModuleIDName(ModuleID funcId, wstring &name, BOOL const full = FALSE);
        virtual HRESULT GetClassIDName(ClassID classId, wstring &name, BOOL const full = FALSE);
        virtual HRESULT GetFunctionIDName(FunctionID funcId, wstring &name, const COR_PRF_FRAME_INFO frameInfo = 0, const BOOL full = FALSE);

        virtual BOOL IsFunctionStatic(FunctionID funcId, const COR_PRF_FRAME_INFO frameInfo = 0);

        virtual HRESULT RemoveEvent(DWORD event);

};

#include "ProfilerCallbackTypedefs.h"

#endif //__PROFILER_DRIVER_COMMON__

