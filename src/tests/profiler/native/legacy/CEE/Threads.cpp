#include "ProfilerCommon.h"
#include <assert.h>

class TestThreadNameChanged
{
    public:

        TestThreadNameChanged();

        HRESULT Verify(IPrfCom * pPrfCom);        

        #pragma region static_wrapper_methods

        static HRESULT ThreadNameChangedWrapper(IPrfCom * pPrfCom, ThreadID managedThreadId, ULONG cchName, WCHAR name[])
        {
            return STATIC_CLASS_CALL(TestThreadNameChanged)->ThreadNameChanged(pPrfCom, managedThreadId, cchName, name);
        }

        #pragma endregion

    private:

        #pragma region callback_handler_prototypes

        HRESULT ThreadNameChanged(IPrfCom * pPrfCom, ThreadID managedThreadId, ULONG cchName, WCHAR name[]);

        #pragma endregion

        ULONG m_ulSuccess;
        ULONG m_ulFailure;
};


/*
 *  Initialize the TestThreadNameChanged class.  
 */
TestThreadNameChanged::TestThreadNameChanged()
{
    // Initialize result counters
    m_ulSuccess   = 0;
    m_ulFailure   = 0;
}


HRESULT TestThreadNameChanged::ThreadNameChanged(IPrfCom * pPrfCom, ThreadID managedThreadId, ULONG cchName, PROFILER_WCHAR nameArg[])
{
    PLATFORM_WCHAR name[STRING_LENGTH];
    ConvertProfilerWCharToPlatformWChar(name, STRING_LENGTH, nameArg, cchName);
    // The thread name provided is not null terminated, so null terminate it
    assert((cchName + 1) < STRING_LENGTH);
    name[cchName] = 0;

    if (cchName == 0x00)
    {
        if (nameArg != NULL)
        {
            m_ulFailure++;
            FAILURE(L"Buffer passed with NULL cchName");
            return E_FAIL;
        }
        DISPLAY(L"ThreadNameChanged with NULL name.");
        m_ulSuccess++;
        return S_OK;
    }

    wstring threadName(name);
    if (threadName.length() != cchName)
    {
        m_ulFailure++;
        FAILURE(L"Reported and actual thread name lengths not equal. " << threadName.length() << " != " << cchName);
    }

    DISPLAY(L"ThreadNameChanged Callback.  New Name: " << threadName);
    
    AppDomainID appDomainID = NULL;
    MUST_PASS(PINFO->GetThreadAppDomain(managedThreadId, &appDomainID));

    switch (PPRFCOM->m_ThreadNameChanged.load())
    {
        case 1:
            if(threadName != L"MainThread")
            {
                m_ulFailure++;
                FAILURE(L"First names do not match " << threadName << L" != MainThread");
            }
            break;

        case 2:
            if(threadName != L"SecondThread")
            {
                m_ulFailure++;
                FAILURE(L"Second names do not match " << threadName << L" != SecondThread");
            }
            break;

        case 3:
            if(threadName != L"ThirdThread")
            {
                m_ulFailure++;
                FAILURE(L"Third names do not match " << threadName << L" != ThirdThread");
            }
            break;

        default:
            FAILURE(L"Extra ThreadNameChanged callback received.");
            break;
    }
    
    m_ulSuccess++;
    return S_OK;
}


HRESULT TestThreadNameChanged::Verify(IPrfCom * pPrfCom)
{
    DISPLAY(L"ThreadNameChanged Verification...\n");

    if ((m_ulFailure == 0) && (m_ulSuccess == 4))
    {
        DISPLAY(L"Test passed.")
        return S_OK;
    }
    else
    {
        FAILURE(L"Either some checks failed, or no successful checks were completed.\n" <<
                L"\tm_ulFailure = " << m_ulFailure << L" m_ulSuccess = " << m_ulSuccess);
        return E_FAIL;
    }
}


HRESULT ThreadNameChangedVerify(IPrfCom * pPrfCom)
{
    LOCAL_CLASS_POINTER(TestThreadNameChanged);
    MUST_PASS(pTestThreadNameChanged->Verify(pPrfCom))
    FREE_CLASS_POINTER(TestThreadNameChanged);
    return S_OK;
}

void ThreadNameChangedInit(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    DISPLAY(L"Initialize ThreadNameChanged module\n");

    // Create and save an instance of test class
    SET_CLASS_POINTER(new TestThreadNameChanged());

    pModuleMethodTable->FLAGS = COR_PRF_MONITOR_THREADS;

    REGISTER_CALLBACK(VERIFY, ThreadNameChangedVerify);
    REGISTER_CALLBACK(THREADNAMECHANGED, TestThreadNameChanged::ThreadNameChangedWrapper);
    
    return;
}


