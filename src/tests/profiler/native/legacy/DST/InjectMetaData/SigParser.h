// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#pragma once

//------------------------------------------------------------------------
// Encapsulates how compressed integers and typeref tokens are encoded into
// a bytestream.
//------------------------------------------------------------------------
class SigParser
{
private:
    PCCOR_SIGNATURE m_ptr;
    DWORD           m_dwLen;

    FORCEINLINE void SkipBytes(ULONG cb)
    {
        assert(cb <= m_dwLen);
        m_ptr += cb;
        m_dwLen -= cb;
    }

public:
    SigParser()
    {
        m_ptr = NULL;
        m_dwLen = 0;
    }

    FORCEINLINE SigParser(const SigParser &sig) :
        m_ptr(sig.m_ptr), m_dwLen(sig.m_dwLen)
    {
    }

    FORCEINLINE SigParser(PCCOR_SIGNATURE ptr, DWORD len)
    {
        m_ptr = ptr;
        m_dwLen = len;
    }

    //=========================================================================
    // The RAW interface for reading signatures.  You see exactly the signature,
    // apart from custom modifiers which for historical reasons tend to get eaten.
    //
    // DO NOT USE THESE METHODS UNLESS YOU'RE TOTALLY SURE YOU WANT
    // THE RAW signature.  You nearly always want GetElemTypeClosed() or 
    // PeekElemTypeClosed() or one of the MetaSig functions.  See the notes above.
    // These functions will return E_T_INTERNAL, E_T_VAR, E_T_MVAR and such
    // so the caller must be able to deal with those
    //=========================================================================

    //------------------------------------------------------------------------
    // Remove one compressed integer (using CorSigUncompressData) from
    // the head of the stream and return it.
    //------------------------------------------------------------------------
    FORCEINLINE HRESULT GetData(ULONG* data)
    {
        ULONG sizeOfData = 0;
        ULONG tempData;

        if (data == NULL)
            data = &tempData;

        HRESULT hr = CorSigUncompressData(m_ptr, m_dwLen, data, &sizeOfData);

        if (SUCCEEDED(hr))
        {
            SkipBytes(sizeOfData);
        }

        return hr;
    }


    //-------------------------------------------------------------------------
    // Remove one byte and return it.
    //-------------------------------------------------------------------------
    FORCEINLINE HRESULT GetByte(BYTE *data)
    {
        if (m_dwLen > 0)
        {
            if (data != NULL)
                *data = *m_ptr;

            SkipBytes(1);

            return S_OK;
        }

        if (data != NULL)
            *data = 0;
        return META_E_BAD_SIGNATURE;
    }

    //-------------------------------------------------------------------------
    // Peek at value of one byte and return it.
    //-------------------------------------------------------------------------
    FORCEINLINE HRESULT PeekByte(BYTE *data)
    {
        if (m_dwLen > 0)
        {
            if (data != NULL)
                *data = *m_ptr;

            return S_OK;
        }

        if (data != NULL)
            *data = 0;
        return META_E_BAD_SIGNATURE;
    }

    // Inlined version
    FORCEINLINE HRESULT GetElemType(CorElementType * etype)
    {
        if (m_dwLen > 0)
        {
            CorElementType typ = (CorElementType) * m_ptr;
            if (etype != NULL)
            {
                * etype = typ;
            }
            SkipBytes(1);
            return S_OK;
        }
        return META_E_BAD_SIGNATURE;
    }

    // Note: Calling convention is always one byte, not four bytes
    HRESULT PeekCallingConvInfo(ULONG *data)
    {
        ULONG tmpData;

        if (data == NULL)
            data = &tmpData;

        HRESULT hr = CorSigUncompressCallingConv(m_ptr, m_dwLen, data);

        return hr;
    }

    // Note: Calling convention is always one byte, not four bytes
    HRESULT GetCallingConvInfo(ULONG * data) 
    {
        ULONG tmpData;

        if (data == NULL)
            data = &tmpData;

        HRESULT hr = CorSigUncompressCallingConv(m_ptr, m_dwLen, data);
        if (SUCCEEDED(hr))
        {
            SkipBytes(1);
        }

        return hr;
    }   

    HRESULT GetCallingConv(ULONG* data)
    {   
        ULONG info;
        HRESULT hr = GetCallingConvInfo(&info);

        if (SUCCEEDED(hr) && data != NULL)
        {
            *data = IMAGE_CEE_CS_CALLCONV_MASK & info;
        }

        return hr; 
    }   

    //------------------------------------------------------------------------
    // Non-destructive read of compressed integer.
    //------------------------------------------------------------------------
    HRESULT PeekData(ULONG *data) const
    {
        _ASSERTE(data != NULL);

        ULONG sizeOfData = 0;
        return CorSigUncompressData(m_ptr, m_dwLen, data, &sizeOfData);
    }


    //------------------------------------------------------------------------
    // Non-destructive read of element type.
    //------------------------------------------------------------------------
    FORCEINLINE HRESULT PeekElemType(CorElementType *etype) const
    {
        _ASSERTE(etype != NULL);

        if (m_dwLen > 0)
        {
            CorElementType typ = (CorElementType) * m_ptr;
            *etype = typ;
            return S_OK;
        }

        return META_E_BAD_SIGNATURE;
    }

    //------------------------------------------------------------------------
    // Is this at the Sentinal (the ... in a varargs signature) that marks
    // the begining of varguments that are not decared at the target

    bool AtSentinel() const
    {
        if (m_dwLen > 0)
            return *m_ptr == ELEMENT_TYPE_SENTINEL;
        else
            return false;
    }

    //------------------------------------------------------------------------
    // Removes a compressed metadata token and returns it.
    //------------------------------------------------------------------------
    FORCEINLINE 
    HRESULT GetToken(mdToken * token)
    {
        DWORD dwLen;
        mdToken tempToken;

        if (token == NULL)
            token = &tempToken;

        HRESULT hr = CorSigUncompressToken(m_ptr, m_dwLen, token, &dwLen);

        if (SUCCEEDED(hr))
        {
            SkipBytes(dwLen);
        }

        return hr;
    }

};  // class SigParser
