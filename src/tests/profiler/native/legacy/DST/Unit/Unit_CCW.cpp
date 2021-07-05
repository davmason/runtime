#include "ProfilerCommon.h"
#include <thread>


//*********************************************************************
//  Descripton: Global helper function to compare two guids   
//  
//  Params:     [IN] REFGUID inGUID1 - Guid # 1
//              [IN] REFGUID inGUID2 - Guid # 2 
//
//  Return:     TRUE if guids are equal, else FALSE
//  
//  Notes:      n/a
//*********************************************************************

BOOL IsSameGUID(REFGUID inGUID1, REFGUID inGUID2)
{
	return (BOOL)( (inGUID1.Data1 == inGUID2.Data1)
                && (inGUID1.Data2 == inGUID2.Data2)
			    && (inGUID1.Data3 == inGUID2.Data3)
			    && (inGUID1.Data4[0] == inGUID2.Data4[0])
			    && (inGUID1.Data4[1] == inGUID2.Data4[1])
			    && (inGUID1.Data4[2] == inGUID2.Data4[2])
			    && (inGUID1.Data4[3] == inGUID2.Data4[3])
			    && (inGUID1.Data4[4] == inGUID2.Data4[4])
			    && (inGUID1.Data4[5] == inGUID2.Data4[5])
			    && (inGUID1.Data4[6] == inGUID2.Data4[6])
			    && (inGUID1.Data4[7] == inGUID2.Data4[7]));
}


//////////////////////////////////////////
// Struct declaration : CCWInfo         //
// A simple wrapper for the COM Vtable  //
//////////////////////////////////////////

struct CCWInfo
{
    CCWInfo(
        const CCWInfo &
        );

    CCWInfo(
        ClassID classID, 
        REFGUID inGUID, 
        void * pVtable, 
        ULONG slots
        );

    CCWInfo & operator = (
        const CCWInfo &
        );

    BOOL Compare(
        void * key
        );

    ClassID m_classID;
    GUID m_GUID;
    void * m_id;
    ULONG m_slots;
};


//////////////////////////////////////////////
// Struct declaration : CCWInfoTable        //
// Container to store the COM Vtable data   //
//////////////////////////////////////////////

struct CCWInfoTable
{
    HRESULT AddVTable(
        ClassID wrappedClassId,
        REFGUID implementedIID,
        void *pVTable,
        ULONG cSlots
        );

    HRESULT RemoveVTable(
        void *pVtable
        );

    CCWInfo * LookUp(
        void *pVTable
        );

    // underlying data structures
    std::map<void*, CCWInfo *> m_CCWTable;
    std::map<void*, CCWInfo *>::iterator m_itrCCWTable;
}
g_ccwTable; // global variable


//////////////////////////////////////////////////////////////////
// Struct declaration : GcOperation                             //  
// Responsible for spawning a worker thread (suspension thread) //
// since ICorProfilerInfo::ForceGC cannot be called from the    //
// main thread (since it is receiving callbacks).               //
//////////////////////////////////////////////////////////////////

struct GcOperation
{
    GcOperation(
        );

    static unsigned __stdcall ThreadProc(
        );

    HRESULT ForceGCWrapper(
        ICorProfilerInfo *
        );

    BOOL m_bInitOK;

    // tracking the reason for runtime suspension
    DWORD m_cRuntimeSuspensionCtr[COR_PRF_SUSPEND_FOR_GC_PREP + 1];

    // how many times has ICorProfilerInfo::ForceGC succeeded?
    DWORD m_cForceGCSucceeded;

    // worker thread's Id
    DWORD m_dwThreadId;

    // handle to worker thread
    HANDLE m_hndSuspensionThread;

    // worker thread perfoms an action when this event is signaled
    AutoEvent m_wakeThreadEvent; 
    
    // should the worker thread exit?
    BOOL m_bThreadExit;

    // cached pointer to ICorProfilerInfo
    ICorProfilerInfo * m_pCorPrfInfo;

    void startGCThread();
}
g_GcOperation; // global variable


//*********************************************************************
//  Descripton: Copy constructor for CCWInfo structure.    
//  
//  Params:     const CCWInfo & ccw - reference guid from which the
//                  new CCWInfo instance is to be constructed
//
//  Return:     n/a
//  
//  Notes:      n/a
//*********************************************************************

CCWInfo::CCWInfo(const CCWInfo & ccw)
:   m_classID   (ccw.m_classID),
    m_id        (ccw.m_id),
    m_slots     (ccw.m_slots)
{
        // assigning the GUID
        m_GUID.Data1 = ccw.m_GUID.Data1;
        m_GUID.Data2 = ccw.m_GUID.Data2;
        m_GUID.Data3 = ccw.m_GUID.Data3;

        for ( int i = 0; i < 8; i++ ) {
            m_GUID.Data4[i] = ccw.m_GUID.Data4[i];
        }
}

//*********************************************************************
//  Descripton: Parameterized constructor for CCWInfo structure.    
//  
//  Params:     ClassID classID -
//                  The class ID for which the vtable has been created
//              REFGUID inGUID  -
//                  The Interface ID implemented by the class    
//              void * pVtable  -
//                  A pointer to the start of the vtable.
//              ULONG slots     -
//                  The number of slots that are in the vtable  
//
//  Return:     n/a
//  
//  Notes:      n/a
//*********************************************************************

CCWInfo::CCWInfo(ClassID classID, REFGUID inGUID, void * pVtable, ULONG slots)
:   m_classID   (classID),
    m_id        (pVtable),
    m_slots     (slots)
{
        // assigning the GUID
        m_GUID.Data1 = inGUID.Data1;
        m_GUID.Data2 = inGUID.Data2;
        m_GUID.Data3 = inGUID.Data3;

        for ( int i = 0; i < 8; i++ ) {
            m_GUID.Data4[i] = inGUID.Data4[i];
        }
}

//*********************************************************************
//  Descripton: Assignment operator for CCWInfo structure.    
//  
//  Params:     const CCWInfo & ccw - reference guid from which the
//                  this CCWInfo instance is to populated
//
//  Return:     n/a
//  
//  Notes:      n/a
//*********************************************************************

CCWInfo & CCWInfo::operator=(const CCWInfo & ccw)
{
    if (this == &ccw) {
        return (*this);  // never assign to self
    }

    this->m_classID = ccw.m_classID;
    this->m_id = ccw.m_id;
    this->m_slots = ccw.m_slots;

    // assigning the GUID
    this->m_GUID.Data1 = ccw.m_GUID.Data1;
    this->m_GUID.Data2 = ccw.m_GUID.Data2;
    this->m_GUID.Data3 = ccw.m_GUID.Data3;

    for ( int i = 0; i < 8; i++ ) {
        this->m_GUID.Data4[i] = ccw.m_GUID.Data4[i];
    }

    return (*this) ;
}

//*********************************************************************
//  Descripton: Compares specified vtable address to the one stored
//              internally.
//  
//  Params:     void * key - The vtable address to be compared
//
//  Return:     TRUE if matching, else FALSE
//  
//  Notes:      n/a
//*********************************************************************

BOOL CCWInfo::Compare (void * key)
{
    return (BOOL)(m_id == key);
}

//*********************************************************************
//  Descripton: Composes a CCWInfo instance from supplied data and 
//              stores it internally.
//  
//  Params:     ClassID classID -
//                  The class ID for which the vtable has been created
//              REFGUID inGUID  -
//                  The Interface ID implemented by the class    
//              void * pVtable  -
//                  A pointer to the start of the vtable.
//              ULONG slots     -
//                  The number of slots that are in the vtable  
//
//  Return:     S_OK    - if method successful
//              E_FAIL  - if vtable entry already exists internally
//              E_OUTOFMEMORY   - if unable to allocate storage for
//                  the vtable structure
//  
//  Notes:      n/a
//*********************************************************************

HRESULT CCWInfoTable::AddVTable(ClassID wrappedClassId, REFGUID implementedIID,
    void *pVTable, ULONG cSlots)
{
    HRESULT hr = S_OK;
	CCWInfo * pCcwInfo = NULL;

    m_itrCCWTable = m_CCWTable.find(pVTable);
    if (m_itrCCWTable != m_CCWTable.end()) {
        hr = E_FAIL;
        goto ErrReturn;
    }

    pCcwInfo = new CCWInfo(wrappedClassId, implementedIID, pVTable, cSlots);
    if (NULL == pCcwInfo) {
        hr = E_OUTOFMEMORY;
        goto ErrReturn;
    }
     
    m_CCWTable[pVTable] = pCcwInfo;

ErrReturn:
    return hr;
}

//*********************************************************************
//  Descripton: Removes a CCWInfo structure (vtable wrapper) if 
//              it happens to be stored internally.
//  
//  Params:     void * pVTable - Pointer to the Vtable
//
//  Return:     S_OK    - if method successful
//              E_FAIL  - if entry for specificed VTable doesn't exist
//  
//  Notes:      n/a
//********************************************************************

HRESULT CCWInfoTable::RemoveVTable(void *pVTable)
{
    HRESULT hr = S_OK;

    m_itrCCWTable = m_CCWTable.find(pVTable);
    if (m_itrCCWTable == m_CCWTable.end()) {
        hr = E_FAIL;
        goto ErrReturn;
    }

    m_CCWTable.erase(m_itrCCWTable);

ErrReturn:
    return hr;
}

//*********************************************************************
//  Descripton: Checks to see if specified CCWInfo structure (vtable 
//              wrapper) is stored internally.
//  
//  Params:     void * pVTable - Pointer to the Vtable
//
//  Return:     If successful - returns pointer to vtable wrapper
//              Else - returns NULL
//
//  Notes:      n/a
//********************************************************************

CCWInfo * CCWInfoTable::LookUp(void *pVTable)
{
    CCWInfo * pCCw = NULL;

    m_itrCCWTable = m_CCWTable.find(pVTable);
    if (m_itrCCWTable == m_CCWTable.end()) {
        goto ErrReturn;
    }

    pCCw = (m_itrCCWTable->second);

ErrReturn:
    return pCCw; 
}

//*********************************************************************
//  Descripton: Constructor for GcOperation structure.    
//  
//  Params:     n/a
//
//  Return:     n/a
//  
//  Notes:      In case the ctor is unable to spawn the worker thread
//              or is unable to create a auto-reset event, it sets
//              m_bInitOK to FALSE. m_bInitOK is then checked by the 
//              method GcOperation::ForceGCWrapper as a pre-condition 
//              check.       
//*********************************************************************

GcOperation::GcOperation()
:   m_bInitOK               (TRUE),
    m_cForceGCSucceeded     (0),
    m_hndSuspensionThread   (NULL),
    m_bThreadExit           (FALSE),
    m_pCorPrfInfo           (NULL)
{
    // initialize the spuspension array counters
    for ( DWORD i = 0; i <= (DWORD)COR_PRF_SUSPEND_FOR_GC_PREP; i++ ) {
        m_cRuntimeSuspensionCtr[i] = 0;
    }

    // Create an auto-reset event that will notify the worker thread
    // to suspend the runtime via forcing the GC to collect.

    // initialize the worker thread which forces GC runtime suspensions
}

void GcOperation::startGCThread() 
{
	try
	{
		std::thread t1(GcOperation::ThreadProc);
		t1.detach();
	} 
	catch (system_error)
	{
		m_bInitOK = FALSE;
        goto ErrReturn;
	}
ErrReturn:
    return;
}


//*********************************************************************
//  Descripton: Signals an event that notifies the worker thread 
//              (suspension thread) to call ICorProfilerInfo::ForceGC().
//  
//  Params:     ICorProfilerInfo * pCorPrfInfo - 
//                  Pointer to a valid ICorProfilerInfo instance.
//
//  Return:     S_OK if successful, else COM error code
//
//  Notes:      n/a
//********************************************************************

HRESULT GcOperation::ForceGCWrapper(ICorProfilerInfo * pCorPrfInfo)
{
    HRESULT hr = S_OK;
    DWORD dwRet = 0;
    
    if (FALSE == m_bInitOK) {
        hr = E_FAIL;
        goto ErrReturn;                                
    }

    if (NULL == pCorPrfInfo) {
        hr = E_INVALIDARG;
        goto ErrReturn;
    }

    m_pCorPrfInfo = pCorPrfInfo;
    // NOTE: I'm avoiding AddRef() calls on purpose 
    // m_pCorPrfInfo will now be read by the worker thread
    
    g_GcOperation.m_wakeThreadEvent.Signal();
    
ErrReturn:
    return hr;
}


//*********************************************************************
//  Descripton: Method in which the suspension thread (worker)
//              happily lives. Here we call ICorProfilerInfo::ForceGC.
//  
//  Params:     n/a.
//
//  Return:     If you really care about the exit code, the method
//              returns S_OK if successful, else a COM error code
//
//  Notes:      n/a
//********************************************************************

unsigned __stdcall GcOperation::ThreadProc()
{
    HRESULT hr = S_OK;
    DWORD dwRet = 0;

    // Looping and waiting for suspension event to be signalled
    while (S_OK == hr && !g_GcOperation.m_bThreadExit) {
        g_GcOperation.m_wakeThreadEvent.Wait();
    
        // is the thread supposed to exit now?
        if (TRUE == g_GcOperation.m_bThreadExit) {
            continue;
        }    

        // some necessary pre-condition check
        if (NULL == g_GcOperation.m_pCorPrfInfo) {
            hr = E_FAIL;
            continue;
        }

        // Now that we have been signaled to force a GC event ....
        hr = g_GcOperation.m_pCorPrfInfo->ForceGC(); 
        if (S_OK != hr) {
            continue;
        }
            
        ++g_GcOperation.m_cForceGCSucceeded;
    }

    return (unsigned) hr ;
}


//*********************************************************************
//  Descripton: Callback from TestProfiler when all the runtime
//              threads are about to be suspended.
//  
//  Params:     IPrfCom * pPrfCom   - 
//                  pointer to test module containing helper functions
//              COR_PRF_SUSPEND_REASON suspendReason -
//                  Indicates why the runtime was suspended
//
//  Return:     n/a
//  
//  Notes:      n/a
//*********************************************************************

HRESULT Unit_CCW_RuntimeSuspendStarted(IPrfCom * pPrfCom,
    COR_PRF_SUSPEND_REASON suspendReason)
{
    HRESULT hr = S_OK;

    switch(suspendReason)
    {
        case COR_PRF_SUSPEND_OTHER:
            g_GcOperation.m_cRuntimeSuspensionCtr[(DWORD)COR_PRF_SUSPEND_OTHER]++;
            break;

        case COR_PRF_SUSPEND_FOR_GC:
            g_GcOperation.m_cRuntimeSuspensionCtr[(DWORD)COR_PRF_SUSPEND_FOR_GC]++;
            break;

        case COR_PRF_SUSPEND_FOR_GC_PREP:
            g_GcOperation.m_cRuntimeSuspensionCtr[(DWORD)COR_PRF_SUSPEND_FOR_GC_PREP]++;
            break;

        case COR_PRF_SUSPEND_FOR_SHUTDOWN:
            g_GcOperation.m_cRuntimeSuspensionCtr[(DWORD)COR_PRF_SUSPEND_FOR_SHUTDOWN]++;
            break;

        case COR_PRF_SUSPEND_FOR_CODE_PITCHING:
            g_GcOperation.m_cRuntimeSuspensionCtr[(DWORD)COR_PRF_SUSPEND_FOR_CODE_PITCHING]++;
            break;

        case COR_PRF_SUSPEND_FOR_INPROC_DEBUGGER:
            g_GcOperation.m_cRuntimeSuspensionCtr[(DWORD)COR_PRF_SUSPEND_FOR_INPROC_DEBUGGER]++;
            break;

        case COR_PRF_SUSPEND_FOR_APPDOMAIN_SHUTDOWN:
            g_GcOperation.m_cRuntimeSuspensionCtr[(DWORD)COR_PRF_SUSPEND_FOR_APPDOMAIN_SHUTDOWN]++;
            break;
		default:
			break;
    }

    return S_OK; // CLR ignores the return value
}

//*********************************************************************
//  Descripton: Callback from TestProfiler when a COM VTable is
//              created by the profiled app. We do a bunch of 
//              verification stuff here.
//  
//  Params:     IPrfCom * pPrfCom   - 
//                  pointer to test module containing helper functions
//              ClassID classID -
//                  The class ID for which the vtable has been created
//              REFGUID inGUID  -
//                  The Interface ID implemented by the class    
//              void * pVtable  -
//                  A pointer to the start of the vtable.
//              ULONG slots     -
//                  The number of slots that are in the vtable  
//
//  Return:     n/a
//  
//  Notes:      n/a
//*********************************************************************

HRESULT Unit_CCW_COMClassicVTableCreated(IPrfCom * pPrfCom, ClassID wrappedClassID,
    REFGUID implementedIID, VOID * pVTable, ULONG cSlots)
{
    HRESULT hr = S_OK;
    WCHAR_STR( name );

    // Pre-condition checks

    if (0 == wrappedClassID) {
        FAILURE(L"ProfilerCallback::COMClassicVTableCreated() returned NULL classID");
        goto ErrReturn;
    }

    if (NULL == pVTable) {
        FAILURE(L"ProfilerCallback::COMClassicVTableCreated() returned NULL *pVTable");
        goto ErrReturn;
    }

    if (cSlots < 3) { // Implements atleast QI, AddRef and Release.
        FAILURE(L"ProfilerCallback::COMClassicVTableCreated() returned unexpected number of slots");
        goto ErrReturn;
    }

    //
    // printf stuff for verification
    //

    hr = pPrfCom->GetClassIDName(wrappedClassID, name);
    if (S_OK != hr) {
        FAILURE(L"PrfHelper::GetNameFromClassID FAILED");
        goto ErrReturn;
    }

    DISPLAY(L"Class Name : " << name.c_str())
    DISPLAY(L"Pointer    : 0x" << HEX(pVTable))
    DISPLAY(L"Number of slots : " << cSlots )


    // store the vtable info
    hr = g_ccwTable.AddVTable(wrappedClassID, implementedIID, pVTable, cSlots);
    if (S_OK != hr) {
        FAILURE(L"Failed to store the VTable entry.");
        goto ErrReturn;
    }

    // force a GC sweep
    hr = g_GcOperation.ForceGCWrapper(pPrfCom->m_pInfo); 
    if (S_OK != hr) {
        FAILURE(L"Setting the ForceGC Request FAILED");
        goto ErrReturn;
    }

ErrReturn:
    return S_OK; // CLR ignores the return value anyways
}

//*********************************************************************
//  Descripton: Callback from TestProfiler when a COM VTable is
//              torn down inside the profiled app. We do a bunch of 
//              verification stuff here.
//  
//  Params:     IPrfCom * pPrfCom   - 
//                  pointer to test module containing helper functions
//              ClassID classID -
//                  The class ID for which the vtable has been created
//              REFGUID inGUID  -
//                  The Interface ID implemented by the class    
//              void * pVtable  -
//                  A pointer to the start of the vtable.
//
//  Return:     n/a
//  
//  Notes:      n/a
//*********************************************************************

HRESULT Unit_CCW_COMClassicVTableDestroyed(IPrfCom * pPrfCom, ClassID wrappedClassID, 
    REFGUID implementedIID, VOID *pVTable)
{
    CCWInfo * pCCWInfo;

    //
    // for unit testing verify that there is an entry in the tables
    // that matches the given data
    //

    pCCWInfo = g_ccwTable.LookUp(pVTable);
    if (NULL == pCCWInfo) {
        FAILURE(L"Entry was not found in the CCW Table");
        goto ErrReturn;
    }

    // compare the values to make sure that they match
    if (pCCWInfo->m_classID != wrappedClassID) {
        FAILURE(L"ClassID mismatch");
        goto ErrReturn;
    }

    if (IsSameGUID(pCCWInfo->m_GUID ,implementedIID) != TRUE) {
        FAILURE(L"GUID mismatch");
        goto ErrReturn;
    }
  

ErrReturn:
    return S_OK; // CLR ignores the return value
}

//*********************************************************************
//  Descripton: Overall verification to ensure the Vtable creation
//              and destuction was as expected. 
//  
//  Params:     IPrfCom * pPrfCom   - 
//                  pointer to test module containing helper functions
//
//  Return:     S_OK if method successful, else E_FAIL
//  
//  Notes:      n/a
//*********************************************************************

HRESULT Unit_CCW_Verify(IPrfCom * pPrfCom)
{
    HRESULT hr = S_OK;

    //
    // check the counters
    //
    if ((pPrfCom->m_COMClassicVTableCreated < pPrfCom->m_COMClassicVTableDestroyed) || 
        (pPrfCom->m_COMClassicVTableCreated == 0)) 
    {
        hr = E_FAIL;        
        FAILURE(L"Unexpected CCW Creation - Destruction Counters")
        FAILURE(L"CCW Creation Count: " << pPrfCom->m_COMClassicVTableCreated)
        FAILURE(L"CCW Destruction count: " << pPrfCom->m_COMClassicVTableDestroyed) 
        goto ErrReturn;
    }
	
    //
    // GC events, at least the number of successful ForceGC's
    //
    if (g_GcOperation.m_cRuntimeSuspensionCtr[(DWORD)COR_PRF_SUSPEND_FOR_GC] < 
        g_GcOperation.m_cForceGCSucceeded) {

        hr = E_FAIL;        
	    FAILURE(L"Unexpected Count Of Suspensions For GC")
    }

ErrReturn:
    return hr;
}

//*********************************************************************
//  Descripton: In this initialization step, we subscribe to profiler 
//              callbacks and also specify the necessary event mask. 
//  
//  Params:     IPrfCom * pPrfCom   - 
//                  pointer to test module containing helper functions
//              PMODULEMETHODTABLE pModuleMethodTable - 
//                  pointer to the method table which enables you
//                  to subscribe to profiler callbacks.
//
//  Return:     n/a
//  
//  Notes:      n/a
//*********************************************************************

void Unit_CCW_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize UnitTests extension");
    g_GcOperation.startGCThread();

    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_CCW | COR_PRF_MONITOR_SUSPENDS;
    pModuleMethodTable->VERIFY = (FC_VERIFY) &Unit_CCW_Verify;

    pModuleMethodTable->COMCLASSICVTABLECREATED = (FC_COMCLASSICVTABLECREATED) &Unit_CCW_COMClassicVTableCreated;
    pModuleMethodTable->COMCLASSICVTABLEDESTROYED = (FC_COMCLASSICVTABLEDESTROYED) &Unit_CCW_COMClassicVTableDestroyed;
    pModuleMethodTable->RUNTIMESUSPENDSTARTED = (FC_RUNTIMESUSPENDSTARTED) &Unit_CCW_RuntimeSuspendStarted;

    return;
}
