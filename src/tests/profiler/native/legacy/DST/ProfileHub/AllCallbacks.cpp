#include "ProfilerCommon.h"

typedef stack<int> NestedRemotingStack_t;
typedef map<ThreadID, NestedRemotingStack_t *> ThreadInvocationMap_t;

NestedRemotingStack_t AllCallbacks_NestedRemotingStack;
ThreadInvocationMap_t AllCallbacks_RemotingStartedInThread;

enum  EnterLeave_t {ENTERLEAVE_ENTER,ENTERLEAVE_LEAVE };

void AllCallbacks_VerifyRemotingIsInactive(IPrfCom * pPrfCom, EnterLeave_t enterLeave);

HRESULT AllCallbacks_AppDomainCreationStarted(IPrfCom * pPrfCom, AppDomainID /*appDomainId*/)
{
    MUST_PASS(PPRFCOM->RemoveEvent(COR_PRF_MONITOR_APPDOMAIN_LOADS));
    return S_OK;
}

HRESULT AllCallbacks_AssemblyLoadStarted(IPrfCom * pPrfCom, AssemblyID /*assemblyID*/)
{
    MUST_PASS(PPRFCOM->RemoveEvent(COR_PRF_MONITOR_ASSEMBLY_LOADS));
    return S_OK;
}


HRESULT AllCallbacks_ModuleLoadStarted(IPrfCom * pPrfCom, ModuleID /*moduleId*/)
{
    MUST_PASS(PPRFCOM->RemoveEvent(COR_PRF_MONITOR_MODULE_LOADS));
    return S_OK;
}


HRESULT AllCallbacks_ClassLoadStarted(IPrfCom * pPrfCom, ClassID /*classId*/)
{
    MUST_PASS(PPRFCOM->RemoveEvent(COR_PRF_MONITOR_CLASS_LOADS));
    return S_OK;
}


HRESULT AllCallbacks_JITCompilationStarted(IPrfCom * pPrfCom, FunctionID /*functionId*/, BOOL /*fIsSafeToBlock*/)
{
    MUST_PASS(PPRFCOM->RemoveEvent(COR_PRF_MONITOR_JIT_COMPILATION | COR_PRF_MONITOR_CACHE_SEARCHES));
    return S_OK;
}


HRESULT AllCallbacks_JITCachedFunctionSearchStarted(IPrfCom * pPrfCom, FunctionID /*functionId*/, BOOL *pbUseCachedFunction)
{
    *pbUseCachedFunction = TRUE;
    MUST_PASS(PPRFCOM->RemoveEvent(COR_PRF_MONITOR_JIT_COMPILATION | COR_PRF_MONITOR_CACHE_SEARCHES));
    return S_OK;
}



HRESULT AllCallbacks_ThreadCreated(IPrfCom * pPrfCom, ThreadID /*managedThreadId*/)
{
    MUST_PASS(PPRFCOM->RemoveEvent(COR_PRF_MONITOR_THREADS));
    return S_OK;
}


HRESULT AllCallbacks_RuntimeSuspendStarted(IPrfCom * pPrfCom, COR_PRF_SUSPEND_REASON /*suspendReason*/)
{
    MUST_PASS(PPRFCOM->RemoveEvent(COR_PRF_MONITOR_SUSPENDS));
    return S_OK;
}


HRESULT AllCallbacks_COMClassicVTableCreated(IPrfCom * pPrfCom, ClassID /*wrappedClassID*/,
                                    REFGUID /*implementedIID*/, void * /*pVTable*/, ULONG /*cSlots*/)
{
    MUST_PASS(PPRFCOM->RemoveEvent(COR_PRF_MONITOR_CCW));
    return S_OK;
}


HRESULT AllCallbacks_RemotingClientSendingMessage(IPrfCom * pPrfCom, GUID * pCookie, BOOL fIsAsync)
{
    // we are not monitoring COOKIES or ASYNC
    if ( pCookie == NULL )
    {
        if ( fIsAsync == TRUE )
            FAILURE(L"RemotingClientSendingMessage() FAILED - ASYNC Calls NOT MONITORED");
    }
    else
        FAILURE(L"ProfilerCallback::RemotingClientSendingMessage() FAILED - NULL Cookie Expected");

    return S_OK;
}


HRESULT AllCallbacks_RemotingClientRedeivingReply(IPrfCom * pPrfCom, GUID *pCookie, BOOL fIsAsync)
{
        // we are not monitoring COOKIES or ASYNC
    if ( pCookie == NULL )
    {
        if ( fIsAsync == TRUE )
            FAILURE(L"ProfilerCallback::RemotingServerReceivingMessage() FAILED - ASYNC Calls NOT MONITORED");
    }
    else
        FAILURE(L"ProfilerCallback::RemotingServerReceivingMessage() FAILED - NULL Cookie Expected");

    return S_OK;
}

HRESULT AllCallbacks_RemotingServerReceivingMessage(IPrfCom * pPrfCom, GUID *pCookie, BOOL fIsAsync)
{
    // we are not monitoring COOKIES or ASYNC
    if ( pCookie == NULL )
    {
        if ( fIsAsync == TRUE )
            FAILURE(L"ProfilerCallback::RemotingServerReceivingMessage() FAILED - ASYNC Calls NOT MONITORED");
    }
    else
        FAILURE(L"ProfilerCallback::RemotingServerReceivingMessage() FAILED - NULL Cookie Expected");

    return S_OK;
}

HRESULT AllCallbacks_RemotingServerInvocationStarted(IPrfCom * pPrfCom)
{
    ThreadID threadID;

    MUST_PASS(PINFO->GetCurrentThreadID(&threadID));

    {
    	lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

        ThreadInvocationMap_t::iterator vt = AllCallbacks_RemotingStartedInThread.find(threadID);
        if ( vt == AllCallbacks_RemotingStartedInThread.end() )
            AllCallbacks_RemotingStartedInThread.insert(ThreadInvocationMap_t::value_type(threadID,new NestedRemotingStack_t()));
        vt = AllCallbacks_RemotingStartedInThread.find(threadID);
        vt->second->push(0);
    }

    return S_OK;
}


HRESULT AllCallbacks_RemotingServerInvocationReturned(IPrfCom * pPrfCom)
{
    ThreadID threadID;

    MUST_PASS(PINFO->GetCurrentThreadID(&threadID));

    {
    	lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

        ThreadInvocationMap_t::iterator vt = AllCallbacks_RemotingStartedInThread.find(threadID);
        if (vt==AllCallbacks_RemotingStartedInThread.end())
        {
            FAILURE(L"Called RemotingServerInvocationReturned without RemotingServerInvocationStarted");
            return E_FAIL;
        }

        if ( vt->second->top() !=0 )
            FAILURE(L"RemotingServerInvocationReturned - unbalanced FunctionEnter/Leave in RemoteInvocation");

        vt->second->pop();
        if (vt->second->size()==0) {
            // No RemotingServerInvocation on this thread.
            delete vt->second;
            AllCallbacks_RemotingStartedInThread.erase(vt);
        }
    }
     return S_OK;
}


HRESULT AllCallbacks_RemotingServerSendingReply( IPrfCom * pPrfCom, GUID *pCookie, BOOL fIsAsync )
{
    // we are not monitoring COOKIES or ASYNC
    if ( pCookie == NULL )
    {
        if ( fIsAsync == TRUE )
            FAILURE(L"ProfilerCallback::RemotingServerSendingReply() FAILED - ASYNC Calls NOT MONITORED");
    }
    else
        FAILURE(L"ProfilerCallback::RemotingServerSendingReply() FAILED - NULL Cookie Expected");


       return S_OK;
}



void AllCallbacks_FunctionEnter3( IPrfCom * pPrfCom, FunctionIDOrClientID /*functionIDOrClientID*/, COR_PRF_ELT_INFO /*eltInfo*/)
{
    AllCallbacks_VerifyRemotingIsInactive(pPrfCom, ENTERLEAVE_ENTER);
}


void AllCallbacks_FunctionLeave3( IPrfCom * pPrfCom, FunctionIDOrClientID /*functionIDOrClientID*/, COR_PRF_ELT_INFO /*eltInfo*/)
{
    AllCallbacks_VerifyRemotingIsInactive(pPrfCom, ENTERLEAVE_LEAVE);
}

void AllCallbacks_FunctionTailCall3( IPrfCom * pPrfCom, FunctionIDOrClientID /*functionIDOrClientID*/, COR_PRF_ELT_INFO /*eltInfo*/)
{
    AllCallbacks_VerifyRemotingIsInactive(pPrfCom, ENTERLEAVE_LEAVE);
}


void AllCallbacks_ExceptionUnwindFunctionLeave( IPrfCom * pPrfCom)
{
    AllCallbacks_VerifyRemotingIsInactive(pPrfCom, ENTERLEAVE_LEAVE);
}


void AllCallbacks_VerifyRemotingIsInactive(IPrfCom *  pPrfCom, EnterLeave_t enterLeave)
{
    ThreadID threadID;


    MUST_PASS(PINFO->GetCurrentThreadID(&threadID));

    {
        lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

        ThreadInvocationMap_t::iterator vt = AllCallbacks_RemotingStartedInThread.find(threadID);
        if (vt != AllCallbacks_RemotingStartedInThread.end())
        {
            // We are in RemotingServerInvocationStarted
            switch (enterLeave)
            {
            case ENTERLEAVE_LEAVE:
                --(vt->second->top());
                if (vt->second->top() < 0)
                    FAILURE(L"No RemotingServerInvocationReturned  received");
                break;
            case ENTERLEAVE_ENTER:
                ++(vt->second->top());
                break;
            default:
                FAILURE(L"Unrecognized parameter to VerifyRemotingIsInactive");
            }
        }
    }
}

HRESULT AllCallbacks_Verify(IPrfCom * pPrfCom)
{
    HRESULT hr = S_OK;

    // The gist of the verification here is to ensure that we only
    // received a particular event of interest either once or no
    // time at all. In essence, after we receive an event the first
    // time we simply turn it off...

    // startup/shutdown events
    if ( PPRFCOM->m_Startup != 1 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected Startup Count = " << PPRFCOM->m_Startup);
    }


    if ( PPRFCOM->m_Shutdown != 1 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected Shutdown Count = " << PPRFCOM->m_Shutdown);
    }

    if (FALSE == pPrfCom->m_bAttachMode)
    {
/*
        if ( PPRFCOM->m_AppDomainCreationStarted != 1 )
        {
            hr = E_FAIL;
            FAILURE( L"Unexpected App Domain Creation Started Count = " << PPRFCOM->m_AppDomainCreationStarted);
        }

        if ( PPRFCOM->m_AppDomainCreationFinished != 0 )
        {
            hr = E_FAIL;
            FAILURE( L"Unexpected App Domain Creation Finished Count = " << PPRFCOM->m_AppDomainCreationFinished);
        }
*/
        if ( PPRFCOM->m_AssemblyLoadStarted != 1 )
        {
            hr = E_FAIL;
            FAILURE( L"Unexpected Assembly Load Started Count = " << PPRFCOM->m_AssemblyLoadStarted);
        }

        if ( PPRFCOM->m_AssemblyLoadFinished != 0 )
        {
            hr = E_FAIL;
            FAILURE( L"Unexpected Assembly Load Finished Count = " << PPRFCOM->m_AssemblyLoadFinished);
        }

        if ( PPRFCOM->m_ModuleLoadStarted != 1 )
        {
            hr = E_FAIL;
            FAILURE( L"Unexpected Module Load Started Count = " << PPRFCOM->m_ModuleLoadStarted);
        }

        if ( PPRFCOM->m_ModuleLoadFinished != 0 )
        {
            hr = E_FAIL;
            FAILURE( L"Unexpected Module Load Finished Count = " << PPRFCOM->m_ModuleLoadFinished);
        }

        if ( PPRFCOM->m_ModuleAttachedToAssembly != 0 )
        {
            hr = E_FAIL;
            FAILURE( L"Unexpected Module Attached To Assembly Count = " << PPRFCOM->m_ModuleAttachedToAssembly);
        }

        if ( PPRFCOM->m_ThreadCreated!= 1 )
        {
            hr = E_FAIL;
            FAILURE( L"Unexpected Thread Created Count = " << PPRFCOM->m_ThreadCreated);
        }

        if (( PPRFCOM->m_JITCompilationStarted != 1 ) && ( PPRFCOM->m_JITCachedFunctionSearchStarted != 1 ))
        {
            hr = E_FAIL;
            FAILURE( L"Unexpected JIT Method Count != PreJIT Count = " << PPRFCOM->m_JITCompilationStarted << L" " << PPRFCOM->m_JITCachedFunctionSearchStarted);
        }

        if ( PPRFCOM->m_JITCompilationFinished != 0 )
        {
            hr = E_FAIL;
            FAILURE( L"Unexpected JIT Compilation Finished Method Count = " << PPRFCOM->m_JITCompilationFinished);
        }
    }
    
/*
    //
    //AppDomain events
    //

    if ( PPRFCOM->m_AppDomainShutdownStarted != 0 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected App Domain Shutdown Started Count = " << PPRFCOM->m_AppDomainShutdownStarted);
    }


    if ( PPRFCOM->m_AppDomainShutdownFinished != 0 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected App Domain Shutdown Finished Count = " << PPRFCOM->m_AppDomainShutdownFinished);
    }
*/
    //
    // assembly events
    //

    if ( PPRFCOM->m_AssemblyUnloadStarted != 0 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected Assembly Unload Started Count = " << PPRFCOM->m_AssemblyUnloadStarted);
    }

    if ( PPRFCOM->m_AssemblyUnloadFinished != 0 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected Assembly Unload Finished Count = " << PPRFCOM->m_AssemblyUnloadFinished);
    }

    //
    // module events
    //

    if ( PPRFCOM->m_ModuleUnloadStarted != 0 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected Module Unload Started Count = " << PPRFCOM->m_ModuleUnloadStarted);
    }

    if ( PPRFCOM->m_ModuleUnloadFinished != 0 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected Module Unload Finished Count = " << PPRFCOM->m_ModuleUnloadFinished);
    }


    //
    // class events
    //
    if ( PPRFCOM->m_ClassLoadStarted != 1 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected Class Load Started Count = " << PPRFCOM->m_ClassLoadStarted);
    }

    if ( PPRFCOM->m_ClassLoadFinished != 0 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected Class Load Finished Count = " << PPRFCOM->m_ClassLoadFinished);
    }

    if ( PPRFCOM->m_ClassUnloadStarted != 0 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected Class Unload Started Count = " << PPRFCOM->m_ClassUnloadStarted);
    }

    if ( PPRFCOM->m_ClassUnloadFinished != 0 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected Class Unload Finished Count = " << PPRFCOM->m_ClassUnloadFinished);
    }

    //
    // JIT events
    //

    if ( PPRFCOM->m_JITCachedFunctionSearchFinished != 0 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected JIT Search Finished Method Count = " << PPRFCOM->m_JITCachedFunctionSearchFinished);
    }

    if ( PPRFCOM->m_JITFunctionPitched!= 0 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected JIT Code Pitch Method Count = " << PPRFCOM->m_JITFunctionPitched);
    }

    //
    // thread events
    //

    if ( PPRFCOM->m_ThreadDestroyed!= 0 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected Thread Destroyed Count = " << PPRFCOM->m_ThreadDestroyed);
    }

    //
    // transition events
    //

    //
    // require if M2U transitions are > 0 then U2M MUST be also > 0
    //
    if ( PPRFCOM->m_ManagedToUnmanagedTransition > 0 )
    {
        if ( PPRFCOM->m_UnmanagedToManagedTransition == 0 )
        {
            hr = E_FAIL;
            FAILURE( L"Unbalanced Managed to Unmanaged Transitions Count:" << PPRFCOM->m_ManagedToUnmanagedTransition << L" " << PPRFCOM->m_UnmanagedToManagedTransition);
        }
    }

    //
    // require if U2M transitions are > 0 then M2U MUST be also > 0
    //
    if ( PPRFCOM->m_UnmanagedToManagedTransition > 0 )
    {
        if ( PPRFCOM->m_ManagedToUnmanagedTransition == 0 )
        {
            hr = E_FAIL;
            FAILURE( L"Unbalanced Managed to Unmanaged Transitions Count:" << PPRFCOM->m_ManagedToUnmanagedTransition << L" " << PPRFCOM->m_UnmanagedToManagedTransition);
        }
    }


    //
    // CCW events
    //
    if ( PPRFCOM->m_COMClassicVTableCreated > 2 ) // anticipate for a thread race
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected CCW Created Count = " << PPRFCOM->m_COMClassicVTableCreated);
    }

    if ( PPRFCOM->m_COMClassicVTableDestroyed != 0 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected CCW Destroyed Count = " << PPRFCOM->m_COMClassicVTableDestroyed);
    }

    //
    // Function Enter-Leave hooks event
    // We are expecting the counters to have the value => 1
    //
    if ( PPRFCOM->m_FunctionEnter3WithInfo == 0)
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected Function Enter Count\nm_FunctionEnter3WithInfo = " << PPRFCOM->m_FunctionEnter3WithInfo);
    }

    if ( PPRFCOM->m_FunctionLeave3WithInfo == 0   &&  PPRFCOM->m_FunctionTailCall3WithInfo == 0)
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected Function Leave/Tail Count\nm_FunctionLeave3WithInfo = " << PPRFCOM->m_FunctionLeave3WithInfo << "\nm_FunctionTailCall3WithInfo = " << PPRFCOM->m_FunctionTailCall3WithInfo);
    }

    //
    // GC
    //

    // we may have a GC or not so do not check the started events
    if ( PPRFCOM->m_ObjectsAllocatedByClass != 0 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected CG Objects Allocated By Class Count = " << PPRFCOM->m_ObjectsAllocatedByClass);
    }

    if ( PPRFCOM->m_RootReferences != 0 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected CG Root References Count = " << PPRFCOM->m_RootReferences);
    }

    if ( PPRFCOM->m_ObjectReferences != 0 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected CG Object References  Count = " << PPRFCOM->m_ObjectReferences);
    }

    if ( PPRFCOM->m_MovedReferences != 0 )
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected CG Moved References  Count = " << PPRFCOM->m_MovedReferences);
    }

    //
    // REMOTING
    //

    //
    // server counter verification
    //
    if ( PPRFCOM->m_RemotingServerInvocationStarted != PPRFCOM->m_RemotingServerInvocationReturned)
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected Server Invocation Started Count / Returned Count : " << PPRFCOM->m_RemotingServerInvocationStarted << L" " << PPRFCOM->m_RemotingServerInvocationReturned);
    }

    if ( PPRFCOM->m_RemotingServerReceivingMessage != PPRFCOM->m_RemotingServerSendingReply)
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected Server Receiving Message Count / Server Sending Reply Count : " << PPRFCOM->m_RemotingServerReceivingMessage << L" " << PPRFCOM->m_RemotingServerSendingReply);
    }

    //
    // client counter verification
    //
    if ( PPRFCOM->m_RemotingClientInvocationStarted!= PPRFCOM->m_RemotingClientInvocationFinished)
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected Client Invocation Started Count / Client Invocation Returned Count : " << PPRFCOM->m_RemotingClientInvocationStarted << L" " << PPRFCOM->m_RemotingClientInvocationFinished);
    }

    if ( PPRFCOM->m_RemotingClientSendingMessage != PPRFCOM->m_RemotingClientReceivingReply)
    {
        hr = E_FAIL;
        FAILURE( L"Unexpected Client Receiving Message Count / Client Sending Reply Count : " << PPRFCOM->m_RemotingClientSendingMessage << L" " << PPRFCOM->m_RemotingClientReceivingReply);
    }

    if( AllCallbacks_RemotingStartedInThread.size() != 0 )
    {
        hr = E_FAIL;
        FAILURE( L"RemotingStartedInThread map not empty!\n");
    }

    return hr;
}

void AllCallbacks_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    pModuleMethodTable->FLAGS = ((COR_PRF_MONITOR_ALL | COR_PRF_DISABLE_INLINING ) &
                               ~(COR_PRF_MONITOR_REMOTING_COOKIE | COR_PRF_MONITOR_REMOTING_ASYNC | COR_PRF_MONITOR_GC | COR_PRF_ENABLE_REJIT)) |
                                 COR_PRF_MONITOR_REMOTING | 
                                 COR_PRF_ENABLE_FRAME_INFO;

    REGISTER_CALLBACK(APPDOMAINCREATIONSTARTED,         AllCallbacks_AppDomainCreationStarted);
    REGISTER_CALLBACK(ASSEMBLYLOADSTARTED,              AllCallbacks_AssemblyLoadStarted);
    REGISTER_CALLBACK(MODULELOADSTARTED,                AllCallbacks_ModuleLoadStarted);
    REGISTER_CALLBACK(CLASSLOADSTARTED,                 AllCallbacks_ClassLoadStarted);
    REGISTER_CALLBACK(JITCOMPILATIONSTARTED,            AllCallbacks_JITCompilationStarted);
    REGISTER_CALLBACK(JITCACHEDFUNCTIONSEARCHSTARTED,   AllCallbacks_JITCachedFunctionSearchStarted);
    REGISTER_CALLBACK(THREADCREATED,                    AllCallbacks_ThreadCreated);
    REGISTER_CALLBACK(RUNTIMESUSPENDSTARTED,            AllCallbacks_RuntimeSuspendStarted);
    REGISTER_CALLBACK(COMCLASSICVTABLECREATED,          AllCallbacks_COMClassicVTableCreated);
    REGISTER_CALLBACK(REMOTINGCLIENTSENDINGMESSAGE,     AllCallbacks_RemotingClientSendingMessage);
    REGISTER_CALLBACK(REMOTINGCLIENTRECEIVINGREPLY,     AllCallbacks_RemotingClientRedeivingReply);
    REGISTER_CALLBACK(REMOTINGSERVERRECEIVINGMESSAGE,   AllCallbacks_RemotingServerReceivingMessage);
    REGISTER_CALLBACK(REMOTINGSERVERINVOCATIONSTARTED,  AllCallbacks_RemotingServerInvocationStarted);
    REGISTER_CALLBACK(REMOTINGSERVERINVOCATIONRETURNED, AllCallbacks_RemotingServerInvocationReturned);
    REGISTER_CALLBACK(REMOTINGSERVERSENDINGREPLY,       AllCallbacks_RemotingServerSendingReply);
    REGISTER_CALLBACK(FUNCTIONENTER3WITHINFO,           AllCallbacks_FunctionEnter3);
    REGISTER_CALLBACK(FUNCTIONLEAVE3WITHINFO,           AllCallbacks_FunctionLeave3);
    REGISTER_CALLBACK(FUNCTIONTAILCALL3WITHINFO,        AllCallbacks_FunctionTailCall3);
    REGISTER_CALLBACK(EXCEPTIONUNWINDFUNCTIONLEAVE,     AllCallbacks_ExceptionUnwindFunctionLeave);
    REGISTER_CALLBACK(VERIFY,                           AllCallbacks_Verify);

    return;
}
