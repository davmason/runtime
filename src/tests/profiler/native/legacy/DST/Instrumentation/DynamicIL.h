#include "stdafx.cpp"
#include <map>
#include <wchar.h>

#undef DISPLAY
#undef LOG
#undef FAILURE
#undef VERBOSE

#pragma warning(push)
#pragma warning( disable : 4996)

#define DISPLAY( message )  {                                                                       \
                                std::wstringstream temp;                                            \
                                temp << message;                                                    \
                                m_pPrfCom->Display(temp);                                           \
                            }

#define FAILURE( message )  {                                                                       \
                                std::wstringstream temp;                                            \
                                temp << message;                                                    \
                                m_pPrfCom->Error(temp) ;                                            \
                            }


#ifndef WINDOWS
int _wcsicmp(const PLATFORM_WCHAR * wstr1, const PLATFORM_WCHAR * wstr2)
{
    int i = 0;
    while (wstr1[i] && wstr2[i])
    {
        if (towlower(wstr1[i]) > towlower(wstr2[i]))
            return 1;
        else if (towlower(wstr2[i]) < towlower(wstr2[i]))
            return -1;
        ++i;
    }
    if (wstr1[i] == wstr2[i]) 
        return 0;
    else if (wstr1[i] < wstr2[i]) 
        return -1;
    else 
        return 1;
}
#endif

using namespace std;

extern PMODULEMETHODTABLE pCallTable;
extern IPrfCom * g_satellite_pPrfCom;

class DynamicIL;
static DynamicIL *g_pDynamicIL;

typedef std::map<FunctionID, CMethodInfo *> FuncMethodMap, *PFuncMethodMap;
typedef pair<const FunctionID, CMethodInfo*> FuncMethodPair;
typedef std::map<FunctionID, CMethodInfo*>::iterator FuncMethodMapIterator;

typedef std::map<ModuleID, CModuleInfo*> ModuleInfoMap, *PModuleInfoMap;
typedef pair<const ModuleID, CModuleInfo*> ModuleInfoPair;
typedef std::map<ModuleID, CModuleInfo*>::iterator ModuleInfoMapIterator;


enum Prolog
{
    PROLOG_EMPTY,
    PROLOG_PUSHPOP,
    PROLOG_PINVOKE,
    PROLOG_CALLI,
    PROLOG_MANAGEDWRAPPERDLL,
    PROLOG_DYNAMICCLASS
};

typedef enum
{
    DYNAMICIL_RESOURCEMODULE = 0xFFFF1000
} DYNAMICIL_FAILURE;

class DynamicIL
{

public:
    IPrfCom* m_pPrfCom;
    Prolog m_eProlog;
    BOOL m_bVerifyAllocator;
    BOOL m_bVerifyCacheToJitCount;
    BOOL m_bVerifyJitToCallbackCount;

    //Variables read from environment
    ULONG m_ulAllocationSize;
    ULONG m_ulInstrumented;
    ULONG m_ulMissHit;

public:
    DynamicIL(IPrfCom* pPrfCom)
    {
        _ASSERTE(g_pDynamicIL == NULL);
        m_pPrfCom = pPrfCom;

        m_bVerifyAllocator = FALSE;
        m_bVerifyCacheToJitCount = FALSE;
        m_bVerifyJitToCallbackCount = FALSE;
        m_ulAllocationSize = 0;

        m_tkPInvokeMethodDef = mdTokenNil;
        m_tkIProbesPInvokeMethodDef = mdTokenNil;
        m_tkDynamicILModuleRef = mdTokenNil;
        m_tkMscorlib = mdTokenNil;
        m_tkSystemObject = mdTokenNil;
        m_tkSystemObject = mdTokenNil;
        m_tkCriticalFinalizerObject = mdTokenNil;
        m_tkDynamicILGenericClass = mdTokenNil;
        m_tkDynamicILGenericMethod = mdTokenNil;
        m_tkDynamicILGenericClassParam = mdTokenNil;

        m_phProbeEnterMethods = new FuncMethodMap();
        m_phCacheLookupMethods = new FuncMethodMap();
        m_pModuleInfos = new ModuleInfoMap();

        m_uDynILManagedModuleID = 0;
        m_uMscorlibModuleID = 0;
        m_ulInstrumented = 0;
        m_ulMissHit = 0;

        m_wszUnmangedProfilerDllNameNoExt[0] = L'\0';
        m_wszUnmanagedCallbackFunctionName[0] = L'\0';
        m_wszManagedProfilerDllNameNoExt[0] = L'\0';
        m_wszManagedProfilerClassName[0] = L'\0';
        m_wszManagedProfilerMethodEnterName[0] = L'\0';

        wcscpy(m_wszUnmangedProfilerDllNameNoExt, L"TestProfiler");
        wcscpy(m_wszUnmanagedCallbackFunctionName, L"InstrumentationMethodEnter");
        wcscpy(m_wszManagedProfilerDllNameNoExt, L"InstrumentationManaged");
        wcscpy(m_wszManagedProfilerClassName, L"Instrumentation.Managed.StaticTargets");
        wcscpy(m_wszManagedProfilerMethodEnterName, L"MethodEnter");

        lock_guard<mutex> guard(m_csHashLock);

    }
    ~DynamicIL()
    {
        if (g_pDynamicIL)
        {
            g_pDynamicIL = NULL;
        }
    }

    HRESULT DynamicIL::GetMetaData(ModuleID moduleID, DWORD dwOpenFlags, const IID &rid, IUnknown** ppOut)
    {
        IUnknown* pOut;
        HRESULT hr = m_pPrfCom->m_pInfo->GetModuleMetaData(moduleID,
            dwOpenFlags,
            rid,
            &pOut);
        if (hr == S_FALSE && pOut == NULL)
        {
            //This means the moduleID is a resource
            //This is a really bad design
            //  <TODO> Should actually verify this a resource module
            return DYNAMICIL_RESOURCEMODULE;
        }
        if (FAILED(hr))
        {
            DISPLAY(L"DynamicIL Failure:\n");
            DISPLAY(L"\tModuleID:" << moduleID);
            DISPLAY(L"\n\tOpenFlags:" << dwOpenFlags);
            DISPLAY(L"\n\tIID:" << rid.Data4);
            FAILURE(L"DynamicIL::GetMetaData FAILED: " << HEX(hr));
        }
        _ASSERTE(ppOut);
        *ppOut = pOut;
        return S_OK;
    }

    void ProbeCallback(UINT_PTR moduleID, UINT_PTR functionID)
    {
        FuncMethodMapIterator iter;
        lock_guard<mutex> guard(m_csHashLock);

        if ((iter = m_phProbeEnterMethods->find(functionID)) != m_phProbeEnterMethods->end())
        {
            //DISPLAY(L"Found %x :: ",iter->first);
            //DISPLAY(L"Probe Hit - " << iter->first <<" "<<iter->second->FullSignature());
            size_t removed = m_phProbeEnterMethods->erase(functionID);
        }

    }


    ///Callback prototypes
    static HRESULT  VerifyWrapper(IPrfCom *pPrfCom)
    {
        return g_pDynamicIL->Verify();
    }

    static HRESULT  ModuleLoadStartedWrapper(IPrfCom *pPrfCom,
        ModuleID moduleId)
    {
        return g_pDynamicIL->ModuleLoadStarted(moduleId);
    }

    static HRESULT  ModuleLoadFinishedWrapper(IPrfCom *pPrfCom,
        ModuleID moduleId,
        HRESULT  hrStatus)
    {
        return g_pDynamicIL->ModuleLoadFinished(moduleId);
    }

    static HRESULT  ModuleUnLoadStartedWrapper(IPrfCom *pPrfCom,
        ModuleID moduleId)
    {
        return g_pDynamicIL->ModuleUnLoadStarted(moduleId);
    }

    static HRESULT  ModuleUnLoadFinishedWrapper(IPrfCom *pPrfCom,
        ModuleID moduleId,
        HRESULT  hrStatus)
    {
        return g_pDynamicIL->ModuleUnLoadFinished(moduleId);
    }

    static HRESULT  JitCompilationFinishedWrapper(IPrfCom *pPrfCom,
        FunctionID functionId,
        HRESULT hrStatus,
        BOOL fIsSafeToBlock)
    {
        return g_pDynamicIL->JitCompilationFinished(functionId, hrStatus, fIsSafeToBlock);
    }

    static HRESULT  JitCompilationStartedWrapper(IPrfCom *pPrfCom,
        FunctionID functionId,
        BOOL fIsSafeToBlock)
    {
        return g_pDynamicIL->JitCompilationStarted(functionId, fIsSafeToBlock);
    }

    static HRESULT  JitCachedFunctionSearchStartedWrapper(IPrfCom *pPrfCom,
        FunctionID functionId,
        BOOL *pbUseCachedFunction)
    {
        return g_pDynamicIL->JitCachedFunctionSearchStarted(functionId, pbUseCachedFunction);
    }

    static HRESULT  JitCachedFunctionSearchFinishedWrapper(IPrfCom *pPrfCom,
        FunctionID functionId,
        COR_PRF_JIT_CACHE result)
    {
        return g_pDynamicIL->JitCachedFunctionSearchFinished(functionId, result);
    }

    static HRESULT  JitInliningWrapper(IPrfCom *pPrfCom,
        FunctionID callerID,
        FunctionID calleeID,
        BOOL* pShouldInline)
    {
        return g_pDynamicIL->JitInlining(callerID, calleeID, pShouldInline);
    }

    static HRESULT  ClassLoadStartedWrapper(IPrfCom *pPrfCom,
        ClassID classId)
    {
        return g_pDynamicIL->ClassLoadStarted(classId);
    }

    HRESULT DynamicIL::Verify()
    {
        if (m_bVerifyJitToCallbackCount && !m_phProbeEnterMethods->empty())
        {
            FuncMethodMapIterator iter;
            DISPLAY(L"Instrumented functions failed to callback\n");
            for (iter = m_phProbeEnterMethods->begin(); iter != m_phProbeEnterMethods->end(); iter++)
            {
                DISPLAY(iter->first << L" " << iter->second->FullSignature());
                ++m_ulMissHit;
            }
        }
        if (m_bVerifyCacheToJitCount && !m_phCacheLookupMethods->empty())
        {
            FuncMethodMapIterator iter;
            DISPLAY(L"CacheLookup Functions failed to send JitCompilation\n");
            for (iter = m_phProbeEnterMethods->begin(); iter != m_phProbeEnterMethods->end(); iter++)
            {
                DISPLAY(iter->first << L" " << iter->second->FullSignature());
                ++m_ulMissHit;
            }

        }

        if (m_ulInstrumented == 0)
        {
            FAILURE(L"NO function is instrumented.\n");
            return E_FAIL;
        }

        if (m_ulMissHit * 10 > m_ulInstrumented * 9)
        {
            FAILURE(L"Less than 90% instrumented functions are hit.\n" << m_ulInstrumented << L" instrumented  " << m_ulMissHit << "mishit");

            return E_FAIL;
        }

        DISPLAY(m_ulInstrumented << L" functions instrumented.");
        return S_OK;
    }
    HRESULT ModuleLoadStarted(ModuleID moduleId)
    {
        DISPLAY(L"ModuleLoadStarted\n");
        VerifyAllocator(moduleId, TRUE);
        return S_OK;
    }
    HRESULT ModuleLoadFinished(ModuleID moduleId);

    HRESULT ModuleUnLoadStarted(ModuleID moduleId)
    {
        DISPLAY(L"ModuleUnLoadStarted\n");
        //From CLR's aspect, Module is not started yet ...
        VerifyAllocator(moduleId, TRUE);
        return S_OK;
    }
    HRESULT ModuleUnLoadFinished(ModuleID moduleId)
    {
        DISPLAY(L"ModuleUnLoadFinished\n");
        VerifyAllocator(moduleId, FALSE);
        return S_OK;
    }

    HRESULT JitCompilationStarted(FunctionID functionId, BOOL fIsSafeToBlock);
    HRESULT JitCompilationFinished(FunctionID functionID, HRESULT hrStatus, BOOL fIsSafeToBlock)
    {
        CMethodInfo* pMethodInfo = new CMethodInfo();
        CreateMethodInfoFromFunctionID(functionID, pMethodInfo);
        _ASSERTE(pMethodInfo);
        if (FAILED(hrStatus))
        {
            FAILURE(L"Failed HRESULT in JitCompilationFinished " << HEX(hrStatus) << L" " << pMethodInfo->FullSignature());
        }
        VerifyAllocator(pMethodInfo->m_moduleID, TRUE);
        return S_OK;
    }

    HRESULT JitCachedFunctionSearchStarted(FunctionID functionId, BOOL *pbUseCachedFunction)
    {
        /*
        WCHAR buf[1024];
        pPrfCom->GetFunctionNameFromFunctionID(functionId,buf,1024);
        DISPLAY((L"JitCachedFunctionSearchStarted: %s\n",buf));
        */
        CMethodInfo *pMethodInfo = new CMethodInfo();
        CreateMethodInfoFromFunctionID(functionId, pMethodInfo);
        VerifyAllocator(pMethodInfo->m_moduleID, TRUE);
        if (m_bVerifyCacheToJitCount)
        {
            lock_guard<mutex> guard(m_csHashLock);
            m_phCacheLookupMethods->insert(pair<FunctionID, CMethodInfo*>(functionId, pMethodInfo));
        }
        *pbUseCachedFunction = TRUE;
        return S_OK;
    }

    HRESULT JitCachedFunctionSearchFinished(FunctionID functionId, COR_PRF_JIT_CACHE result)
    {
        CMethodInfo *pMethodInfo = new CMethodInfo();
        CreateMethodInfoFromFunctionID(functionId, pMethodInfo);
        VerifyAllocator(pMethodInfo->m_moduleID, TRUE);

        //<TODO> Perhaps some verification between SearchStarted and SearchFinished
        //

        return S_OK;
    }

    HRESULT JitInlining(FunctionID callerID, FunctionID calleeID, BOOL* pShouldInline)
    {
        CMethodInfo *pCallerMethodInfo = new CMethodInfo();
        CMethodInfo *pCalleeMethodInfo = new CMethodInfo();

        CreateMethodInfoFromFunctionID(callerID, pCallerMethodInfo);
        CreateMethodInfoFromFunctionID(calleeID, pCalleeMethodInfo);

        VerifyAllocator(pCallerMethodInfo->m_moduleID, TRUE);
        VerifyAllocator(pCalleeMethodInfo->m_moduleID, TRUE);

        //<TODO> Need lots more work to verify Inlining is on or off
        //   Probably should use FunctionEnter / Leave to verify
        //
        *pShouldInline = FALSE;
        return S_OK;
    }

    HRESULT ClassLoadStarted(ClassID classId)
    {
        WCHAR_STR(buf);
        m_pPrfCom->GetClassIDName(classId, buf);
        //DISPLAY(L"ClassLoadStarted: " << buf);
        return S_OK;
    }
private:

    inline BOOL IsMscorlibDLL(ModuleID moduleID)
    {
        return (m_uMscorlibModuleID == moduleID);
    }

    inline BOOL IsManagedWrapperDLL(ModuleID moduleID)
    {
        return (m_uDynILManagedModuleID == moduleID);
    }

    void DynamicIL::VerifyAllocator(ModuleID moduleID, BOOL expectSuccess);


    HRESULT GetModuleNameFromModuleID(ModuleID moduleID, PLATFORM_WCHAR *buff, INT buffLength);
    HRESULT CreateMethodInfoFromFunctionID(FunctionID functionID, CMethodInfo *pmethodInfo);
    PCCOR_SIGNATURE DynamicIL::ParseSignature(IMetaDataImport* pMetaDataImport, PCCOR_SIGNATURE pCorSignature, PLATFORM_WCHAR* wszBuffer);

    void DynamicIL::DumpMethod(PBYTE method, int methodSize, int offSet);
    void DynamicIL::ShowMethod(CMethodInfo *pMethodInfo);
    void DynamicIL::DumpBytes(LPCBYTE pBuffer, const ULONG ulBufferSize);
    BOOL DynamicIL::CreateProlog(CMethodInfo *pMethodInfo);
    void DynamicIL::EmitIL(BYTE* pBuffer, ULONG* pCurrentOffset, OpcodeInfo opcodeInfo);
    void DynamicIL::EmitILToken(BYTE* pBuffer, ULONG* pCurrentOffset, ULONG32 token);
    void DynamicIL::EmitILPTR(BYTE* pBuffer, ULONG* pCurrentOffset, UINT_PTR token, int tokenSize);
    void DynamicIL::CreateNewFunctionBodyTiny(CMethodInfo *pMethodInfo);
    void DynamicIL::CreateNewFunctionBodyFat(CMethodInfo *pMethodInfo);
    void DynamicIL::CreatePInvokeMethod(CModuleInfo* pModuleInfo);
    void DynamicIL::CreateManagedWrapperAssemblyRef(CModuleInfo* pModuleInfo);
    void DynamicIL::CreateClassInMscorlib(CModuleInfo* pModuleInfo);
    void DynamicIL::CreateDynamicClass(CModuleInfo* pModuleInfo);
    void DynamicIL::FixSEHSections(CMethodInfo *pMethodInfo);
    void DynamicIL::SEHWalker(CMethodInfo *pMethodInfo);
    void DynamicIL::AddSuppressUnmanagedCodeSecurityAttributeToMethod(IMetaDataImport2* pMetaDataImport, IMetaDataEmit2* pMetaDataEmit, mdToken tkOwner);
    BOOL DynamicIL::ShouldInstrumentMethod(CMethodInfo* pMethodInfo);
    void DynamicIL::CheckIfMethodIsPartOfCER(IMetaDataImport2* pMetaDataImport, CMethodInfo* pMethodInfo);
    BOOL DynamicIL::CheckIfMethodExtendsCriticalObject(IMetaDataImport2* pMetaDataImport, mdToken tkTypeDef);
    mdAssemblyRef DynamicIL::GetMscorlibAssemblyRef(CModuleInfo* pModuleInfo);

private:

    FuncMethodMap *m_phProbeEnterMethods;
    FuncMethodMap *m_phCacheLookupMethods;
    ModuleInfoMap *m_pModuleInfos;
    mdMethodDef m_tkPInvokeMethodDef;
    mdMethodDef m_tkIProbesPInvokeMethodDef;
    mdModuleRef m_tkDynamicILModuleRef;
    mdAssemblyRef m_tkMscorlib;
    mdTypeRef m_tkSuppressSecurity;
    mdTypeDef m_tkSystemObject;
    mdTypeDef m_tkCriticalFinalizerObject;
    mdTypeDef m_tkDynamicILGenericClass;
    mdTypeDef m_tkDynamicILGenericMethod;
    mdGenericParam m_tkDynamicILGenericClassParam;
    mdMemberRef m_tkDynamicILManagedFunctionEnter;

    ModuleID m_uDynILManagedModuleID;
    ModuleID m_uMscorlibModuleID;

    PLATFORM_WCHAR m_wszUnmangedProfilerDllNameNoExt[MAX_PATH];
    PLATFORM_WCHAR m_wszUnmanagedCallbackFunctionName[MAX_PATH];
    PLATFORM_WCHAR m_wszManagedProfilerDllNameNoExt[MAX_PATH];
    PLATFORM_WCHAR m_wszManagedProfilerClassName[MAX_PATH];
    PLATFORM_WCHAR m_wszManagedProfilerMethodEnterName[MAX_PATH];

    mutex m_csHashLock;

};

extern "C"
{
    void __stdcall InstrumentationMethodEnter(UINT_PTR moduleID, UINT_PTR functionID)
    {
        g_pDynamicIL->ProbeCallback(moduleID, functionID);
    }
};

extern "C" void Instrumentation_Satellite_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& TestName)
{
    if (g_pDynamicIL == NULL)
    {
        g_pDynamicIL = new DynamicIL(pPrfCom);
    }
    _ASSERTE(g_pDynamicIL);

    IPrfCom *m_pPrfCom = pPrfCom;
    DISPLAY(L"DynamicIL Initialize: " << TestName);

    //Defaults
    g_pDynamicIL->m_eProlog = PROLOG_EMPTY;
    g_pDynamicIL->m_bVerifyAllocator = FALSE;
    g_pDynamicIL->m_bVerifyCacheToJitCount = FALSE;
    g_pDynamicIL->m_bVerifyJitToCallbackCount = FALSE;

    if (TestName.find(L"PushPop") != string::npos)
    {
        g_pDynamicIL->m_eProlog = PROLOG_PUSHPOP;
    }
    else if (TestName.find(L"PInvoke") != string::npos)
    {
        g_pDynamicIL->m_eProlog = PROLOG_PINVOKE;
        g_pDynamicIL->m_bVerifyJitToCallbackCount = TRUE;
    }
    else if (TestName.find(L"Calli") != string::npos)
    {
        g_pDynamicIL->m_eProlog = PROLOG_CALLI;
        g_pDynamicIL->m_bVerifyJitToCallbackCount = TRUE;
    }
    else if (TestName.find(L"ManagedWrapperDll") != string::npos)
    {
        g_pDynamicIL->m_eProlog = PROLOG_MANAGEDWRAPPERDLL;
        g_pDynamicIL->m_bVerifyJitToCallbackCount = TRUE;
    }
    else if (TestName.find(L"InstrumentAll") != string::npos)
    {
        pModuleMethodTable->FLAGS |= COR_PRF_USE_PROFILE_IMAGES;
        g_pDynamicIL->m_eProlog = PROLOG_MANAGEDWRAPPERDLL;
        g_pDynamicIL->m_bVerifyJitToCallbackCount = TRUE;
    }
    else if (TestName.find(L"VerifyAllocator") != string::npos)
    {
        char *envVar = NULL;
        char *stopString = NULL;
        g_pDynamicIL->m_bVerifyAllocator = TRUE;
        wstring env = ReadEnvironmentVariable(L"INSTRUMENTATION_ALLOCATIONSIZE");
        if (env.size() > 0)
        {
            g_pDynamicIL->m_ulAllocationSize = wcstol(env.c_str(), NULL, 10);  //strtoul(envVar, &stopString, 0);
        }
        else
        {
            //Environment variable not set so use the default allocation size
            g_pDynamicIL->m_ulAllocationSize = 255;  //0xFF
        }
        DISPLAY(L"AllocationSize: " << g_pDynamicIL->m_ulAllocationSize);

    }
    else
    {
        FAILURE(L"Unknown TestName: " << TestName);
    }

    //<TODO> Add various configurations to DynamicIL
    //   1) Type of instrumentation (i.e. Managed/Native)
    //   2) Wrapping of functions in EH tables
    //   3) Inserting hooks before MethodCalls with the Method Name
    //   4) Dynamic Generic IL
    //
    //<TODO> Make a call to a managed DLL which in turn calls the profiler
    //

    pModuleMethodTable->VERIFY = (FC_VERIFY)&DynamicIL::VerifyWrapper;
    pModuleMethodTable->MODULELOADSTARTED = (FC_MODULELOADSTARTED)&DynamicIL::ModuleLoadStartedWrapper;
    pModuleMethodTable->MODULELOADFINISHED = (FC_MODULELOADFINISHED)&DynamicIL::ModuleLoadFinishedWrapper;
    pModuleMethodTable->MODULEUNLOADSTARTED = (FC_MODULEUNLOADSTARTED)&DynamicIL::ModuleUnLoadStartedWrapper;
    pModuleMethodTable->MODULEUNLOADFINISHED = (FC_MODULEUNLOADFINISHED)&DynamicIL::ModuleUnLoadFinishedWrapper;

    pModuleMethodTable->JITCOMPILATIONSTARTED = (FC_JITCOMPILATIONSTARTED)&DynamicIL::JitCompilationStartedWrapper;
    pModuleMethodTable->JITCOMPILATIONFINISHED = (FC_JITCOMPILATIONFINISHED)&DynamicIL::JitCompilationFinishedWrapper;
    pModuleMethodTable->JITCACHEDFUNCTIONSEARCHSTARTED = (FC_JITCACHEDFUNCTIONSEARCHSTARTED)&DynamicIL::JitCachedFunctionSearchStartedWrapper;
    pModuleMethodTable->JITCACHEDFUNCTIONSEARCHFINISHED = (FC_JITCACHEDFUNCTIONSEARCHFINISHED)&DynamicIL::JitCachedFunctionSearchFinishedWrapper;

    pModuleMethodTable->JITINLINING = (FC_JITINLINING)&DynamicIL::JitInliningWrapper;

    pModuleMethodTable->CLASSLOADSTARTED = (FC_CLASSLOADSTARTED)&DynamicIL::ClassLoadStartedWrapper;

    pModuleMethodTable->FLAGS |= COR_PRF_DISABLE_TRANSPARENCY_CHECKS_UNDER_FULL_TRUST |
        COR_PRF_USE_PROFILE_IMAGES |
        COR_PRF_MONITOR_ASSEMBLY_LOADS |
        COR_PRF_MONITOR_CACHE_SEARCHES |
        COR_PRF_MONITOR_CLASS_LOADS |
        COR_PRF_MONITOR_JIT_COMPILATION |
        COR_PRF_MONITOR_MODULE_LOADS;

    pCallTable = pModuleMethodTable;
    g_satellite_pPrfCom = pPrfCom;

    return;

}

#pragma warning(pop)
