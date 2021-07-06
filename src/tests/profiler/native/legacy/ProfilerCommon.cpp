// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
//
// ProfilerCommon.cpp
//
// Implements PrfCommon, the instantiation of the IPrfCom interface.
//
// ======================================================================================
#ifdef __clang__
#pragma clang diagnostic ignored "-Wnull-arithmetic"
#endif // __clang__
#include "ProfilerCommon.h"

#ifdef __clang__
#include <sched.h>
#include <signal.h>
#include <setjmp.h>
#endif 

#pragma warning(push)
#pragma warning( disable : 4996)

// Modified print macros for use in ProfilerCommon.cpp
#undef DISPLAY
#undef LOG
#undef FAILURE
#undef VERBOSE

#define DISPLAY( message )  {                                                           \
                                std::wstringstream temp;                                \
                                temp << message;                                        \
                                Display(temp);    						                \
                            }

#define FAILURE( message )  {                                                           \
                                std::wstringstream temp;                                \
                                temp << message;                                        \
                                Error(temp) ;                                           \
                            }

#ifdef OSX
#define malloc_usable_size malloc_size
#include <malloc/malloc.h>
#endif

// Modified ICorProfilerInfo macro for use in ProfilerCommon
#undef  PINFO
#define PINFO m_pInfo

#ifndef WINDOWS
bool IsStackPtr(void * ptr)
{
    if (!ptr) {
        return 0 == malloc_usable_size(ptr);
    }
    return false;
}
#endif

bool ValidatePtr(void* ptr, size_t size)
{
#ifdef WINDOWS
    return IsBadReadPtr(ptr, size) == 0;
#else
    for (size_t i = 0; i < size; i++)
    {
        if (!IsStackPtr((void*)((char*)(ptr)+i)))
            return false;
    }
    return true;
#endif
}

BOOL StrCmpIns(const PLATFORM_WCHAR *lhs, const PLATFORM_WCHAR *rhs)
{
    while((*lhs != 0) && (*rhs != 0))
    {
        if(towlower(*lhs) != towlower(*rhs))
        {
            return FALSE;
        }

        ++lhs;
        ++rhs;
    }

    return (*lhs == 0) && (*rhs == 0);
}

string ToNarrow(wstring str)
{
    if (str.empty())
    {
        return string();
    }
    
    string dest(str.size(), L' ');
    wcstombs(&dest[0], str.c_str(), str.size());

    return dest;
}

wstring ToWide(string str)
{
    if (str.empty())
    {
        return wstring();
    }

    wstring dest(str.size(), L' ');
    mbstowcs(&dest[0], str.c_str(), str.size());

    return dest;
}

wstring ReadEnvironmentVariable(wstring name)
{
    string narrowName = ToNarrow(name);
    char *env = getenv(narrowName.c_str());

    if(env == NULL)
    {
        return wstring();
    }

    return ToWide(string(env));
}

INT CompareCaseInsensitiveWString(const PLATFORM_WCHAR * wstr1, const PLATFORM_WCHAR * wstr2)
{
    int i = 0;
    while (wstr1[i] && wstr2[i])
    {
        if (towlower(wstr1[i]) > towlower(wstr2[i]))
            return 1;
        else if (towlower(wstr2[i]) > towlower(wstr2[i]))
            return -1;
        ++i;
    }
    if (wstr1[i] == wstr2[i])
        return 0;
    else if (wstr1[i] < wstr2[i])
        return -1;
    else
        return 1;
}

VOID YieldProc()
{
#if defined(WIN32) || defined(WIN64)
    Sleep(0);
#else
    sched_yield();
#endif
}

VOID YieldProc(DWORD dwSleepMs)
{
#if defined(WIN32) || defined(WIN64)
    Sleep(dwSleepMs);
#else
    struct timespec t = {0};
    struct timespec rem = {0};
    t.tv_nsec = dwSleepMs * 1000;
    nanosleep(&t, &rem);
#endif
}

PrfCommon::PrfCommon(ICorProfilerInfo3 *pInfo3)
{
    DWORD dwRet = 0;
    
    wstring wszEnableAsynch = ReadEnvironmentVariable(ENVVAR_BPD_ENABLEASYNC); 
    if (wszEnableAsynch.size() != 0)
    {
        m_fEnableAsynch = true;
    }    

    m_pTestClassInstance = NULL;
    
    // Initialize ICorProfilerInfo* interfaces
    m_pInfo = pInfo3;
    m_pInfo->AddRef();
    HRESULT hr = pInfo3->QueryInterface(IID_ICorProfilerInfo4, (void **)&m_pInfo4);
    if (FAILED(hr))
    {
        FAILURE(L"QueryInterface(IID_ICorProfilerInfo4) failed with hr=0x" << std::hex << hr);  
    }

    if (m_fEnableAsynch)
    {
        // Set up Event used to communicate with Asynchronous satellite when loaded.
        m_asynchEvent = shared_ptr<ManualEvent>(new ManualEvent());
    }
    else
    {
        m_asynchEvent = NULL;
    }

    // Are we running in startup mode or attach mode?    
    wstring attach = ReadEnvironmentVariable(ENVVAR_BPD_ATTACHMODE); 
    m_bAttachMode = attach.size() > 0;
        
    // Are we supposed to use regular DSS or the new EBP-based implementation?    
    wstring wszUseDssWithEBP = ReadEnvironmentVariable(ENVVAR_BPD_USEDSSWITHEBP); 
    m_bUseDSSWithEBP = wszUseDssWithEBP.size() > 0;


#pragma region Callback Counter Initialization
    m_Startup = 0;
    m_Shutdown = 0;

    //attach
    m_ProfilerAttachComplete = 0;
    m_ProfilerDetachSucceeded = 0;

    // threads
    m_ThreadCreated = 0;
    m_ThreadDestroyed = 0;
    m_ThreadAssignedToOSThread = 0;
    m_ThreadNameChanged = 0;

    // appdomains
    m_AppDomainCreationStarted = 0;
    m_AppDomainCreationFinished = 0;
    m_AppDomainShutdownStarted = 0;
    m_AppDomainShutdownFinished = 0;

    // assemblies
    m_AssemblyLoadStarted = 0;
    m_AssemblyLoadFinished = 0;
    m_AssemblyUnloadStarted = 0;
    m_AssemblyUnloadFinished = 0;

    // modules
    m_ModuleLoadStarted = 0;
    m_ModuleLoadFinished = 0;
    m_ModuleUnloadStarted = 0;
    m_ModuleUnloadFinished = 0;
    m_ModuleAttachedToAssembly = 0;

    // classes
    m_ClassLoadStarted = 0;
    m_ClassLoadFinished = 0;
    m_ClassUnloadStarted = 0;
    m_ClassUnloadFinished = 0;
    m_FunctionUnloadStarted = 0;

    // JIT
    m_JITCompilationStarted = 0;
    m_JITCompilationFinished = 0;
    m_JITCachedFunctionSearchStarted = 0;
    m_JITCachedFunctionSearchFinished = 0;
    m_JITFunctionPitched = 0;
    m_JITInlining = 0;

    // exceptions
    m_ExceptionThrown = 0;
    m_ExceptionSearchFunctionEnter = 0;
    m_ExceptionSearchFunctionLeave = 0;
    m_ExceptionSearchFilterEnter = 0;
    m_ExceptionSearchFilterLeave = 0;

    m_ExceptionSearchCatcherFound = 0;
    m_ExceptionCLRCatcherFound = 0;
    m_ExceptionCLRCatcherExecute = 0;

    m_ExceptionOSHandlerEnter = 0;
    m_ExceptionOSHandlerLeave = 0;

    m_ExceptionUnwindFunctionEnter = 0;
    m_ExceptionUnwindFunctionLeave = 0;
    m_ExceptionUnwindFinallyEnter = 0;
    m_ExceptionUnwindFinallyLeave = 0;
    m_ExceptionCatcherEnter = 0;
    m_ExceptionCatcherLeave = 0;

     // transitions
    m_ManagedToUnmanagedTransition = 0;
    m_UnmanagedToManagedTransition = 0;

    // ccw
    m_COMClassicVTableCreated = 0;
    m_COMClassicVTableDestroyed = 0;

        // suspend
    m_RuntimeSuspendStarted = 0;
    m_RuntimeSuspendFinished = 0;
    m_RuntimeSuspendAborted = 0;
    m_RuntimeResumeStarted = 0;
    m_RuntimeResumeFinished = 0;
    m_RuntimeThreadSuspended = 0;
    m_RuntimeThreadResumed = 0;

    // gc
    m_MovedReferences = 0;
    m_ObjectAllocated = 0;
    m_ObjectReferences = 0;
    m_RootReferences = 0;
    m_ObjectsAllocatedByClass = 0;

    // remoting
    m_RemotingClientInvocationStarted = 0;
    m_RemotingClientInvocationFinished = 0;
    m_RemotingClientSendingMessage = 0;
    m_RemotingClientReceivingReply = 0;

    m_RemotingServerInvocationStarted = 0;
    m_RemotingServerInvocationReturned = 0;
    m_RemotingServerSendingReply = 0;
    m_RemotingServerReceivingMessage = 0;

    // suspension counter array
    m_ForceGCEventCounter = 0;
    m_dwForceGCSucceeded = 0;

    // enter-leave counters
    m_FunctionEnter = 0;
    m_FunctionLeave = 0;
    m_FunctionTailcall = 0;

    m_FunctionEnter2 = 0;
    m_FunctionLeave2 = 0;
    m_FunctionTailcall2 = 0;

    m_FunctionIDMapper = 0;

	// ELT3 FastPath Counters
	m_FunctionEnter3 = 0;
	m_FunctionLeave3 = 0;
	m_FunctionTailCall3 = 0;

	// ELT3 SlowPath Counters
	m_FunctionEnter3WithInfo = 0;
	m_FunctionLeave3WithInfo = 0;
	m_FunctionTailCall3WithInfo = 0;

	// FunctionIDMapper2 counter
	m_FunctionIDMapper2 = 0;

    m_GarbageCollectionStarted = 0;
    m_GarbageCollectionFinished = 0;
    m_FinalizeableObjectQueued = 0;
    m_RootReferences2 = 0;
    m_HandleCreated = 0;
    m_HandleDestroyed = 0;
    m_SurvivingReferences = 0;

    // Keep track of active callbacks to prepare for Hard Suspend
    m_ActiveCallbacks = 0;

#pragma endregion // Callback Counter Initialization

	m_ulError = 0;
}

PrfCommon::~PrfCommon()
{
    if (m_pInfo)
    {
        m_pInfo->Release();
        m_pInfo = NULL;
    }

    if (m_pInfo4)
    {
        m_pInfo4->Release();
        m_pInfo4 = NULL;
    }
}

// Called by Asynchronous satellite to setup TestProfiler for Asynchronous Profiling
HRESULT PrfCommon::InitializeForAsynch()
{
   m_fEnableAsynch = TRUE;
   return S_OK;
}


// Called by Asynchronous satellite to signal request for Hard Suspend.  Function returns when
// TestProfiler is ready for a Hard Suspend.
 HRESULT PrfCommon::RequestHardSuspend()
{
    lock_guard<recursive_mutex> guard(m_asynchronousCriticalSection);

    // ResetEvent to nonsignaled state.  Callbacks wait on this event.  Signaled allows execution.
    m_asynchEvent->Reset();

    // Wait for threads that started execution before signal to return
    while(m_ActiveCallbacks != 0)
    {
        YieldProc();
    }

    return S_OK;
}


// Called by Asynchronous satellite to signal HardSuspend is over.
HRESULT PrfCommon::ReleaseHardSuspend()
{
    // Set HardSuspend event so waiting and new TestProfiler threads can continue
    m_asynchEvent->Signal();
    
    return S_OK;
}

VOID PrfCommon::Display(std::wstringstream& sstrm)
{
    lock_guard<mutex> guard(m_outputMutex);

	sstrm << L"\n";
    wprintf(L"%s", sstrm.str().c_str());
#ifdef _WIN32
    OutputDebugStringW(sstrm.str().c_str());
#endif // _WIN32
	//sstrm.str(L"");
	sstrm.clear();
}

//
// Prints error message in the string stream to the console window. Also launches failure dialog with error
// message
//
VOID PrfCommon::Error(std::wstringstream& sstrm)
{
	m_ulError++;

    // Create title string with key word for DHandler and process/thread info
    // Log error to output file
    std::wstringstream prefix;
    prefix << L"\nERROR: Test Failure: ";
    Display(prefix);
    
	Display(sstrm);
	//sstrm.str(L"");
	sstrm.clear();
}


HRESULT PrfCommon::RemoveEvent(DWORD eventM)
{
    HRESULT hr = S_OK;
    DWORD eventMask;

    hr = PINFO->GetEventMask(&eventMask);
    if (FAILED(hr))
        return hr;

    eventMask = ~((~eventMask) ^ eventM);
    hr = PINFO->SetEventMask(eventMask | COR_PRF_MONITOR_APPDOMAIN_LOADS);

    return hr;
}

HRESULT PrfCommon::GetAppDomainIDName(AppDomainID appdomainId, wstring &name, const BOOL full)
{
    PROFILER_WCHAR appDomainNameTemp[STRING_LENGTH];
    PLATFORM_WCHAR appDomainName[STRING_LENGTH];
    ULONG nameLength = 0;
    ProcessID procId = 0;

    if (appdomainId == NULL)
    {
        return E_FAIL;
    }

    MUST_PASS(PINFO->GetAppDomainInfo(appdomainId,
                                      STRING_LENGTH,
                                      &nameLength,
                                      appDomainNameTemp,
                                      &procId));

    ConvertProfilerWCharToPlatformWChar(appDomainName, STRING_LENGTH, appDomainNameTemp, STRING_LENGTH);
    if (full)
    {
        wchar_t procStr[STRING_LENGTH];
        swprintf_s(procStr, STRING_LENGTH, L"%u", (ULONG)procId);
        name += L"0x";
        name += procStr;
        name += L":";
    }

    name += appDomainName;
    return S_OK;
}

HRESULT PrfCommon::GetAssemblyIDName(AssemblyID assemblyId, wstring &name, const BOOL full)
{
    PROFILER_WCHAR assemblyNameTemp[STRING_LENGTH];
    PLATFORM_WCHAR assemblyName[STRING_LENGTH];
    AppDomainID appID = 0;
    ULONG nameLength = 0;

    if (assemblyId == NULL)
    {
        return E_FAIL;
    }

    MUST_PASS(PINFO->GetAssemblyInfo(assemblyId,
                                     STRING_LENGTH,
                                     &nameLength,
                                     assemblyNameTemp,
                                     &appID,
                                     NULL));
    
    ConvertProfilerWCharToPlatformWChar(assemblyName, STRING_LENGTH, assemblyNameTemp, STRING_LENGTH);
    
    if (full)
    {
        MUST_PASS(GetAppDomainIDName(appID, name, full));
        name += L"-";
    }

    name += assemblyName;
    return S_OK;
}


HRESULT PrfCommon::GetModuleIDName(ModuleID modId, wstring &name, BOOL const full)
{
    PROFILER_WCHAR moduleNameTemp[STRING_LENGTH];
    PLATFORM_WCHAR moduleName[STRING_LENGTH];
    ULONG nameLength = 0;
    AssemblyID assemID;

    if (modId == NULL)
    {
        return E_FAIL;
    }

    MUST_PASS(PINFO->GetModuleInfo(modId,
                                   NULL,
                                   STRING_LENGTH,
                                   &nameLength,
                                   moduleNameTemp,
                                   &assemID));

    ConvertProfilerWCharToPlatformWChar(moduleName, STRING_LENGTH, moduleNameTemp, STRING_LENGTH);

    if (full)
    {
        MUST_PASS(GetAssemblyIDName(assemID, name, full));
        name += L"!";
    }

    name += moduleName;
    return S_OK;
}


HRESULT PrfCommon::GetClassIDName(ClassID classId, wstring &name, const BOOL full)
{
    ModuleID modId;
    mdTypeDef classToken;
    ClassID parentClassID;
    ULONG32 nTypeArgs;
    ClassID typeArgs[SHORT_LENGTH];
    HRESULT hr = S_OK;

    if (classId == NULL)
    {
        return E_FAIL;
    }

    hr = PINFO->GetClassIDInfo2(classId,
                                &modId,
                                &classToken,
                                &parentClassID,
                                SHORT_LENGTH,
                                &nTypeArgs,
                                typeArgs);
    if (CORPROF_E_CLASSID_IS_ARRAY == hr)
    {
        // We have a ClassID of an array.
        name += L"ArrayClass";
        return S_OK;
    }
    else if (CORPROF_E_CLASSID_IS_COMPOSITE == hr)
    {
        // We have a composite class
        name += L"CompositeClass";
        return S_OK;
    }
    else if (CORPROF_E_DATAINCOMPLETE == hr)
    {
        // type-loading is not yet complete. Cannot do anything about it.
        name += L"DataIncomplete";
        return S_OK;
    }
    else if (FAILED(hr))
    {
        FAILURE(L"GetClassIDInfo returned " << HEX(hr) << L" for ClassID " << HEX(classId));
        return hr;
    }

    COMPtrHolder<IMetaDataImport> pMDImport;
    MUST_PASS(PINFO->GetModuleMetaData(modId,
                                       (ofRead | ofWrite),
                                       IID_IMetaDataImport,
                                       (IUnknown **)&pMDImport ));


    PROFILER_WCHAR wNameTemp[STRING_LENGTH];
    PLATFORM_WCHAR wName[STRING_LENGTH];
    DWORD dwTypeDefFlags = 0;
    MUST_PASS(pMDImport->GetTypeDefProps(classToken,
                                         wNameTemp,
                                         STRING_LENGTH,
                                         NULL,
                                         &dwTypeDefFlags,
                                         NULL));

    ConvertProfilerWCharToPlatformWChar(wName, STRING_LENGTH, wNameTemp, STRING_LENGTH);

    if (full)
    {
        MUST_PASS(GetModuleIDName(modId, name, full));
        name += L".";
    }

    name += wName;
    if (nTypeArgs > 0)
        name += L"<";

    for(ULONG32 i = 0; i < nTypeArgs; i++)
    {

        wstring typeArgClassName;
        typeArgClassName.clear();
        MUST_PASS(GetClassIDName(typeArgs[i], typeArgClassName, FALSE));
        name += typeArgClassName;

        if ((i + 1) != nTypeArgs)
            name += L", ";
    }

    if (nTypeArgs > 0)
        name += L">";

    return hr;
}


HRESULT PrfCommon::GetFunctionIDName(FunctionID funcId, wstring &name, const COR_PRF_FRAME_INFO frameInfo, const BOOL full)
{
    // If the FunctionID is 0, we could be dealing with a native function.
    if (funcId == NULL)
    {
        name += L"Unknown_Native_Function";
        return S_OK;
    }

    ClassID classId = NULL;
    ModuleID moduleId = NULL;
    mdToken token = NULL;
    ULONG32 nTypeArgs = NULL;
    ClassID typeArgs[SHORT_LENGTH];

    MUST_PASS(PINFO->GetFunctionInfo2(funcId,
                                      frameInfo,
                                      &classId,
                                      &moduleId,
                                      &token,
                                      SHORT_LENGTH,
                                      &nTypeArgs,
                                      typeArgs));

    COMPtrHolder<IMetaDataImport> pIMDImport;
    MUST_PASS(PINFO->GetModuleMetaData(moduleId,
                                       ofRead,
                                       IID_IMetaDataImport,
                                       (IUnknown **)&pIMDImport));

    PROFILER_WCHAR funcNameTemp[STRING_LENGTH];
    PLATFORM_WCHAR funcName[STRING_LENGTH];
    MUST_PASS(pIMDImport->GetMethodProps(token,
                                         NULL,
                                         funcNameTemp,
                                         STRING_LENGTH,
                                         0,
                                         0,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL));

    ConvertProfilerWCharToPlatformWChar(funcName, STRING_LENGTH, funcNameTemp, STRING_LENGTH);

    if (full)
    {
        wstring className;

        // If the ClassID returned from GetFunctionInfo is 0, then the function is a shared generic function.
        if (classId != 0)
        {
            MUST_PASS(GetClassIDName(classId, className, full));
        }
        else
        {
            className = L"SharedGenericFunction";
        }

        name += className;
        name += L"::";
    }

    name += funcName;

    // Fill in the type parameters of the generic method
    if (nTypeArgs > 0)
        name += L"<";

    for(ULONG32 i = 0; i < nTypeArgs; i++)
    {
        wstring typeArgClassName;
        typeArgClassName.clear();
        MUST_PASS(GetClassIDName(typeArgs[i], typeArgClassName, FALSE));
        name += typeArgClassName;

        if ((i + 1) != nTypeArgs)
            name += L", ";
    }

    if (nTypeArgs > 0)
        name += L">";

    return S_OK;
}


BOOL PrfCommon::IsFunctionStatic(FunctionID funcId, const COR_PRF_FRAME_INFO frameInfo)
{
    if (funcId == NULL)
    {
        FAILURE(L"NULL FunctionID passed to IsFunctionStatic!\n");
        return FALSE;
    }

    ModuleID moduleId = NULL;
    mdToken token = NULL;
    MUST_PASS(PINFO->GetFunctionInfo2(funcId,
                                      frameInfo,
                                      NULL,
                                      &moduleId,
                                      &token,
                                      NULL,
                                      NULL,
                                      NULL));

    COMPtrHolder<IMetaDataImport> pIMDImport;
    MUST_PASS(PINFO->GetModuleMetaData(moduleId,
                                       ofRead,
                                       IID_IMetaDataImport,
                                       (IUnknown **)&pIMDImport));

    DWORD methodAttr = NULL;
    MUST_PASS(pIMDImport->GetMethodProps(token,
                                         NULL,
                                         NULL,
                                         NULL,
                                         0,
                                         &methodAttr,
                                         NULL,
                                         NULL,
                                         NULL,
                                         NULL));

    if (IsMdStatic(methodAttr))
        return TRUE;
    else
        return FALSE;
}

// Return TRUE iff wszContainer ends with wszProspectiveEnding (case-insensitive)
BOOL ContainsAtEnd(const PLATFORM_WCHAR *wszContainer, const PLATFORM_WCHAR *wszProspectiveEnding)
{
    size_t cchContainer = wcslen(wszContainer);
    size_t cchEnding = wcslen(wszProspectiveEnding);

    if (cchContainer < cchEnding)
        return FALSE;

    if (cchEnding == 0)
        return FALSE;

    return StrCmpIns(wszProspectiveEnding, &wszContainer[cchContainer - cchEnding]);
}
#pragma warning(pop)
