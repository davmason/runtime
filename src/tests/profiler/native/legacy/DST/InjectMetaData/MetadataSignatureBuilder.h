#pragma once

struct MetadataSignatureBuilder
{
public:
    MetadataSignatureBuilder(BOOL autoCleanup = FALSE);
    MetadataSignatureBuilder(const MetadataSignatureBuilder &rhs);
    ~MetadataSignatureBuilder();
    void GetSignature(PCCOR_SIGNATURE*ppSignature, ULONG* pcbSignature) const;
    PCCOR_SIGNATURE GetSignature() const;
    ULONG GetCbSignature();
    HRESULT WriteCallingConvInfo(ULONG callConv);
    HRESULT WriteData(ULONG data);
    HRESULT WriteToken(mdToken token);
    HRESULT WriteElemType(CorElementType et);

    void DeleteSignature();

    MetadataSignatureBuilder & operator=(const MetadataSignatureBuilder &rhs);

protected:
    HRESULT EnsureCapacity(ULONG32 newCapacity);

private:
    BYTE* m_sigBytes;
    ULONG32 m_sigBytesCapacity;
    ULONG32 m_sigBytesLength;
    BOOL m_autoCleanup;
};