// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "ProfilerCommon.h"
#include "simplerwlock.hpp"

BOOL SimpleRWLock::TryEnterRead()
{
    LONG RWLock;

    do {
        RWLock = m_RWLock;
        if( RWLock == -1 ) return FALSE;
        _ASSERTE (RWLock >= 0);
    } while( RWLock != InterlockedCompareExchange( &m_RWLock, RWLock+1, RWLock ));

    m_dwThreadIdLastOwner = GetCurrentThreadId();

    return TRUE;
}

//=====================================================================        
void SimpleRWLock::EnterRead()
{
    BOOL bInfinite = TRUE;
    while (bInfinite)
    {
        // prevent writers from being starved. This assumes that writers are rare and 
        // dont hold the lock for a long time. 
        while (IsWriterWaiting())
        {
            int spinCount = m_spinCount;
            while (spinCount > 0) {
                spinCount--;
                YieldProcessor();
            }
            Sleep(0);
        }

        if (TryEnterRead())
        {
            return;
        }

        DWORD i = 50;
        do
        {
            if (TryEnterRead())
            {
                return;
            }

            // Delay by approximately 2*i clock cycles (Pentium III).
            // This is brittle code - future processors may of course execute this
            // faster or slower, and future code generators may eliminate the loop altogether.
            // The precise value of the delay is not critical, however, and I can't think
            // of a better way that isn't machine-dependent - petersol.
            int sum = 0;
            for (int delayCount = i; --delayCount; ) 
            {
                sum += delayCount;
                YieldProcessor();           // indicate to the processor that we are spining 
            }
            if (sum == 0)
            {
                // never executed, just to fool the compiler into thinking sum is live here,
                // so that it won't optimize away the loop.
                static char dummy;
                dummy++;
            }
            // exponential backoff: wait 3 times as long in the next iteration
            i = i*3;
        }
        while (i < 50000);

        Sleep(0);
    }
}

//=====================================================================        
BOOL SimpleRWLock::TryEnterWrite()
{
    LONG RWLock = InterlockedCompareExchange( &m_RWLock, -1, 0 );

    _ASSERTE (RWLock >= 0 || RWLock == -1);

    if( RWLock ) {
        return FALSE;
    }

    m_dwThreadIdLastOwner = GetCurrentThreadId();

    // WriterLock obtained - reset the flag telling Readers
    // that a Writer is waiting (if there were multiple
    // Writers waiting the other waiting threads will set
    // the flag again so they won't get starved.
    ResetWriterWaiting();

    return TRUE;
}

//=====================================================================        
void SimpleRWLock::EnterWrite()
{
    BOOL bInfinite = TRUE;
    while (bInfinite)
    {
        if (TryEnterWrite())
        {
            return;
        }

        DWORD i = 50;
        do
        {
            if (TryEnterWrite())
            {
                return;
            }

            // notify potential readers that a writer is waiting
            SetWriterWaiting();

            // Delay by approximately 2*i clock cycles (Pentium III).
            // This is brittle code - future processors may of course execute this
            // faster or slower, and future code generators may eliminate the loop altogether.
            // The precise value of the delay is not critical, however, and I can't think
            // of a better way that isn't machine-dependent - petersol.
            int sum = 0;
            for (int delayCount = i; --delayCount; ) 
            {
                sum += delayCount;
                YieldProcessor();           // indicate to the processor that we are spining 
            }
            if (sum == 0)
            {
                // never executed, just to fool the compiler into thinking sum is live here,
                // so that it won't optimize away the loop.
                static char dummy;
                dummy++;
            }
            // exponential backoff: wait 3 times as long in the next iteration
            i = i*3;
        }
        while (i < 50000);

        Sleep(0);
    }
}

