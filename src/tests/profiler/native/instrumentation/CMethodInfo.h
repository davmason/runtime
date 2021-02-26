#include "stdafx.cpp"
#include <wchar.h>

#define PrologBufferSize 256


#pragma warning(push)
#pragma warning( disable : 4996)

class CMethodInfo
{
public:
    CMethodInfo()
    {    
        m_assemblyID = NULL;
        m_classID = NULL;
        m_functionID = NULL;
        m_moduleID = NULL;
        m_pBaseLoadAddress = NULL;
        
        m_bIsStatic = FALSE;
        m_bIsPartOfCER = FALSE;

        m_signatureBlob = NULL;
        m_cbSignatureBlob = 0;
        m_ulCodeRVA = 0;
        m_dwMethodAttr = 0;
        m_dwMethodImplFlags = 0;
        m_cTypeArgs = 0;

        m_tkTypeDef = NULL;
        m_pMethodHeader = NULL;
        m_cbMethodSize = 0;
        m_pNewMethodHeader = NULL;
        m_cbNewMethodSize = 0;
        
        m_ILProlog = new BYTE[PrologBufferSize];
        m_cbILProlog = 0;
        m_cbNewMethodSize = 0;
        m_tkMethodDef = NULL;

        m_nArgumentCount = 0;
        
        m_wszClassName[0] = '\0';
        m_wszMethodName[0] = '\0';
        m_wszModuleName[0] = '\0';
        m_wszModuleNameNoPath[0] = '\0';
        m_wszMethodParameters[0] = '\0';	
    }    
    
public:

    BOOL CMethodInfo::CanModifyTinyHeader()
    {
        _ASSERTE(IsTinyHeader()); 
        if((GetMethodCodeSize() + m_cbILProlog) >= 50)
        {
            return FALSE;
        }
        return TRUE;
    }

    int CMethodInfo::GetNewMethodExtraSize(int& nAlignment)
    {
        return GetMethodExtraSize(nAlignment,FALSE); 
    }
  
    int CMethodInfo::GetOldMethodExtraSize(int& nAlignment)
    {
        return GetMethodExtraSize(nAlignment,TRUE);
    }
private:
    int CMethodInfo::GetMethodExtraSize(int& nAlignment, BOOL calculateOld)
    {
        LPCBYTE pHeader;

        if(calculateOld)
        {
            _ASSERTE(m_pMethodHeader != NULL);
            pHeader = m_pMethodHeader;
        }
        else
        {
            _ASSERTE(m_pNewMethodHeader != NULL);
            pHeader = m_pNewMethodHeader;
        }
        _ASSERTE(pHeader);
        _ASSERTE(HasSEH());
        int nExtraSize = -1;
        nAlignment = 0;
        COR_ILMETHOD_FAT* pMethod = (COR_ILMETHOD_FAT*)pHeader;
        const COR_ILMETHOD_SECT* pFirstSect = pMethod->GetSect();

        _ASSERTE(pFirstSect);
        ULONG cbMethodSize = pMethod->Size * 4;
        _ASSERTE(cbMethodSize == HEADERSIZE_FAT);
        cbMethodSize += pMethod->GetCodeSize();
        //nAlignment = (int)( (BYTE*)pFirstSect - (BYTE*)((BYTE*)pHeader + cbMethodSize) );
        nAlignment = (int)( (ULONGLONG)pFirstSect - (ULONGLONG)pHeader - cbMethodSize );
        _ASSERTE( nAlignment >= 0 );

        const COR_ILMETHOD_SECT* pLastSect  = pFirstSect;
	    if ( pFirstSect )
	    {
		    // Skip to the next extra sect.
            while ( pLastSect->More() )
                pLastSect = pLastSect->Next();
            
            // Skip to where the end of this sect it
            BYTE * pEnd = (BYTE *)pLastSect + pLastSect->DataSize();
            
            // Extra is delta from first extra sect to first sect past this method.
            nExtraSize = (int)( pEnd - (BYTE*)pFirstSect );
	    }
	    return nExtraSize;
    }
public:
    inline BOOL CMethodInfo::HasSEH()
    {
        if(IsFatHeader())
        {
            if(((IMAGE_COR_ILMETHOD*)m_pMethodHeader)->Fat.Flags & CorILMethod_MoreSects)
            {
                return TRUE;
            }
        }
        return FALSE;
    }

    void CMethodInfo::ConvertToFatHeader()
    {
        _ASSERTE(IsTinyHeader());
        static BYTE newFatHeader[sizeof(COR_ILMETHOD_FAT) + (64 - sizeof(COR_ILMETHOD_TINY))];

        COR_ILMETHOD_TINY* tinyMethod = (COR_ILMETHOD_TINY*)m_pMethodHeader;
        COR_ILMETHOD_FAT* fatMethod = (COR_ILMETHOD_FAT*)newFatHeader;

        fatMethod->Flags = CorILMethod_FatFormat;
        fatMethod->Size = sizeof(COR_ILMETHOD_FAT) / sizeof(DWORD);
        fatMethod->MaxStack = tinyMethod->GetMaxStack();
        fatMethod->CodeSize = tinyMethod->GetCodeSize();
        fatMethod->LocalVarSigTok = tinyMethod->GetLocalVarSigTok();

        // Copy the rest of the method body.
        memcpy( fatMethod->GetCode(), tinyMethod->GetCode(), tinyMethod->GetCodeSize() );

        m_cbMethodSize = sizeof(COR_ILMETHOD_FAT) + fatMethod->GetCodeSize();
        m_pMethodHeader = newFatHeader;
    }

    inline BOOL CMethodInfo::IsFatHeader()
    {
        //Can't access the Header as Fat until we know its Fat.
        //  Only the Fat Header is aligned.
        return (!IsTinyHeader());
    }

    inline BOOL CMethodInfo::IsTinyHeader()
    {
        _ASSERTE(m_pMethodHeader);
        IMAGE_COR_ILMETHOD *pMethod = (IMAGE_COR_ILMETHOD*)m_pMethodHeader;
        //Copied from CorHlpr.h
        return (pMethod->Tiny.Flags_CodeSize & (CorILMethod_FormatMask >> 1)) == CorILMethod_TinyFormat;
    }

    inline ULONG CMethodInfo::GetMethodCodeSize()
    {
        IMAGE_COR_ILMETHOD *pMethod = (IMAGE_COR_ILMETHOD*)m_pMethodHeader;

        if(IsFatHeader())
        {
            _ASSERTE(!IsTinyHeader());
            return (ULONG) pMethod->Tiny.Flags_CodeSize;
        }
        else
        {
            _ASSERTE(!IsFatHeader());
            //Copied from CorHlpr.h
            return (ULONG) (((unsigned)pMethod->Tiny.Flags_CodeSize) >> (CorILMethod_FormatShift-1));
        }

    }

    inline BOOL CMethodInfo::IsStaticMethod()
    {
        return (BOOL)((m_dwMethodAttr & mdStatic) != 0);
    }

    PLATFORM_WCHAR* CMethodInfo::FullSignature()
    {
        PLATFORM_WCHAR sig[10*MAX_PATH];
        sig[0] = '\0';
        wcscat_s(sig, 10*MAX_PATH, m_wszModuleNameNoPath);
        wcscat_s(sig, 10*MAX_PATH, L"!");
        wcscat_s(sig, 10*MAX_PATH, m_wszClassName);
        wcscat_s(sig, 10*MAX_PATH, L"::");
        wcscat_s(sig, 10*MAX_PATH, m_wszMethodName);
        wcscat_s(sig, 10*MAX_PATH, L"(");
        wcscat_s(sig, 10*MAX_PATH, m_wszMethodParameters);
        wcscat_s(sig, 10*MAX_PATH, L")");
        wcscpy(m_wszFullSignature,sig);
        return m_wszFullSignature;
    }
private:
    PLATFORM_WCHAR m_wszFullSignature[10*MAX_PATH];

public:
    AssemblyID m_assemblyID;
    ClassID m_classID;
    FunctionID m_functionID;
    ModuleID m_moduleID;    
    mdToken m_tkMethodDef;
    PLATFORM_WCHAR m_wszClassName[MAX_PATH];
    PLATFORM_WCHAR m_wszMethodName[MAX_PATH];
    PLATFORM_WCHAR m_wszModuleName[MAX_PATH];
    PLATFORM_WCHAR m_wszModuleNameNoPath[MAX_PATH];

    LPCBYTE m_pBaseLoadAddress;

    BOOL m_bIsStatic;
    BOOL m_bIsPartOfCER;

    //Store information about method signature,
    //  parameters, and asorted other goodies
    PCCOR_SIGNATURE m_signatureBlob;
    ULONG m_cbSignatureBlob;
    ULONG m_ulCodeRVA;

    DWORD m_dwMethodAttr;
    DWORD m_dwMethodImplFlags;

    ULONG32 m_cTypeArgs;
    ClassID m_typeArgs[MAX_PATH];

    mdTypeDef m_tkTypeDef;

    //<TODO> Need to parse Method Args
    ULONG m_nArgumentCount;
    PLATFORM_WCHAR m_wszMethodParameters[10 * MAX_PATH];
    
    //This is the Original method header and method size
    //
    LPCBYTE m_pMethodHeader;
    ULONG m_cbMethodSize;

    //This is the new method body
    //  Contains the header and the ILFunctionBody
    LPCBYTE m_pNewMethodHeader;
    ULONG m_cbNewMethodSize;

    //The custom Prolog to add into the function
    BYTE *m_ILProlog;
    ULONG m_cbILProlog;


};
#pragma warning(pop)
