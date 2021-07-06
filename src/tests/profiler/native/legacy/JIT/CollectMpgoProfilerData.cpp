// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "JITCommon.h"
#include <assert.h>
// Legacy support
#include "../LegacyCompat.h"
// Legacy support

#ifdef __clang__
#pragma clang diagnostic ignored "-Wformat"
#endif // __clang__

#ifdef OSX
#include <mach-o/dyld.h>
#endif

#pragma warning(push)
#pragma warning( disable : 4996)

#undef DISPLAY
#undef FAILURE
#define DISPLAY( message )  {                                                                       \
                                assert(g_pPrfCom != NULL);                                            \
                                std::wstringstream temp;                                            \
                                temp << message;                                                    \
                                g_pPrfCom->Display(temp);                                             \
                            }

// Writes failure to log file
#define FAILURE( message )  {                                                                       \
                                assert(g_pPrfCom != NULL);                                            \
                                std::wstringstream temp;                                            \
                                temp << message;                                                    \
                                g_pPrfCom->Error(temp) ;                                              \
                            }    // Prints to log file when logging is enabled. (set PRF_DBG=1)

#ifdef __linux__
#include <unistd.h>
#endif // __linux__

IPrfCom *g_pPrfCom = NULL;

struct MpgoProfileData
{
    wchar_t moduleName[512];
    wchar_t className[512];
    wchar_t functionName[512];
    int order;
    LPCBYTE baseLoadAddress;
    ModuleID moduleID;
    ClassID classID;
    FunctionID functionID;
    ULONG32 codeInfoLength;
    ULONG32 mapInfoLength;
    COR_PRF_CODE_INFO* codeInfo;
    COR_DEBUG_IL_TO_NATIVE_MAP* mapInfo;
};

#define XML_FORMAT_STRING_HEADER "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
#define XML_FORMAT_STRING_ROOT_START "<MpgoProfilerData version=\"1.0\">\n"
#define XML_FORMAT_STRING_METHOD_START "<Method methodID=\"0x%llx\" classID=\"0x%llx\" moduleID=\"0x%llx\" methodName=\"%s::%s\" moduleName=\"%s\" order=\"%d\" moduleBaseAddress=\"0x%p\" >\n"
#define XML_FORMAT_STRING_BLOCKINFO_START "<CodeRegions>\n"
#define XML_FORMAT_STRING_BLOCKINFO "<CodeRegion startAddress=\"0x%p\" size=\"0x%llx\" />\n"
#define XML_FORMAT_STRING_BLOCKINFO_END "</CodeRegions>\n"
#define XML_FORMAT_STRING_MAPINFO_START "<Blocks>\n"
#define XML_FORMAT_STRING_MAPINFO "<Block ilOffset=\"0x%x\" nativeStartOffset=\"0x%x\" nativeEndOffset=\"0x%x\" />\n"
#define XML_FORMAT_STRING_MAPINFO_END "</Blocks>\n"
#define XML_FORMAT_STRING_METHOD_END "</Method>\n"
#define XML_FORMAT_STRING_ROOT_END "</MpgoProfilerData>\n"

typedef map<wstring, MpgoProfileData*> MpgoProfileDataMap;

class CollectMpgoProfilerData
{
public:

    CollectMpgoProfilerData(IPrfCom * pPrfCom);
    ~CollectMpgoProfilerData();

    HRESULT FuncEnter2(IPrfCom * pPrfCom,
        FunctionID mappedFuncId,
        UINT_PTR clientData,
        COR_PRF_FRAME_INFO func,
        COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo);

	HRESULT FunctionEnter3WithInfo(IPrfCom * pPrfCom,
        FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo);

    static HRESULT FuncEnter2Wrapper(IPrfCom * pPrfCom,
        FunctionID mappedFuncId,
        UINT_PTR clientData,
        COR_PRF_FRAME_INFO func,
        COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo)
    {
        return Instance()->FuncEnter2(pPrfCom, mappedFuncId, clientData, func, argumentInfo);
    }

	static HRESULT FunctionEnter3WithInfoWrapper(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo)
    {
        return Instance()->FunctionEnter3WithInfo(pPrfCom, functionIDOrClientID, eltInfo);
    }

	static HRESULT FunctionLeave3WithInfoWrapper(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo)
    {
        return Instance()->FunctionLeave3WithInfo(pPrfCom, functionIDOrClientID, eltInfo);
    }

	static HRESULT FunctionTailCall3WithInfoWrapper(IPrfCom * pPrfCom, FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo)
    {
        return Instance()->FunctionTailCall3WithInfo(pPrfCom, functionIDOrClientID, eltInfo);
    }

    HRESULT FuncTailcall2(IPrfCom * pPrfCom,
        FunctionID mappedFuncId,
        UINT_PTR clientData,
        COR_PRF_FRAME_INFO func);

	HRESULT FunctionTailCall3WithInfo(IPrfCom * pPrfCom,
        FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo);

    static HRESULT FuncTailcall2Wrapper(IPrfCom * pPrfCom,
        FunctionID mappedFuncId,
        UINT_PTR clientData,
        COR_PRF_FRAME_INFO func)
    {
        return Instance()->FuncTailcall2(pPrfCom, mappedFuncId, clientData, func);
    }

    HRESULT FuncLeave2(IPrfCom * pPrfCom,
        FunctionID mappedFuncId,
        UINT_PTR clientData,
        COR_PRF_FRAME_INFO func,
        COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange);

	HRESULT FunctionLeave3WithInfo(IPrfCom * pPrfCom,
		FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo);

    static HRESULT FuncLeave2Wrapper(IPrfCom * pPrfCom,
        FunctionID mappedFuncId,
        UINT_PTR clientData,
        COR_PRF_FRAME_INFO func,
        COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange)
    {
        return Instance()->FuncLeave2(pPrfCom, mappedFuncId, clientData, func, retvalRange);
    }


    HRESULT Verify(IPrfCom * pPrfCom);

    static CollectMpgoProfilerData* Instance()
    {
        return This;
    }

    void SetCodeInfo(MpgoProfileData *mpd, ULONG32 codeInfoLength, COR_PRF_CODE_INFO codeInfo[]);
    void SetMapInfo(MpgoProfileData *mpd, ULONG32 mapInfoLength, COR_DEBUG_IL_TO_NATIVE_MAP mapInfo[]);
    void WriteMpgoProfilerData();
    void GetMpgoProfileFileName(wstring& filename);
    void TranslateXmlEntities(wstring& data);
    void FreeMpgoProfilerDataMemory();

    MpgoProfileDataMap mpgoProfilerDataMap;
    static CollectMpgoProfilerData* This;
    std::mutex criticalSection;
    int orderCounter;
};

CollectMpgoProfilerData* CollectMpgoProfilerData::This = NULL;

CollectMpgoProfilerData::CollectMpgoProfilerData(IPrfCom *pPrfCom)
{
    orderCounter = 1;  // used to store the method order

    // Static this pointer
    CollectMpgoProfilerData::This = this;
}

CollectMpgoProfilerData::~CollectMpgoProfilerData()
{
    FreeMpgoProfilerDataMemory();
    // Static this pointer
    CollectMpgoProfilerData::This = NULL;

}

void CollectMpgoProfilerData::SetCodeInfo(MpgoProfileData *mpd, ULONG32 codeInfoLength, COR_PRF_CODE_INFO codeInfo[])
{
    mpd->codeInfoLength = codeInfoLength;

    if(codeInfoLength >0)
    {
        mpd->codeInfo = new COR_PRF_CODE_INFO[codeInfoLength];

        for(ULONG32 i =0; i<codeInfoLength; i++)
        {
            mpd->codeInfo[i].startAddress = codeInfo[i].startAddress;
            mpd->codeInfo[i].size = codeInfo[i].size;
        }
    }
}

void CollectMpgoProfilerData::SetMapInfo(MpgoProfileData *mpd, ULONG32 mapInfoLength, COR_DEBUG_IL_TO_NATIVE_MAP mapInfo[])
{
    mpd->mapInfoLength = mapInfoLength;

    if(mapInfoLength >0)
    {
        mpd->mapInfo = new COR_DEBUG_IL_TO_NATIVE_MAP[mapInfoLength];

        for(ULONG32 i =0; i<mapInfoLength; i++)
        {
            mpd->mapInfo[i].ilOffset = mapInfo[i].ilOffset;
            mpd->mapInfo[i].nativeStartOffset = mapInfo[i].nativeStartOffset;
            mpd->mapInfo[i].nativeEndOffset = mapInfo[i].nativeEndOffset;
        }
    }
}

void CollectMpgoProfilerData::TranslateXmlEntities(wstring& data)
{
    wstring ::size_type idx;
    while((idx = data.find(L"<")) != wstring::npos)
        data = data.replace(idx, 1, L"&lt;");
    while((idx = data.find(L">")) != wstring::npos)
        data = data.replace(idx, 1, L"&gt;");
}

HRESULT CollectMpgoProfilerData::FuncEnter2(IPrfCom * pPrfCom,
                                            FunctionID funcID,
                                            UINT_PTR /*clientData*/,
                                            COR_PRF_FRAME_INFO func,
                                            COR_PRF_FUNCTION_ARGUMENT_INFO * /*argumentInfo*/)
{
    const int MAX_BUF_SIZE = 1024;
    wchar_t buf[MAX_BUF_SIZE];
    const int MAX_CODE_INFOS = 1000;
    NewArrayHolder<COR_PRF_CODE_INFO> codeInfo(new COR_PRF_CODE_INFO[MAX_CODE_INFOS]);
    ULONG32 codeInfoCount;

    const int MAX_MAP_INFOS = 15000;
    NewArrayHolder<COR_DEBUG_IL_TO_NATIVE_MAP> mapInfo(new COR_DEBUG_IL_TO_NATIVE_MAP[MAX_MAP_INFOS]);
    ULONG32 mapInfoCount;

    ModuleID modID;
    ClassID classID;
    mdToken token;
    ClassID typeArgs[SHORT_LENGTH];
    ULONG32 nTypeArgs;

    MUST_PASS(PINFO->GetFunctionInfo2(
        funcID,
        func,
        &classID,
        &modID,
        &token,
        SHORT_LENGTH,
        &nTypeArgs,
        typeArgs));

    WCHAR_STR( funcName );
    MUST_PASS(PPRFCOM->GetFunctionIDName(funcID, funcName, func, FALSE));

    WCHAR_STR( moduleName );
    MUST_PASS(PPRFCOM->GetModuleIDName(modID, moduleName, FALSE));

    WCHAR_STR( className );
    MUST_PASS(PPRFCOM->GetClassIDName(classID, className, FALSE));

	wstring key;
    swprintf_s(buf, MAX_BUF_SIZE, L"0x%llx0x%llx0x%llx", modID, classID, funcID);
    key = buf;

    if(mpgoProfilerDataMap.find(key) == mpgoProfilerDataMap.end())
    {
        MUST_PASS(pPrfCom->m_pInfo->GetCodeInfo2(funcID,
            MAX_CODE_INFOS,
            &codeInfoCount,
            codeInfo));

        MUST_PASS(pPrfCom->m_pInfo->GetILToNativeMapping(funcID,
            MAX_MAP_INFOS,
            &mapInfoCount,
            mapInfo));

        DWORD length;
        WCHAR modName[1024];
        LPCBYTE ppBaseLoadAddress = NULL;

        MUST_PASS(pPrfCom->m_pInfo->GetModuleInfo(modID,
                                        &ppBaseLoadAddress,
                                        1024,
                                        &length,
                                        modName,
                                        NULL));

        MpgoProfileData *mpd = new MpgoProfileData;

        TranslateXmlEntities(funcName);
        TranslateXmlEntities(className);
        TranslateXmlEntities(moduleName);

        assert(wcslen(funcName.c_str())<512);
        assert(wcslen(className.c_str())<512);
        assert(wcslen(moduleName.c_str())<512);
        assert(codeInfoCount <= MAX_CODE_INFOS);
        assert(mapInfoCount <= MAX_MAP_INFOS);

        mpd->order = orderCounter++;
        mpd->baseLoadAddress = ppBaseLoadAddress;
        mpd->classID = classID;
        mpd->moduleID = modID;
        mpd->functionID = token;
        wcscpy(mpd->functionName, funcName.c_str());
        wcscpy(mpd->className, className.c_str());
        wcscpy(mpd->moduleName, moduleName.c_str());

        SetCodeInfo(mpd, codeInfoCount, codeInfo);
        SetMapInfo(mpd, mapInfoCount, mapInfo);

        mpgoProfilerDataMap[key] = mpd;
    }

    return S_OK;
}

HRESULT CollectMpgoProfilerData::FunctionEnter3WithInfo(IPrfCom * pPrfCom,
														FunctionIDOrClientID functionIDOrClientID, 
														COR_PRF_ELT_INFO eltInfo)
{

	HRESULT hr = S_OK;
	COR_PRF_FRAME_INFO frameInfo = NULL;
	ULONG pcbArgumentInfo = sizeof(COR_PRF_FUNCTION_ARGUMENT_INFO);
	COR_PRF_FUNCTION_ARGUMENT_INFO * pArgumentInfo = new COR_PRF_FUNCTION_ARGUMENT_INFO;
	hr = pPrfCom->m_pInfo->GetFunctionEnter3Info(functionIDOrClientID.functionID, eltInfo, &frameInfo, &pcbArgumentInfo, pArgumentInfo);
	if (hr != HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) && hr != S_OK)
	{
		delete pArgumentInfo;
		FAILURE("\nFailed inside CollectMpgoProfilerData::FunctionEnter3WithInfo. Did not get expected HRs (ERROR_INSUFFICIENT_BUFFER or S_OK)")
		return hr;
	}
	else if (hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER))
	{
		delete pArgumentInfo;
		pArgumentInfo = reinterpret_cast <COR_PRF_FUNCTION_ARGUMENT_INFO *>  (new BYTE[pcbArgumentInfo]); 

		hr = pPrfCom->m_pInfo->GetFunctionEnter3Info(functionIDOrClientID.functionID, eltInfo, &frameInfo, &pcbArgumentInfo, pArgumentInfo);
		if(hr != S_OK)
		{
			delete (BYTE *)pArgumentInfo;
			printf("\nGetFunctionEnter3Info failed with E_FAIL");
			return E_FAIL;
		}
	}
	delete (BYTE *) pArgumentInfo;
	return FuncEnter2(pPrfCom, functionIDOrClientID.functionID, NULL, frameInfo, pArgumentInfo);
}

void CollectMpgoProfilerData::GetMpgoProfileFileName(wstring& mpgoProfileFileName)
{
    const int bufSize = 256;
    wchar_t buf[bufSize] = {0};
    DWORD moduleFileNameLength = 0;

#ifdef _WIN32
    moduleFileNameLength = GetModuleFileNameW(NULL, buf, bufSize);
#else // _WIN32
    char narrowBuf[bufSize] = {0};

#ifdef OSX
    uint32_t size = bufSize;
    moduleFileNameLength = _NSGetExecutablePath(narrowBuf, &size);
#else
    moduleFileNameLength = readlink("/proc/self/exe", narrowBuf, bufSize);
#endif

    mbstowcs(buf, narrowBuf, bufSize);
#endif // _WIN32

    int i=moduleFileNameLength-1;
    while(i>0 && (buf[i] != L'\\' && buf[i] != '/'))
        i--;

    if(i == 0)
    {
        DISPLAY(L"CollectMpgoProfilerData::GetMpgoProfileFileName failed\n");
        return;
    }

    buf[i+1] = 0;
    wcscat_s(buf, 256, L"MpgoProfilerData.xml");

    mpgoProfileFileName = buf;
}

void CollectMpgoProfilerData::FreeMpgoProfilerDataMemory()
{
    MpgoProfileDataMap :: const_iterator mapIter;

    for(mapIter = mpgoProfilerDataMap.begin(); mapIter != mpgoProfilerDataMap.end(); mapIter++)
    {
        MpgoProfileData* mpd = mapIter->second;

        if(mpd->codeInfo != NULL)
            delete [] mpd->codeInfo;

        if(mpd->mapInfo != NULL)
            delete [] mpd->mapInfo;

        delete mpd;
    }
}

void CollectMpgoProfilerData::WriteMpgoProfilerData()
{
    wstring mpgoProfileFileName;
    const int BUFSIZE = 512;
    char className[BUFSIZE] = {0};
    char functionName[BUFSIZE] = {0};
    char moduleName[BUFSIZE] = {0};
    size_t sizeConverted = 0;
    FILE *fp = NULL;
    MpgoProfileDataMap :: const_iterator mapIter;

    GetMpgoProfileFileName(mpgoProfileFileName);

    string narrowFileName = ToNarrow(mpgoProfileFileName);
    fp = fopen(narrowFileName.c_str(), "w");
    if(fp != NULL)
    {
        fprintf(fp, XML_FORMAT_STRING_HEADER);
        fprintf(fp, XML_FORMAT_STRING_ROOT_START);

        for(mapIter = mpgoProfilerDataMap.begin(); mapIter != mpgoProfilerDataMap.end(); mapIter++)
        {
            MpgoProfileData* mpd = mapIter->second;

            sizeConverted = wcstombs(className, mpd->className, BUFSIZE);
            sizeConverted = wcstombs(functionName, mpd->functionName, BUFSIZE);
            sizeConverted = wcstombs(moduleName, mpd->moduleName, BUFSIZE);

            fprintf(fp, XML_FORMAT_STRING_METHOD_START, 
                mpd->functionID, 
                mpd->classID, 
                mpd->moduleID, 
                className, 
                functionName,
                moduleName, 
                mpd->order, 
                mpd->baseLoadAddress);

            fprintf(fp, XML_FORMAT_STRING_BLOCKINFO_START);

            for(ULONG32 i=0; i<mpd->codeInfoLength; i++)
            {
                fprintf(fp, XML_FORMAT_STRING_BLOCKINFO, 
                    (void *)mpd->codeInfo[i].startAddress,
                    mpd->codeInfo[i].size);
            }

            fprintf(fp, XML_FORMAT_STRING_BLOCKINFO_END);
            fprintf(fp, XML_FORMAT_STRING_MAPINFO_START);

            for(ULONG32 i=0; i<mpd->mapInfoLength; i++)
            {
                fprintf(fp, XML_FORMAT_STRING_MAPINFO, 
                    mpd->mapInfo[i].ilOffset,
                    mpd->mapInfo[i].nativeStartOffset,
                    mpd->mapInfo[i].nativeEndOffset);
            }
            fprintf(fp, XML_FORMAT_STRING_MAPINFO_END);
            fprintf(fp, XML_FORMAT_STRING_METHOD_END);
        }

        fprintf(fp, XML_FORMAT_STRING_ROOT_END);
        fclose(fp);
        DISPLAY(L"'" << mpgoProfileFileName.c_str() << L"' written successfully\n");
    }
    else
    {
        DISPLAY(L"CollectMpgoProfilerData::WriteMpgoProfilerData failed: errno: " << errno << L"\n");
    }
}

HRESULT CollectMpgoProfilerData::FuncTailcall2(IPrfCom *pPrfCom,
                                               FunctionID /*mappedFuncId*/,
                                               UINT_PTR /*clientData*/,
                                               COR_PRF_FRAME_INFO /*func*/)
{
    return S_OK;
}


HRESULT CollectMpgoProfilerData::FuncLeave2(IPrfCom *pPrfCom,
                                            FunctionID funcId,
                                            UINT_PTR clientData,
                                            COR_PRF_FRAME_INFO func,
                                            COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange)
{
    return S_OK;
}

HRESULT CollectMpgoProfilerData::FunctionLeave3WithInfo(IPrfCom *pPrfCom,
                                             FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo)
{
    return S_OK;
}

HRESULT CollectMpgoProfilerData::FunctionTailCall3WithInfo(IPrfCom *pPrfCom,
                                             FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo)
{
    return S_OK;
}

HRESULT CollectMpgoProfilerData::Verify(IPrfCom *pPrfCom)
{

    // TODO: comment about CS
    WriteMpgoProfilerData();

    return S_OK;
}

CollectMpgoProfilerData* global_CollectMpgoProfilerData;

HRESULT CollectMpgoProfilerData_Verify(IPrfCom * pPrfCom)
{
    HRESULT hr = CollectMpgoProfilerData::Instance()->Verify(pPrfCom);

    delete global_CollectMpgoProfilerData;

    return hr;
}

void CollectMpgoProfilerData_Initialize (IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, bool useELT3Callbacks)
{
    g_pPrfCom = pPrfCom;

	DISPLAY(L"Profiler API Test: CollectMpgoProfilerData API\n");

	global_CollectMpgoProfilerData = new CollectMpgoProfilerData(pPrfCom);

	pModuleMethodTable->FLAGS =
		COR_PRF_MONITOR_ENTERLEAVE |
		COR_PRF_ENABLE_FRAME_INFO | 
		COR_PRF_ENABLE_JIT_MAPS;

	pModuleMethodTable->VERIFY = (FC_VERIFY) &CollectMpgoProfilerData_Verify;
	if(!useELT3Callbacks)
	{
		pModuleMethodTable->FUNCTIONENTER2 = (FC_FUNCTIONENTER2) &CollectMpgoProfilerData::FuncEnter2Wrapper;
		pModuleMethodTable->FUNCTIONLEAVE2 = (FC_FUNCTIONLEAVE2) &CollectMpgoProfilerData::FuncLeave2Wrapper;
		pModuleMethodTable->FUNCTIONTAILCALL2 = (FC_FUNCTIONTAILCALL2) &CollectMpgoProfilerData::FuncTailcall2Wrapper;
	}
	else
	{
		pModuleMethodTable->FUNCTIONENTER3WITHINFO = (FC_FUNCTIONENTER3WITHINFO) &CollectMpgoProfilerData::FunctionEnter3WithInfoWrapper;
		pModuleMethodTable->FUNCTIONLEAVE3WITHINFO = (FC_FUNCTIONLEAVE3WITHINFO) &CollectMpgoProfilerData::FunctionLeave3WithInfoWrapper;
		pModuleMethodTable->FUNCTIONTAILCALL3WITHINFO = (FC_FUNCTIONTAILCALL3WITHINFO) &CollectMpgoProfilerData::FunctionTailCall3WithInfoWrapper;
	}
	return;
}


#pragma warning(pop)
