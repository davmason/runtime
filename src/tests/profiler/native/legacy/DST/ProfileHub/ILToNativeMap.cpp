#include "../../ProfilerCommon.h"

#pragma warning(push)
#pragma warning( disable : 4996)

#include <cstdio>

#define dimensionof(a) 		(sizeof(a)/sizeof(*(a)))

// ILToNativeMap test isn't so much a profiler test as a component of the ETW
// JIT sequence map test.  This profiler module calls the profiling API GetILToNativeMapping
// and spews the output to a file.  The ETW JIT sequence map test then compares the ETW
// sequence map events with this profiler-generated file to validate that the ETW events
// are cool.
// 
// See perf\etw\ILToNativeMap.cs for more information

class ILToNativeMap
{
public:
    FILE * m_fpMapXml;
    ModuleID m_moduleIDToTest;
    PLATFORM_WCHAR m_wszModuleToJitPath[MAX_PATH];
    PLATFORM_WCHAR *m_pwszModuleToJitName;

    ILToNativeMap() :
        m_fpMapXml(NULL),
        m_moduleIDToTest(NULL),
        m_pwszModuleToJitName(NULL)
    {
        memset(m_wszModuleToJitPath, 0, sizeof(m_wszModuleToJitPath));
    }

};

HRESULT ILToNativeMap_JITCompilationFinished(IPrfCom * pPrfCom, FunctionID functionID, HRESULT hrStatus, BOOL fIsSafeToBlock)
{
    HRESULT hr;
    mdToken methodDef;
    ClassID classID;
    ModuleID moduleID;
    LOCAL_CLASS_POINTER(ILToNativeMap);

    hr = PINFO->GetFunctionInfo(
        functionID,
        &classID,
        &moduleID,
        &methodDef);
    if (FAILED(hr))
    {
        FAILURE(L"GetFunctionInfo for FunctionID '" << HEX(functionID) << L"' failed.  HRESULT = " << HEX(hr) << L".");
        return S_OK;
    }

    if (moduleID != pILToNativeMap->m_moduleIDToTest)
    {
        return S_OK;
    }

    COR_DEBUG_IL_TO_NATIVE_MAP map[7000];
    ULONG32 cMapReturned;

    hr = PINFO->GetILToNativeMapping(
        functionID,
        dimensionof(map),
        &cMapReturned,
        map);
    if (FAILED(hr))
    {
        FAILURE(L"GetILToNativeMapping for FunctionID '" << HEX(functionID) << L"' (methodDef '" 
            << HEX(methodDef) << L"') failed.  HRESULT = " << HEX(hr) << L".");
        return S_OK;
    }

    {
        lock_guard<recursive_mutex> guard(pPrfCom->m_criticalSection);

        fwprintf(
            pILToNativeMap->m_fpMapXml,
            L"    <method token=\"0x%x\" id=\"0x%p\">\n",
            methodDef,
            (void *)functionID);
        fwprintf(
            pILToNativeMap->m_fpMapXml,
            L"        <sequencePoints>\n");
        
        for (ULONG32 i=0; i < cMapReturned; i++)
        {
            fwprintf(
                pILToNativeMap->m_fpMapXml,
                L"            <seqPoint ilOffset=\"%d\" nativeOffset=\"%d\"/>\n",
                map[i].ilOffset,
                map[i].nativeStartOffset);
        }

        fwprintf(
            pILToNativeMap->m_fpMapXml,
            L"        </sequencePoints>\n");
        fwprintf(
            pILToNativeMap->m_fpMapXml,
            L"    </method> <!-- token 0x%x -->\n",
            methodDef);


        fflush(pILToNativeMap->m_fpMapXml);
    }


    return S_OK;
}

HRESULT ILToNativeMap_ModuleLoadFinished(IPrfCom * pPrfCom, ModuleID moduleID, HRESULT hrStatus)
{
    HRESULT hr;

    LOCAL_CLASS_POINTER(ILToNativeMap);

    if (pILToNativeMap->m_moduleIDToTest != 0)
    {
        // We've already written to the XML file, so skip this module
        return S_OK;
    }


    LPCBYTE pbBaseLoadAddr;
    PLATFORM_WCHAR wszName[STRING_LENGTH];
    PROFILER_WCHAR wszNameTemp[STRING_LENGTH];
    ULONG cchNameIn = STRING_LENGTH;
    ULONG cchNameOut;
    AssemblyID assemblyID;
    DWORD dwModuleFlags;
    hr = PINFO->GetModuleInfo2(
        moduleID,
        &pbBaseLoadAddr,
        cchNameIn,
        &cchNameOut,
        wszNameTemp,
        &assemblyID,
        &dwModuleFlags);
    if (FAILED(hr))
    {
        FAILURE(L"GetModuleInfo2 failed for ModuleID '" << HEX(moduleID) << L"', hr = " << HEX(hr));
        return S_OK;
    }

    ConvertProfilerWCharToPlatformWChar(wszName, STRING_LENGTH, wszNameTemp, STRING_LENGTH);

    // Is this the module we're supposed to be tracking IL-to-native mapping for?
    if (!ContainsAtEnd(wszName, L"system.ni.dll"))
        return S_OK;

    pILToNativeMap->m_moduleIDToTest = moduleID;

    // WCHAR wszMapXmlPath[MAX_PATH];
    // DWORD dwRet = GetEnvironmentVariable(L"BPD_MAPXMLPATH", wszMapXmlPath, dimensionof(wszMapXmlPath));
    wstring wszMapXmlPath = ReadEnvironmentVariable(L"BPD_MAPXMLPATH");
    if (wszMapXmlPath.size() == 0)
    {
        FAILURE(L"Failed to read value of environment variable BPD_MAPXMLPATH.");
        return S_OK;
    }
    
    string narrowPath = ToNarrow(wszMapXmlPath);
    pILToNativeMap->m_fpMapXml = fopen(narrowPath.c_str(), "wt");
    if (pILToNativeMap->m_fpMapXml == NULL)
    {
        FAILURE(L"Unable to create file '" << wszMapXmlPath << L"'.  Err=" << errno);
        return S_OK;
    }    
    
    fwprintf(
        pILToNativeMap->m_fpMapXml,
        L"<SymbolData assembly=\"%s\">\n",
        wszName);
    fflush(pILToNativeMap->m_fpMapXml);

    return S_OK;
}

HRESULT ILToNativeMap_ModuleUnloadStarted(IPrfCom * pPrfCom, ModuleID moduleID)
{
    return S_OK;
}

HRESULT ILToNativeMap_Verify(IPrfCom * pPrfCom)
{
    // Validation of this test is done by the app itself, not the profiler.  The app
    // determines whether the ETL events match the profiler-logged il-to-native-maps,
    // and fails appropriately if they don't, or if something happened that prevented
    // sufficient validation from even occurring.	
    LOCAL_CLASS_POINTER(ILToNativeMap);

    fwprintf(
        pILToNativeMap->m_fpMapXml,
        L"</SymbolData>");
    fflush(pILToNativeMap->m_fpMapXml);
    fclose(pILToNativeMap->m_fpMapXml);
	
    return S_OK;
}

void ILToNativeMap_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable)
{
    ILToNativeMap * pILToNativeMap = new ILToNativeMap;
    SET_CLASS_POINTER(pILToNativeMap);

    pModuleMethodTable->FLAGS = 
            COR_PRF_MONITOR_JIT_COMPILATION   |
            COR_PRF_MONITOR_MODULE_LOADS      |
            COR_PRF_DISABLE_ALL_NGEN_IMAGES
            ;

    REGISTER_CALLBACK(JITCOMPILATIONFINISHED,       ILToNativeMap_JITCompilationFinished);
    REGISTER_CALLBACK(MODULELOADFINISHED,           ILToNativeMap_ModuleLoadFinished);
    REGISTER_CALLBACK(MODULEUNLOADSTARTED,          ILToNativeMap_ModuleUnloadStarted);
    REGISTER_CALLBACK(VERIFY,                       ILToNativeMap_Verify);

    wstring path = ReadEnvironmentVariable(L"BPD_MODULETOJIT");
    if (path.size() == 0)
    {
        FAILURE(L"Failed to read value of environment variable BPD_MODULETOJIT.");
        return;
    }

    wcscpy(pILToNativeMap->m_wszModuleToJitPath, path.c_str());

    pILToNativeMap->m_pwszModuleToJitName = wcsrchr(pILToNativeMap->m_wszModuleToJitPath, '\\');
    if (pILToNativeMap->m_pwszModuleToJitName == NULL)
    {
        pILToNativeMap->m_pwszModuleToJitName = &pILToNativeMap->m_wszModuleToJitPath[0];
    }
    else
    {
        pILToNativeMap->m_pwszModuleToJitName++;
    }
}
#pragma warning(pop)
