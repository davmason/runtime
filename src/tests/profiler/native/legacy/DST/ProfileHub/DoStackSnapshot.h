// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "../../ProfilerCommon.h"

#if defined(__linux__) || defined(OSX)
#include <sys/mman.h>
#include <vector>
#include <thread>
#include <array>
#endif // __linux__ 


// Modified print macros for use in DoStackSnapshot.cpp
#undef DISPLAY
#undef LOG
#undef FAILURE
#undef VERBOSE

#define DISPLAY( message )	{																		\
                                std::wstringstream temp;                                            \
                                temp << message;                                                    \
								m_pPrfCom->Display(temp);					                        \
							}

#define FAILURE( message )	{																		\
                                std::wstringstream temp;                                            \
                                temp << message;                                                    \
								m_pPrfCom->Error(temp) ;						                    \
							}

#undef  PINFO
#define PINFO m_pPrfCom->m_pInfo
#undef  PPRFCOM
#define PPRFCOM m_pPrfCom


/*
 *  Used to keep track of full name of frame for Shadow/Snapshot stacks
 */
struct FunctionNameAndInfo
{
    wstring funcName;
    FunctionID funcId;

    /*
     * It may be interesting at some point to track additional info about the functions
     *
        ULONG enter;
        ULONG leave;
        ULONG tailcall;
        ULONG exLeave;
    */
};


/*
 *  Used to keep track of interesting register state of current state of frames
 *  in Snapshot stack
 */
struct FunctionIDAndRegisterInfo
{
    FunctionID funcId;

#ifdef THREAD_HIJACK
    // Keep interesting register info
    #if defined(_X86_)
        DWORD EIP;
        DWORD ESP;
		DWORD EBP;

    #elif defined(_IA64_)
        ULONGLONG StIIP;
        ULONGLONG StSP;

    #elif defined(_AMD64_)
        ULONG64 Rip;
        ULONG64 Rsp;
    #endif
#endif // THREAD_HIJACK

	BOOL operator==(const FunctionIDAndRegisterInfo& r)
	{
		return (funcId == r.funcId
#ifdef THREAD_HIJACK
#if defined(_X86_)
			&& EIP == r.EIP
			&& ESP == r.ESP
			&& EBP == r.EBP
#elif defined(_IA64_)
			&& StIIP == r.StIIP
			&& StSP == r.StSP
#elif defined(_AMD64_)
			&& Rip == r.Rip
			&& Rsp == r.Rsp
#endif
#endif // THREAD_HIJACK
			);

	}

	BOOL operator!=(const FunctionIDAndRegisterInfo& r)
	{
		return !(*this==r);
	}
};
inline BOOL operator==(const FunctionID& l, const FunctionIDAndRegisterInfo& r)
{
	return l == r.funcId;
}
inline BOOL operator!=(const FunctionID& l, const FunctionIDAndRegisterInfo& r)
{
	return !(l==r);
}

class Semaphore
{
private:
    std::mutex m_mtx;
    std::condition_variable m_cv;
    std::atomic<long> m_count;

public:
    Semaphore() = default;
    ~Semaphore() = default;
    Semaphore(Semaphore& other) = delete;
    Semaphore(Semaphore&& other) = delete;
    Semaphore& operator= (Semaphore& other) = delete;
    Semaphore& operator= (Semaphore&& other) = delete;

    void Take()
    {
        std::unique_lock<mutex> lock(m_mtx);
        while(m_count <= 0)
        {
            m_cv.wait(lock);
        }

        --m_count;
    }

    BOOL TryTake()
    {
        std::unique_lock<mutex> lock(m_mtx);
        if (m_count > 0)
        {
            --m_count;
            return TRUE;
        }

        return FALSE;
    }

    void Release()
    {
        std::unique_lock<mutex> lock(m_mtx);
        ++m_count;
        m_cv.notify_one();
    }
};

#ifdef WIN32
//
// Class that maintains a private heap for STL containers
//
class PrivateHeap
{
public :
    PrivateHeap()
    {
        m_hHeap = ::HeapCreate(0, 0, 0);
    }

    ~PrivateHeap()
    {
        ::HeapDestroy(m_hHeap);
    }

    HANDLE GetHeap()
    {
        return m_hHeap;
    }

private :
    HANDLE m_hHeap;
};

//
// Private heap instance
//
extern PrivateHeap g_privateHeap;
#endif // WIN32

//
// STL-compatible allocator that use the private heap
// By default VC now use the default process heap which will
// conflict with the LockHeapThread as well as the profilee
// Using a private heap could avoid potential deadlock
//
template<class Ty>
    class heap_allocator {
#if defined(__linux__) || defined(OSX)
    static constexpr size_t PageSize = 4096;
    char *m_base;
    char *m_nextFree;
#endif // __linux__
public:
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef Ty *pointer;
    typedef const Ty *const_pointer;
    typedef Ty& reference;
    typedef const Ty& const_reference;
    typedef Ty value_type;

public :
    heap_allocator()
    {
#if defined(__linux__) || defined(OSX)
        m_base = reinterpret_cast<char *>(mmap(
                                                NULL, 
                                                PageSize, 
                                                PROT_READ | PROT_WRITE, 
                                                MAP_ANONYMOUS, 
                                                -1, 
                                                0
                                            ));

        if (m_base == MAP_FAILED)
        {
            throw "failed to alloc heap";
        }

        m_nextFree = m_base;
#endif // __linux__
    }

    ~heap_allocator()
    {
#if defined(__linux__) || defined(OSX)
        munmap(m_base, PageSize);
#endif // __linux__
    }    

    pointer address(reference val) const
    {
        return &val;
    }

    const_pointer address(const_reference val) const
    {
        return &val;
    }

    template<class Other>
    struct rebind
    {
        // convert a name<_Ty> to a name<_Other> */
	    typedef heap_allocator<Other> other;
    };
    
    template<class Other>
    heap_allocator(const heap_allocator<Other>& right) throw()
    {
    }

    heap_allocator(const heap_allocator& right) throw()
    {
    }

    template<class Other>
    heap_allocator& operator=(const heap_allocator<Other>& right)
    {
        return *this;
    }

    
    heap_allocator& operator=(const heap_allocator& right)
    {
        return *this;
    }
    

    pointer allocate(size_type count)
    {
#ifdef WIN32
		return reinterpret_cast<pointer>(HeapAlloc(g_privateHeap.GetHeap(), HEAP_ZERO_MEMORY, count * sizeof(Ty)));
#else // WIN32
        size_t allocSize = (count * sizeof(Ty)) / sizeof(char);
        void *retValue = reinterpret_cast<void *>(m_nextFree);
        m_nextFree += allocSize;
        return reinterpret_cast<pointer>(retValue);
#endif // WIN32
    }

    void deallocate(pointer ptr, size_type count)        
    {
#ifdef WIN32
        ::HeapFree(g_privateHeap.GetHeap(), 0, ptr);
#else // WIN32
        // Do nothing
#endif // WIN32
    }

    void construct(pointer ptr, const Ty& val)
    {
        new (ptr) Ty(val);
    }

    void destroy(pointer ptr)
    {
        ptr->Ty::~Ty();
    }

    size_type max_size() const throw()
    {
		return ((size_t)(-1) / sizeof (Ty));
    }

    inline bool operator==(heap_allocator const&) { return true; }
    inline bool operator!=(heap_allocator const& a) { return !operator==(a); }
};

/*
 *  STL containers used to keep track of FunctionID's and related info
 */
typedef map<FunctionID, FunctionNameAndInfo *>ThreadFunctionMap;
typedef vector<FunctionID> FunctionStack;

// Need to use heap_allocator (which use the private heap) as we call push_back in StackSnapshotCallStack with other thread suspended
typedef vector<FunctionIDAndRegisterInfo, heap_allocator<FunctionIDAndRegisterInfo> > FunctionAndRegStack;


/*
 *  Main data structure used on a per-thread basis.
 */
struct ThreadInfoAndStacks
{
    // Shadow and Function Stacks
    FunctionAndRegStack snapStack;
    FunctionAndRegStack ebpStack;
    FunctionStack shadowStack;
    ThreadFunctionMap functionMap;

    // Thread info for easier debugging
    ThreadID threadId;
#ifdef THREAD_HIJACK
    HANDLE threadHandle;
#endif // THREAD_HIJACK
    DWORD win32threadId;
};


/*
 *  STL containers used to keep track of ThreadID's and related info
 */
typedef map<ThreadID, ThreadInfoAndStacks *> ThreadInfoStackMap;
typedef map<ThreadID, FunctionStack *> ThreadStackMap;
#ifdef THREAD_HIJACK
typedef map<ThreadID, HANDLE> ThreadHandleMap;
#endif // THREAD_HIJACK


/*
 *  Enums defining how DoStackSnapshot is to be tested.
 */
enum SynchronousCallMethod { NO_SYNCH, CURRENT_THREAD, CROSS_THREAD, SYNC_ASP };
enum AsynchronousSampleMethod { NO_ASYNCH, SAMPLE, SAMPLE_AND_TEST_DEADLOCK, HIJACK, HIJACKANDINSPECT, ASYNC_AVAILABILITY };


/*                     
 *  Timeout used to determine if a hijack has been lost.
 */
#define WAIT_FOR_SINGLE_OBJECT_TIMEOUT 30000L


//
// Helper thread that can lock the process default heap in a different thread
//
#ifdef WIN32
class LockHeapThread
{
public :
    LockHeapThread()
    {
        m_hHeapLocked = NULL;
        m_hUnlockHeap = NULL;
        m_hLockHeapThread = NULL;
    }

    BOOL Initialize()
    {
        m_hHeapLocked = ::CreateEvent(NULL, TRUE, FALSE, NULL);
        if (m_hHeapLocked == NULL)
        {
            return FALSE;
        }

        m_hUnlockHeap = ::CreateEvent(NULL, TRUE, FALSE, NULL);
        if (m_hUnlockHeap == NULL)
        {
            return FALSE;
        }

        return TRUE;
    }

    BOOL LockHeap()
    {
        // Heap not unlocked
        if (m_hLockHeapThread != NULL)
        {
            return FALSE;
        }

        ::ResetEvent(m_hHeapLocked);
        ::ResetEvent(m_hUnlockHeap);

        m_hLockHeapThread = ::CreateThread(NULL, 0, LockHeapThreadProc, this, 0, NULL);
        
        ::WaitForSingleObject(m_hHeapLocked, INFINITE);

        return TRUE;
    }

    BOOL UnlockHeap()
    {
        if (m_hLockHeapThread == NULL)
        {
            return FALSE;
        }

        ::SetEvent(m_hUnlockHeap);

        ::WaitForSingleObject(m_hLockHeapThread, INFINITE);
        ::CloseHandle(m_hLockHeapThread);
        m_hLockHeapThread = NULL;

        return TRUE;
    }

private :
    static DWORD WINAPI LockHeapThreadProc(LPVOID lpParam)
    {
        LockHeapThread *pThis = reinterpret_cast<LockHeapThread *>(lpParam);

        // Lock the heap
        ::HeapLock(::GetProcessHeap());

        ::SetEvent(pThis->m_hHeapLocked);

        ::WaitForSingleObject(pThis->m_hUnlockHeap, INFINITE);

        // Unlock the heap
        ::HeapUnlock(::GetProcessHeap());

        return 0;
    }

private :
    HANDLE m_hHeapLocked;
    HANDLE m_hUnlockHeap;
    HANDLE m_hLockHeapThread;
};
#endif // WIN32

#if defined(__linux__) || defined(OSX)
// On Linux there isn't a way to lock the heap, so create a bunch of threads that constantly allocate
// to try to simulate allocation contention.
class LockHeapThread
{
public :
    LockHeapThread()
    {
    }

    BOOL Initialize()
    {
        keepRunning = TRUE;

        return TRUE;
    }

    BOOL LockHeap()
    {
        auto func = [&] (int threadNum) {
            while(keepRunning.load())
            {
                storage[threadNum] = new int(5);
                delete storage[threadNum];
            }
        };
        
        for(int i = 0; i < NumThreads; ++i)
        {
            std::shared_ptr<std::thread> t(new thread(func, i));
            
            threads.push_back(t);
        }

        return TRUE;
    }

    BOOL UnlockHeap()
    {
        keepRunning = FALSE;

        for(auto&& t : threads)
        {
            t->join();
        }

        return TRUE;
    }

private :
    static constexpr int NumThreads = 10;

    std::atomic<BOOL> keepRunning;
    std::array<int *, NumThreads> storage;
    std::vector<std::shared_ptr<std::thread>> threads;
};
#endif // __linux__

/******************************************************************************
 *
 *  DoStackSnapshot Class:
 *
 *  Used to test ICorProfilerInfo2::DoStackSnapshot in the following scenarios.
 *
 *      -DSS called on the current thread from a profiler callback.
 *      -DSS called cross thread from a profiler callback.
 *      -DSS called from an asynchronous unmanaged thread (Sampling)
 *      -DSS called from a HiJacked Thread
 *      -DSS called cross thread from a HiJacked Thread
 *      -Measure DSS availability when called from asynchronous unmanaged thread (Sampling)
 *
 *****************************************************************************/

class DoStackSnapshot
{
    public:

	DoStackSnapshot(IPrfCom *pPrfCom,
                    SynchronousCallMethod requestedCallMethod,
                    AsynchronousSampleMethod requestedSampleMethod,
                    ULONG minimumAvailabilityRate);

    ~DoStackSnapshot();

    /**************************************************************************
     *
     *  Profiler callbacks maintain correct Shadow Stacks -Used to verify Snapshot Stacks
     *
     *************************************************************************/

    HRESULT AddFunctionToShadowStack(FunctionID mappedFuncId);

    HRESULT RemoveFunctionFromShadowStack(FunctionID mappedFuncId,
                                         BOOL bUnwind);
                                         
    HRESULT RemoveExceptionFromShadowStack();

    HRESULT ThreadCreated(ThreadID managedThreadId);
        
    HRESULT ThreadDestroyed(ThreadID managedThreadId);

    HRESULT ModuleLoadFinished(ModuleID moduleId, HRESULT hrStatus);

    // static methods

    static HRESULT FunctionEnter3WithInfoWrapper(IPrfCom * pPrfCom,
		FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO /*eltInfo*/)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->AddFunctionToShadowStack(functionIDOrClientID.functionID);
    }

    static HRESULT FunctionTailCall3WithInfoWrapper(IPrfCom * pPrfCom,
		FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO /*eltInfo*/)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->RemoveFunctionFromShadowStack(functionIDOrClientID.functionID, FALSE);
    }

    static HRESULT FunctionLeave3WithInfoWrapper(IPrfCom * pPrfCom,
		FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO /*eltInfo*/)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->RemoveFunctionFromShadowStack(functionIDOrClientID.functionID, FALSE);
    }

    static HRESULT ExceptionSearchFilterEnterWrapper(IPrfCom * pPrfCom,
                FunctionID functionId)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->AddFunctionToShadowStack(functionId);
    }

    static HRESULT ExceptionSearchFilterLeaveWrapper(IPrfCom * pPrfCom)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->RemoveExceptionFromShadowStack();
    }

    static HRESULT ExceptionUnwindFunctionLeaveWrapper(IPrfCom * pPrfCom)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->RemoveExceptionFromShadowStack();
    }

    static HRESULT ThreadCreatedWrapper(IPrfCom * pPrfCom,
                    ThreadID managedThreadId)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->ThreadCreated(managedThreadId);
    }

    static HRESULT ThreadDestroyedWrapper(IPrfCom * pPrfCom,
                    ThreadID managedThreadId)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->ThreadDestroyed(managedThreadId);
    }

	static HRESULT ModuleLoadFinishedWrapper(IPrfCom *pPrfCom,
											ModuleID moduleId,
											HRESULT hrStatus)
	{
		return STATIC_CLASS_CALL(DoStackSnapshot)->ModuleLoadFinished(moduleId,hrStatus);
	}
					
    /**************************************************************************
     *
     *  Profiler callbacks to do Synchronous DSS.  Only used when not providing Async
     *  support to a Sample or Hijacking extension.
     *
     *************************************************************************/

    #pragma region static_wrapper_methods

    static HRESULT AssemblyLoadStartedWrapper(IPrfCom * pPrfCom,
                                            AssemblyID /*assemblyId*/)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->InvokeDoStackSnapshotWrapper(pPrfCom);
    }

    static HRESULT ClassLoadStartedWrapper(IPrfCom * pPrfCom,
                                            ClassID /*classId*/)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->InvokeDoStackSnapshotWrapper(pPrfCom);
    }

    static HRESULT ExceptionThrownWrapper(IPrfCom * pPrfCom,
                                            ObjectID /*thrownObjectId*/)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->InvokeDoStackSnapshotWrapper(pPrfCom);
    }

    static HRESULT GarbageCollectionStartedWrapper(IPrfCom * pPrfCom,
                                            INT /*cGenerations*/,
                                            BOOL /*generationCollected*/[],
                                            COR_PRF_GC_REASON /*reason*/)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->InvokeDoStackSnapshotWrapper(pPrfCom);
    }

    static HRESULT HandleCreatedWrapper(IPrfCom * pPrfCom,
                                            GCHandleID /*handleId*/,
                                            ObjectID /*initialObjectId*/)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->InvokeDoStackSnapshotWrapper(pPrfCom);
    }

    static HRESULT HandleDestroyedWrapper(IPrfCom * pPrfCom,
                                            GCHandleID /*handleId*/)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->InvokeDoStackSnapshotWrapper(pPrfCom);
    }

    static HRESULT JITCachedFunctionSearchStartedWrapper(IPrfCom * pPrfCom,
                                            FunctionID /*functionId*/,
                                            BOOL * /*pbUseCachedFunction*/)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->InvokeDoStackSnapshotWrapper(pPrfCom);
    }

    static HRESULT ManagedToUnmanagedTransitionWrapper(IPrfCom * pPrfCom,
                                            FunctionID /*functionId*/,
                                            COR_PRF_TRANSITION_REASON /*reason*/)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->InvokeDoStackSnapshotWrapper(pPrfCom);
    }

    static HRESULT ModuleLoadStartedWrapper(IPrfCom * pPrfCom,
                                            ModuleID /*moduleId*/)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->InvokeDoStackSnapshotWrapper(pPrfCom);
    }

    static HRESULT ObjectAllocatedWrapper(IPrfCom * pPrfCom,
                                            ObjectID /*objectId*/,
                                            ClassID /*classId*/)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->InvokeDoStackSnapshotWrapper(pPrfCom);
    }

    static HRESULT UnmanagedToManagedTransitionWrapper(IPrfCom * pPrfCom,
                                            FunctionID /*functionId*/,
                                            COR_PRF_TRANSITION_REASON /*reason*/)
    {
        return STATIC_CLASS_CALL(DoStackSnapshot)->InvokeDoStackSnapshotWrapper(pPrfCom);
    }

    #pragma endregion


    static HRESULT STDMETHODCALLTYPE InvokeDoStackSnapshotWrapper(IPrfCom * pPrfCom)
    {
        LOCAL_CLASS_POINTER(DoStackSnapshot);

        if (pDoStackSnapshot->m_callMethod == CURRENT_THREAD)
            return pDoStackSnapshot->SnapshotOneThread(NULL);

		else if (pDoStackSnapshot->m_callMethod == CROSS_THREAD)
            return pDoStackSnapshot->SnapshotAllThreads();

		else if (pDoStackSnapshot->m_callMethod == SYNC_ASP)
            return pDoStackSnapshot->MeasureDSSAvailability(NULL);

		else
            return E_FAIL;
    }


    static HRESULT JITCompilationStartedWrapper(IPrfCom * pPrfCom,
                    FunctionID /*functionId*/,
                    BOOL fIsSafeToBlock)
    {
        if (fIsSafeToBlock)
            return InvokeDoStackSnapshotWrapper(pPrfCom);
        else
            return S_OK;
    }


    /**************************************************************************
     *
     *  ThreadSuspension are tracked to know when it is safe to HiJack and to synchronize
     *  profiler hijacks and GC thread hijacks.
     *
     *************************************************************************/

    static HRESULT RuntimeSuspendStarted(IPrfCom * pPrfCom)
    {
#ifdef THREAD_HIJACK
        STATIC_CLASS_CALL(DoStackSnapshot)->ProhibitHijackAndWait();
#endif
        return S_OK;
    }

    static HRESULT RuntimeResumeStarted(IPrfCom * pPrfCom)
    {
#ifdef THREAD_HIJACK
        STATIC_CLASS_CALL(DoStackSnapshot)->AllowHijack();
#endif
        return S_OK;
    }

    /**************************************************************************
     *
     *  Functions for invoking DoStackSnapshot and evaluating the resulting Snapshot stack
     *
     *************************************************************************/

    HRESULT SnapshotAllThreads();

    HRESULT SnapshotOneThread(ThreadID threadId);

    HRESULT StackSnapshotCallback(
                    FunctionID funcId,
                    UINT_PTR ip,
                    COR_PRF_FRAME_INFO frameInfo,
                    ULONG32 contextSize,
                    BYTE context[],
                    void *clientData);

    HRESULT StackEbpCallback(
                    FunctionID funcId,
                    UINT_PTR ip,
                    COR_PRF_FRAME_INFO frameInfo,
                    ULONG32 contextSize,
                    BYTE context[],
                    void *clientData);

    BOOL GetThreadInfoAndStack(ThreadID threadId,
                    ThreadInfoAndStacks *threadInfo);

    HRESULT CompareStacks(ThreadID threadId,
                          FunctionStack *shadowStack,
                          FunctionAndRegStack *snapshotStack,
                          FunctionAndRegStack *ebpStack,
                          FunctionAndRegStack *snapshotRegInfoStack,
                          ThreadFunctionMap *functionMap);
    
    HRESULT CompareShadowAndSnapshotStacks(ThreadID threadId,
                    FunctionStack *shadowStack,
                    FunctionAndRegStack *snapshotStack,
                    FunctionAndRegStack *snapStackRegInfo,
                    ThreadFunctionMap *functionMap);

    HRESULT CompareShadowAndEbpStacks(ThreadID threadId,
                                      FunctionStack *shadowStack,
                                      FunctionAndRegStack *ebpStack,
                                      FunctionAndRegStack *snapshotRegInfoStack,
                                      ThreadFunctionMap *functionMap);
    HRESULT CompareSnapshotAndEbpStacks(ThreadID threadId,
                                        FunctionAndRegStack *snapStack,
                                        FunctionAndRegStack *ebpStack,
                                        FunctionAndRegStack *snapshotRegInfoStack,
                                        ThreadFunctionMap *functionMap);
    
    HRESULT DisplayStacks(ThreadID threadId,
                    FunctionStack *shadowStack,
                    FunctionAndRegStack *snapshotStack,
                    FunctionAndRegStack *ebpStack,
                    FunctionAndRegStack *snapStackRegInfo,
                    ThreadFunctionMap *functionMap);

    HRESULT MeasureDSSAvailability(ThreadID threadId);


    /**************************************************************************
     *
     *  Functions for supporting an Asynchronous DoStackSnapshot
     *
     *************************************************************************/

    VOID StartSampling();
    VOID StopSampling();
    VOID SampleNow();
    static VOID SampleThreadFuncHelper(DoStackSnapshot *dss);
    static VOID SampleNowThreadFuncHelper(DoStackSnapshot *dss);
    static VOID StopThreadFuncHelper(DoStackSnapshot *dss);
    VOID SampleThreadFunc();
#ifdef THREAD_HIJACK
    VOID SampleNowThreadFunc();
#endif // THREAD_HIJACK
    VOID StopThreadFunc();
    VOID SampleAllThreads();
#ifdef THREAD_HIJACK
    VOID HiJackThread(ThreadID threadId, HANDLE threadHandle);
    VOID HiJackSignalDone() { m_hiJackSignal.Signal(); }
#endif // THREAD_HIJACK

    // HiJack State
    BOOL IsHijackAllowed();
    VOID HijackFinished();
    VOID ProhibitHijackAndWait();
    VOID AllowHijack();


    /**************************************************************************
     *
     *  DoStackSnapshot member function and variables.
     *
     *************************************************************************/

    HRESULT Verify();

    static DoStackSnapshot* Instance()
    {
        return This;
    }

    /*  This pointer used by static methods to find instance of class */
    static DoStackSnapshot* This;

    /*  Used to keep pointer to PrfCommon routines and ICorProfilerInfo */
    IPrfCom *m_pPrfCom;

    /*  Current method of synchronous and asynchronous testing. */
    AsynchronousSampleMethod m_sampleMethod;
    SynchronousCallMethod    m_callMethod;

    /*  Handle to asynchronous sampling thread. */
    BOOL m_asynchThread;
    std::atomic<BOOL> m_sampleThreadRunning;
#ifdef THREAD_HIJACK
    std::atomic<BOOL> m_sampleNowThreadRunning;
#endif // THREAD_HIJACK
    std::atomic<BOOL> m_stopThreadRunning;

    /*  Main data store for information on threads and their stacks */
    ThreadInfoStackMap m_threadInfoStackMap;

    /*
     *  Synchronization object used by
     *      AsynchThreadFunc()
     *      SnapshotAllThreads()
     *      ThreadCreated()
     *      ThreadDestoryed()
     *      Verify()
     *
     *  In the cross thread cases, this allows synchronization without blocking, while the other asynchronous
     *  and thread functions can block.
     *
     *  When multiple profiler callbacks want to sample cross thread, they can not block if another thread is
     *  already sampling.  If they do block they can introduce a deadlock if the runtime attempts to suspend
     *  the tread for a GC or AppDomain unload.  To avoid this, the test uses a Semaphore with a count of 1.
     *  If a thread wants to sample and can not take the Semaphore, it must return immediately.
     */
    Semaphore m_semaphore;

    /*  Used to get current shadow stack for thread being sampled. */
    BOOL m_bFirstDSSCallBack;
    ThreadID m_tCurrentDSSThread;
    ThreadInfoAndStacks m_tsThreadInfo;

#ifdef THREAD_HIJACK
    /*  HiJacking Variables */
    BYTE m_threadContext[sizeof(CONTEXT)];

    #if defined(_X86_)
        DWORD m_originalEIP;
        DWORD m_originalESP;

    #elif defined(_IA64_)
        ULONGLONG m_originalStIIP;
        ULONGLONG m_originalStIPSR;
        ULONGLONG m_originalIntSp;
        ULONGLONG m_originalIntGp;

    #elif defined(_AMD64_)
        ULONG64 m_originalRip;
        ULONG64 m_originalRsp;
    #endif

    // HiJacking state related info
    std::mutex m_csSuspensions;
    ManualEvent m_hiJackFinished;
    std::atomic<BOOL> m_bActiveSuspension;
    std::atomic<BOOL> m_bActiveHijack;

    std::atomic<LONG> m_activeHiJack;
    ThreadID m_hijackedThreadID;
#endif //THREAD_HIJACK

    std::mutex m_sampling;
    std::atomic<BOOL> m_bKeepGoing;

    /*  Asynchronous events and associated variables */
    #define ASYNCHRONOUS_EVENTS 3
    /*  0 == Timer for Sample */
    /*  1 == Event for Stop */
    /*  2 == Event for SampleNow */

    #define ASYNCHRONOUS_TIMER 0
    #define ASYNCHRONOUS_STOP 1
    #define ASYNCHRONOUS_SAMPLE_NOW 2

    AutoEvent m_sample;
    AutoEvent m_stop;
#ifdef THREAD_HIJACK
    AutoEvent m_sampleNow;
    AutoEvent m_hiJackSignal;
#endif // THREAD_HIJACK

    /*  Sample time out */
    DWORD m_liWaitTimeOut;

    /*  Success/Failure counters */
    ULONG m_ulSuccess;
    ULONG m_ulFailure;
    ULONG m_ulUnmanaged;
	ULONG m_ulInvalidArg;
    ULONG m_ulUnsafe;
    ULONG m_ulAborted;
    ULONG m_ulSemaphoreAbandon;
    ULONG m_ulWaitForSingleObject;
    ULONG m_ulHijackAbortUnmanaged;
    ULONG m_ulSOKEmptySnapshot;
	ULONG m_ulSOKEmptyShadow;
	ULONG m_ulSOKEmptyEbp;
    ULONG m_ulEFailEmptySnapshot;
    ULONG m_ulSampleTicks;
    ULONG m_ulMinAvail;
  
    BOOL m_bEbpWalking;
    UINT32 m_uDssFlag;
    BOOL m_bEbpStack;
    ULONG m_uMatched;
    ULONG m_uNotMatched;
    ULONG m_uDssLockThread;
 
    ULONG m_uSnapshotFrames;
    ULONG m_uShadowFrames;
    ULONG m_uEbpFrames;

	BOOL m_bEEStarted;

	ULONG m_uTImerCount;
	DWORD m_dwEventMask;
	DWORD m_ulEbpOnNonX86;

    LockHeapThread m_lockHeapThread;
};

/*
 *  Static function used passed to DoStackSnapshot.  This invokes our DSS callback in the DoStackSnapshot
 *  class.
 */
HRESULT __stdcall DoStackSnapshotStackSnapShotCallbackWrapper(
                    FunctionID funcId,
                    UINT_PTR ip,
                    COR_PRF_FRAME_INFO frameInfo,
                    ULONG32 contextSize,
                    BYTE context[],
                    void *clientData);

HRESULT __stdcall DoStackSnapshotEbpCallbackWrapper(
                    FunctionID funcId,
                    UINT_PTR ip,
                    COR_PRF_FRAME_INFO frameInfo,
                    ULONG32 contextSize,
                    BYTE context[],
                    void *clientData);

