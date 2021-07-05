#include "stdafx.h"

MetadataSignatureBuilder::MetadataSignatureBuilder(BOOL autoCleanup) :
m_sigBytes(NULL),
m_sigBytesCapacity(0),
m_sigBytesLength(0),
m_autoCleanup(autoCleanup)
{
}

MetadataSignatureBuilder::MetadataSignatureBuilder(const MetadataSignatureBuilder &rhs)
{
    // The auto cleanup would be very bad if you are making copies because we aren't
    // deep copying the internal buffer.
    _ASSERTE(!rhs.m_autoCleanup);
    m_sigBytes = rhs.m_sigBytes;
    m_sigBytesCapacity = rhs.m_sigBytesCapacity;
    m_sigBytesLength = rhs.m_sigBytesLength;
    m_autoCleanup = FALSE;
}

MetadataSignatureBuilder & MetadataSignatureBuilder::operator=(const MetadataSignatureBuilder &rhs)
{
    // The auto cleanup would be very bad if you are making copies because we aren't
    // deep copying the internal buffer.
    _ASSERTE(!rhs.m_autoCleanup);
    m_sigBytes = rhs.m_sigBytes;
    m_sigBytesCapacity = rhs.m_sigBytesCapacity;
    m_sigBytesLength = rhs.m_sigBytesLength;
    m_autoCleanup = FALSE;
    return *this;
}

MetadataSignatureBuilder::~MetadataSignatureBuilder()
{
    if (m_autoCleanup)
    {
        DeleteSignature();
    }
}

void MetadataSignatureBuilder::GetSignature(PCCOR_SIGNATURE* ppSignature, ULONG* pcbSignature) const
{
    *ppSignature = (PCCOR_SIGNATURE)m_sigBytes;
    *pcbSignature = (ULONG)m_sigBytesLength;
}

PCCOR_SIGNATURE MetadataSignatureBuilder::GetSignature() const
{
    return (PCCOR_SIGNATURE)m_sigBytes;
}

ULONG MetadataSignatureBuilder::GetCbSignature()
{
    return (ULONG)m_sigBytesLength;
}

HRESULT MetadataSignatureBuilder::WriteCallingConvInfo(ULONG callConv)
{
    HRESULT hr = S_OK;
    IfFailRet(EnsureCapacity(m_sigBytesLength + 1));
    *(m_sigBytes + m_sigBytesLength) = (BYTE)(callConv & 0xFF);
    m_sigBytesLength++;
    return S_OK;
}

HRESULT MetadataSignatureBuilder::WriteData(ULONG data)
{
    HRESULT hr = S_OK;
    IfFailRet(EnsureCapacity(m_sigBytesLength + 4)); // the data might be up to 4 bytes, though it will probably be smaller
    m_sigBytesLength += CorSigCompressData(data, m_sigBytes + m_sigBytesLength);
    return S_OK;
}

HRESULT MetadataSignatureBuilder::WriteToken(mdToken token)
{
    HRESULT hr = S_OK;
    IfFailRet(EnsureCapacity(m_sigBytesLength + 4)); // the token might be up to 4 bytes, though it will probably be smaller
    m_sigBytesLength += CorSigCompressToken(token, m_sigBytes + m_sigBytesLength);
    return S_OK;
}

HRESULT MetadataSignatureBuilder::WriteElemType(CorElementType et)
{
    HRESULT hr = S_OK;
    IfFailRet(EnsureCapacity(m_sigBytesLength + 4)); // the token might be up to 4 bytes, though it will probably be smaller
    m_sigBytesLength += CorSigCompressData(et, m_sigBytes + m_sigBytesLength);
    return S_OK;
}

HRESULT MetadataSignatureBuilder::EnsureCapacity(ULONG32 newCapacity)
{
    BYTE* pNewSigBytes = NULL;
    if (m_sigBytesCapacity < newCapacity)
    {
        pNewSigBytes = new (nothrow)BYTE[newCapacity];
        IfNullRetOOM(pNewSigBytes);
        memset(pNewSigBytes, 0, newCapacity);
    }
    if (m_sigBytesLength != 0)
    {
        //(m_sigBytesLength != 0) => (m_sigBytesCapacity != 0) =>(m_sigBytes != NULL)
        _ASSERTE(m_sigBytes != NULL);
        memcpy(pNewSigBytes, m_sigBytes, m_sigBytesLength);
    }

    if (m_sigBytes != NULL)
        delete[] m_sigBytes;
    m_sigBytes = pNewSigBytes;
    m_sigBytesCapacity = newCapacity;
    return S_OK;
}

void MetadataSignatureBuilder::DeleteSignature()
{
    if (m_sigBytes != NULL)
        delete[] m_sigBytes;
    m_sigBytes = NULL;
    m_sigBytesLength = 0;
    m_sigBytesCapacity = 0;
}