// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// CallbackImpl.h
//
// Declares the class that implements ICorProfilerCallback* (the real meat of the
// profiler)
//
// ======================================================================================

#pragma once

#include "../../../holder.h"

#include <map>
#include <vector>
using namespace std;

#define dimensionof(a)      (sizeof(a)/sizeof(*(a)))


struct ProfConfig;
class ReJITScript;

#undef FAILURE
#undef DISPLAY

#define DISPLAY( message )  {                                                                       \
                                assert(pPrfCom != NULL);                                            \
                                std::wstringstream temp;                                            \
                                temp << message;                                                    \
                                pPrfCom->Display(temp);  				                            \
                            }

#define FAILURE( message )  {                                                                       \
                                assert(pPrfCom != NULL);                                            \
                                std::wstringstream temp;                                            \
                                temp << message;                                                    \
                                pPrfCom->Error(temp) ;  					                        \
                            }

#define TEST_FAILURE_OUTSIDE_CALLBACK_CLASS(message)   \
{ \
    IPrfCom * pPrfCom = m_pPrfCom; \
    /* DebugBreak(); */                                                          \
    FAILURE(L"TEST FAILURE: ");                         \
    FAILURE(message);       \
    /* throw; */   \
}

#define TEST_FAILURE(message)                                                  \
{ \
    IPrfCom * pPrfCom = m_pPrfCom; \
    /* DebugBreak();           */                                               \
    FAILURE(L"TEST FAILURE at Orange script line number " << std::dec << m_iScriptLineNumberCur << L": ");   \
    FAILURE(message);   \
    /* throw; */   \
}

#define TEST_DISPLAY(message)                                                  \
{ \
    IPrfCom * pPrfCom = m_pPrfCom; \
    DISPLAY(message);   \
}

struct ScriptEntry
{
    enum Command
    {
        kWaitJit,
        kWaitCall,
        kWaitModule,
        kWaitInline,
        kWaitRejitError,
        kRejit,
        kRejitWithInlines,
        kRejitThreadLoop,
        kRevert,
        kRevertThreadLoop,
        kStackwalk,
        kStopAllThreadLoops,
        kWaitRejitRepeatUntil,
        kNop,
    };

    Command command;
    PLATFORM_WCHAR wszOrigEntryText[1024];
    PLATFORM_WCHAR wszModule[256];
    PLATFORM_WCHAR wszClass[256];
    PLATFORM_WCHAR wszFunction[256];
    PLATFORM_WCHAR wszFunction2[256];
    PLATFORM_WCHAR wszFunction3[256];
    int nVersion;
    DWORD dwSleepMs;
    int nLineNumber;
    BOOL fShared;
    HRESULT hrRejitError;
};

template <class _ID, class _Info>
class IDToInfoMap
{
public:
    typedef std::map<_ID, _Info> Map;
    typedef typename Map::iterator Iterator;
    typedef typename Map::const_iterator Const_Iterator;
    typedef typename Map::size_type Size_type;

    IDToInfoMap(IPrfCom * pPrfCom)
        : m_pPrfCom(pPrfCom)
    {
    }

    Size_type GetCount()
    {
        std::lock_guard<recursive_mutex> csHolder(m_cs);
        return m_map.size();
    }

    BOOL LookupIfExists(_ID id, _Info * pInfo)
    {
        std::lock_guard<recursive_mutex> csHolder(m_cs);
        Const_Iterator iterator = m_map.find(id);
        if (iterator == m_map.end())
        {
            return FALSE;
        }

        *pInfo = iterator->second;
        return TRUE;
    }

    _Info Lookup(_ID id)
    {
        std::lock_guard<recursive_mutex> csHolder(m_cs);
        _Info info;
        if (!LookupIfExists(id, &info))
        {
            TEST_FAILURE_OUTSIDE_CALLBACK_CLASS(L"IDToInfoMap lookup failed.\n");
        }

        return info;
    }

    void Erase(_ID id)
    {
        std::lock_guard<recursive_mutex> csHolder(m_cs);
        Size_type cElementsRemoved = m_map.erase(id);
        if (cElementsRemoved != 1)
        {
            TEST_FAILURE_OUTSIDE_CALLBACK_CLASS(L"IDToInfoMap: Expected to remove 1 element, but removed '" 
                << cElementsRemoved << "' elements instead.");
        }
    }

    void Update(_ID id, _Info info)
    {
        std::lock_guard<recursive_mutex> csHolder(m_cs);
        m_map[id] = info;
    }

    Const_Iterator Begin()
    {
        return m_map.begin();
    }

    Const_Iterator End()
    {
        return m_map.end();
    }

    class LockHolder
    {
    public:
        LockHolder(IDToInfoMap<_ID, _Info> * pIDToInfoMap) :
            m_csHolder(pIDToInfoMap->m_cs)
        {
        }

    private:
        std::lock_guard<recursive_mutex> m_csHolder;
    };

private:
    Map m_map;
    std::recursive_mutex m_cs;
    IPrfCom * m_pPrfCom;
};

typedef IDToInfoMap<mdMethodDef, int> MethodDefToLatestVersionMap;
    
struct ModuleInfo
{
    PLATFORM_WCHAR                      m_wszModulePath[512];
    IMetaDataImport *                   m_pImport;
    mdToken                             m_mdEnterProbeRef;
    mdToken                             m_mdExitProbeRef;

    // Map methodDef metadata token to the latest version number (from the script) that we've
    // rejitted it to, in this module.
    MethodDefToLatestVersionMap *       m_pMethodDefToLatestVersionMap;
};

struct FunctionVersion
{
    // This array is indexed by the script-supplied version number
    ReJITID m_rgRejitIDs[100];
};

// A single entry in the single-thread shadow stack
struct ShadowStackFrameInfo
{
    ModuleID        m_moduleID;
    mdMethodDef     m_methodDef;
    int             m_nVersion;
};

struct DSSFrameInfo
{
    FunctionID          m_functionID;
    UINT_PTR            m_ip;
    ModuleID            m_moduleID;
    mdMethodDef         m_methodDef;
};

struct DSSData
{
    ReJITScript *               m_pCallback;
    ThreadID                    m_threadID;
    DSSFrameInfo                m_rgDSSFrameInfos[100];
    int                         m_cDSSFrameInfos;
};

struct ReJitTestInfo 
{
    ReJITScript *               m_pCallback;
    int                         m_iCommandFirst;
    int                         m_iCommandLast;
};

class ReJITScript
{
public:
    enum TestType
    {
        kRejitTestType_Deterministic,
        kRejitTestType_Nondeterministic,
    };

    static void Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const std::wstring& testName);


    ReJITScript(IPrfCom * pPrfCom, TestType testType, BOOL isAttach);
    virtual ~ReJITScript();

    HRESULT InitializeCommon();
    HRESULT Verify();
    HRESULT AppDomainCreationStarted(AppDomainID appDomainID);
    HRESULT AppDomainShutdownFinished(AppDomainID appDomainID, HRESULT hrStatus);
    HRESULT ModuleLoadFinished(ModuleID moduleID, HRESULT hrStatus);
    HRESULT ModuleUnloadStarted(ModuleID moduleID);
    HRESULT JITCompilationStarted(FunctionID functionID, BOOL fIsSafeToBlock);
    HRESULT JITCompilationFinished(FunctionID functionID, HRESULT hrStatus, BOOL fIsSafeToBlock);
    HRESULT JITInlining(FunctionID caller, FunctionID callee, BOOL * pfShouldInline);
    HRESULT ReJITCompilationStarted(FunctionID functionID, ReJITID rejitId, BOOL fIsSafeToBlock);
    HRESULT GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl);
    HRESULT ReJITCompilationFinished(FunctionID functionId, ReJITID rejitId, HRESULT hrStatus, BOOL fIsSafeToBlock);
    HRESULT ReJITError(ModuleID moduleId, mdMethodDef methodId, FunctionID functionId, HRESULT hrStatus);
    HRESULT StackSnapshotCallback(
        FunctionID funcId,
        UINT_PTR ip,
        COR_PRF_FRAME_INFO frameInfo,
        ULONG32 contextSize,
        BYTE context[],
        DSSData * pDssData);

    void NtvEnteredFunction(ModuleID moduleIDCur, mdMethodDef mdCur, int nVersionCur);
    void NtvExitedFunction(ModuleID moduleIDCur, mdMethodDef mdCur, int nVersionCur);

protected:
    static HRESULT SupportsInterface(REFIID riid);
    static DWORD WINAPI TestReJitOrRevertStatic(LPVOID lpParameter);
    static DWORD WINAPI DoStackwalkThreadProc(LPVOID pParameter);

    typedef IDToInfoMap<FunctionID, FunctionVersion> FunctionIDToVersionMap;
    typedef IDToInfoMap<ModuleID, ModuleInfo> ModuleIDToInfoMap;
    
    TestType m_testType;
    COMPtrHolder<ICorProfilerInfo6> m_pProfilerInfo;
    volatile LONG m_cModuleInfos;
    IPrfCom * m_pPrfCom;

    // FunctionID -> (Script Version Number -> ReJITID)
    FunctionIDToVersionMap m_functionIDToVersionMap;

    // ModuleID -> (Misc info, plus methodDef -> latest script version number)
    ModuleIDToInfoMap m_moduleIDToInfoMap;

    // No-frills shadow stack. Like everything else about the deterministic
    // rejit script tests, this assumes only a single managed thread that
    // calls rejitted functions.
    ShadowStackFrameInfo m_rgShadowStackFrameInfo[100];
    int m_cShadowStackFrameInfos;

    // Parsed script
    ScriptEntry m_rgScriptEntries[1000];
    DWORD m_cScriptEntries;
    DWORD m_iScriptEntryCur;
    int m_iScriptLineNumberCur;

    BOOL m_fAtLeastOneSuccessfulRejitFinished;

    std::atomic<LONG> m_cAppDomains;

    // * TRUE means instrumented code should call into GAC'd ProfilerHelper.dll
    // * FALSE means instrumented code should call into new helper methods we
    //     pump into mscorlib.dll
    BOOL m_fInstrumentationHooksInSeparateAssembly;

    BOOL m_fIsAttachScenario;

    // If the instrumented code must call into managed helpers that we pump
    // into mscorlib (as opposed to calling into managed helpers statically
    // compiled into ProfilerHelper.dll), then these tokens are used to
    // refer to those helpers as they will appear in the modified mscorlib
    // metadata.
    mdMethodDef m_mdEnterPInvoke;
    mdMethodDef m_mdExitPInvoke;
    mdMethodDef m_mdEnter;
    mdMethodDef m_mdExit;
    mdMethodDef m_mdIntPtrExplicitCast;

    ModuleID m_modidMscorlib;
    std::atomic<BOOL> m_fStopAllThreadLoops;
    std::atomic<LONG> m_cLoopingThreads;
    int m_cRejitStarted;
    int m_getReJITParametersDelayMs;

    void SpawnThreadToDoStackwalk();
    void DoStackwalk(DSSData * pDssData);
    void CompareDSSAndShadowStacks(DSSData * pDssData);
    BOOL CompareDSSAndShadowFrames(
        DSSFrameInfo * pDssFrame, 
        ShadowStackFrameInfo * pShadowFrame,
        int nShadowFrame);

    void CallRequestReJIT(
        UINT cFunctionsToRejit, 
        ModuleID * rgModuleIDs, 
        mdMethodDef * rgMethodIDs);

    void CallRequestRevert(
        UINT cFunctionsToRevert, 
        ModuleID * rgModuleIDs, 
        mdMethodDef * rgMethodIDs);

    void CallRequestReJIT(const vector<COR_PRF_METHOD> &methods);
    void RequestReJITWithNgenInlines(const vector<COR_PRF_METHOD> &inlinees);

    void TestReJitOrRevert(int iCommandFirst, int iCommandLast);
    HRESULT AddCoreLib();
    void TestReJitOrRevertEntireModule(
        ModuleID moduleID,
        const ModuleInfo * pModInfo,
        ScriptEntry::Command cmd,
        int nVersion);
    BOOL ShouldAvoidInstrumentingType(IMetaDataImport * pImport, mdTypeDef td);
    void DisableInlining(ICorProfilerFunctionControl * pFunctionControl);

    HRESULT TestCatchUp();
    void    TestGetReJITIDs(FunctionID functionId);
    
    void BlockIfNecessary(LPCSTR szCallbackName);
    BOOL SeekToNextNewline(FILE * fp);
    BOOL ScriptFileToString(wchar_t *wszScript, UINT cchScript);
    HRESULT InterpretLineFromScript(wchar_t *wszLine, int nLineNumber);
    HRESULT ReadScript();
    void GetClassAndFunctionNamesFromMethodDef(
        IMetaDataImport * pImport,
        ModuleID moduleID, 
        mdMethodDef methodDef,
        PROFILER_WCHAR *wszTypeDefName,
        ULONG cchTypeDefName,
        PROFILER_WCHAR *wszMethodDefName,
        ULONG cchMethodDefName);
    void VerifyFunctionVersion(
        ModuleID moduleID,
        mdMethodDef methodDef,
        int nVersion);
    BOOL DoRejitFromRepeatCommandIfNecessary(ModuleID moduleID, mdMethodDef methodDef);
    BOOL FinishWaitIfAppropriate(
        ModuleID moduleID,
        mdMethodDef methodDef,
        mdMethodDef methodDef2,
        int nVersionCur,
        BOOL fShared,
        HRESULT hrRejitError,
        ScriptEntry::Command command);
    BOOL DoesModuleMatchScriptEntry(ModuleID moduleID, BOOL fShared, ScriptEntry * pEntry);
    BOOL DoesFunctionMatchScriptEntry(
        ModuleID moduleID,
        mdMethodDef methodDef,
        ScriptEntry * pEntry,
        int nScriptFunctionNumber);
    void ExecuteCommandsUntilNextWait();
    void ExecuteRejitOrRevertCommands(ScriptEntry::Command cmd);
    void ExecuteRejitOrRevertCommandsSync(ScriptEntry::Command cmd);
    void ExecuteRejitOrRevertThreadLoopCommands(ScriptEntry::Command cmd);
    void InitThreadParamForRejitOrRevert(ScriptEntry::Command cmd, ReJitTestInfo * pParameter);
    void AddMemberRefs(IMetaDataAssemblyImport * pAssemblyImport, IMetaDataAssemblyEmit * pAssemblyEmit, IMetaDataEmit * pEmit, ModuleInfo * pModuleInfo);
    BOOL FindMscorlibReference(
        IMetaDataAssemblyImport * pAssemblyImport,
        mdAssemblyRef * rgAssemblyRefs,
        ULONG cAssemblyRefs,
        mdAssemblyRef * parMscorlib);
    mdMethodDef GetTokenFromName(
        IMetaDataImport * pImport, 
        const PLATFORM_WCHAR *wszClass,
        const PLATFORM_WCHAR *wszFunction);
    BOOL IsModuleInScript(const PLATFORM_WCHAR *wszModulePath);

    void AddHelperMethodDefs(IMetaDataImport * pImport, IMetaDataEmit * pEmit);
    void AddHelperTypeForwarders(IMetaDataAssemblyImport * pAssemblyImport, IMetaDataAssemblyEmit * pAssemblyEmit);
    void AddPInvoke(IMetaDataEmit * pEmit, mdTypeDef td, const PLATFORM_WCHAR *wszName, mdModuleRef modrefTarget, mdMethodDef * pmdPInvoke);
    void GetSecuritySafeCriticalAttributeToken(IMetaDataImport * pImport, mdMethodDef * pmdSafeCritical);
    void AddManagedHelperMethod(IMetaDataEmit * pEmit, mdTypeDef td, const PLATFORM_WCHAR *wszName, mdMethodDef mdTargetPInvoke, ULONG rvaDummy, mdMethodDef mdSafeCritical, mdMethodDef * pmdHelperMethod);
    void SetILFunctionBodyForManagedHelper(ModuleID moduleID, mdMethodDef methodDef);
    
    void AdvanceToNextScriptCommand()
    {
        TEST_DISPLAY(L"Finished executing line # '" << std::dec << m_iScriptLineNumberCur << L"': '" << 
            m_rgScriptEntries[m_iScriptEntryCur].wszOrigEntryText << L"'");
        m_iScriptEntryCur++;
        m_iScriptLineNumberCur = m_rgScriptEntries[m_iScriptEntryCur].nLineNumber;
    }
};

extern ReJITScript * g_pReJITScript;
