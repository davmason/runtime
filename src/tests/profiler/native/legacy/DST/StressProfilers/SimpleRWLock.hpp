// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _SimpleRWLock_hpp_
#define _SimpleRWLock_hpp_

class SimpleRWLock
{
private:
    BOOL IsWriterWaiting()
    {
        return m_WriterWaiting != 0;
    }

    void SetWriterWaiting()
    {
        InterlockedExchange(&m_WriterWaiting, 1);
    }

    void ResetWriterWaiting()
    {
        InterlockedExchange(&m_WriterWaiting, 0);
    }

    // lock used for R/W synchronization
    volatile LONG m_RWLock;     

    // spin count for a reader waiting for a writer to release the lock
    LONG m_spinCount;

    DWORD m_dwThreadIdLastOwner;

    // used to prevent writers from being starved by readers
    // we currently do not prevent writers from starving readers since writers 
    // are supposed to be rare.
    volatile LONG m_WriterWaiting;

    static void AcquireReadLock(SimpleRWLock *s) { s->EnterRead(); }
    static void ReleaseReadLock(SimpleRWLock *s) { s->LeaveRead(); }

    static void AcquireWriteLock(SimpleRWLock *s) { s->EnterWrite(); }
    static void ReleaseWriteLock(SimpleRWLock *s) { s->LeaveWrite(); }

public:
    SimpleRWLock ()
    {
        m_RWLock = 0;
        m_spinCount = 2000;
        m_WriterWaiting = FALSE;
        m_dwThreadIdLastOwner = 0;
    }
    // Acquire the reader lock.
    BOOL TryEnterRead();
    void EnterRead();

    // Acquire the writer lock.
    BOOL TryEnterWrite();
    void EnterWrite();

    // Leave the reader lock.
    void LeaveRead()
    {
        InterlockedDecrement(&m_RWLock);
    }

    // Leave the writer lock.
    void LeaveWrite()
    {
        InterlockedExchange (&m_RWLock, 0);
    }

};

#endif
