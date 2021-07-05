#pragma once

#include "../../../holder.h"

#define dimensionof(a) 		(sizeof(a)/sizeof(*(a)))

struct ProfConfig;
class InjectMetaData;


#undef DISPLAY
#undef LOG
#undef FAILURE
#undef VERBOSE

#define DISPLAY( message )  {                                                                           \
                                std::wstringstream temp;                                                \
                                temp << message;                                                        \
                                m_pPrfCom->Display(temp);                                               \
                            }

#define FAILURE( message )  {                                                                           \
                                std::wstringstream temp;                                                \
                                temp << message;                                                        \
                                m_pPrfCom->Error(temp) ;                                                \
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
    FAILURE(message);   \
    /* throw; */   \
}

#define TEST_DISPLAY(message)                                                  \
{ \
    IPrfCom * pPrfCom = m_pPrfCom; \
    DISPLAY(message);   \
}

// Quicker form of ContainsAtEnd when prospective ending is a constant string
#define ENDS_WITH(str, constant) \
    (_wcsicmp(constant, &str[wcslen(str) - ((sizeof(constant) / sizeof(constant[0])) - 1)]) == 0)

#define MAX_MSIL_OPCODE (0x22+0x100)

// class CSHolder
// {
// public:
//     CSHolder(CRITICAL_SECTION * pcs)
//     {
//         m_pcs = pcs;
//         EnterCriticalSection(m_pcs);
//     }

//     ~CSHolder()
//     {
//         _ASSERTE(m_pcs != NULL);
//         LeaveCriticalSection(m_pcs);
//     }

// private:
//     CRITICAL_SECTION * m_pcs;
// };

// This is the base class for all the tests. 
class InjectMetaData
{
public:
    enum TestType
    {
        kNewUserString,
        kInjectTypes,
        kInjectGenericTypes,
        kInjectModuleRef,
        kInjectAssemblyRef,
        kInjectMethodSpec,
        kInjectTypeSpec,
        kInjectExternalMethodCall,
        kInjectExternalMethodCallCoreLib
    };

    static void Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& testName);

    InjectMetaData(IPrfCom * pPrfCom, TestType testType);
    virtual ~InjectMetaData();

    virtual HRESULT Verify();
    HRESULT AppDomainCreationStarted(AppDomainID appDomainID);
    HRESULT AppDomainShutdownFinished(AppDomainID appDomainID, HRESULT hrStatus);
    virtual HRESULT ModuleLoadFinished(ModuleID moduleID, HRESULT hrStatus);
    HRESULT ModuleUnloadStarted(ModuleID moduleID);
    virtual HRESULT JITCompilationStarted(FunctionID functionID, BOOL fIsSafeToBlock);
    HRESULT ReJITCompilationStarted(FunctionID functionID, ReJITID rejitId, BOOL fIsSafeToBlock);
    virtual HRESULT GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl);
    virtual HRESULT ReJITError(ModuleID moduleId, mdMethodDef methodId, FunctionID functionId, HRESULT hrStatus);

    virtual int NativeFunction();
protected:
    TestType m_testType;
    COMPtrHolder<ICorProfilerInfo7> m_pProfilerInfo;
    IPrfCom * m_pPrfCom;

    void CallRequestReJIT(UINT cFunctionsToRejit, ModuleID * rgModuleIDs, mdMethodDef * rgMethodIDs);
    void CallRequestReJIT(const vector<COR_PRF_METHOD> &methods);
    void GetClassAndFunctionNamesFromMethodDef(IMetaDataImport * pImport, ModuleID moduleID, mdMethodDef methodDef, LPWSTR wszTypeDefName,ULONG cchTypeDefName, LPWSTR wszMethodDefName, ULONG cchMethodDefName);

protected:
    ULONG m_ulRejitCallCount;
    ModuleID m_modidTarget;
    ModuleID m_modidSource;
    mdMethodDef m_tokmdTargetMethod;

    COMPtrHolder<IMetaDataEmit2> m_pTargetEmit;
    COMPtrHolder<IMetaDataImport2> m_pTargetImport;
    COMPtrHolder<IMetaDataImport2> m_pSourceImport;

    COMPtrHolder<IMetaDataAssemblyEmit> m_pAssemblyEmit;
    COMPtrHolder<IMetaDataAssemblyImport> m_pAssemblyImport;
};

extern InjectMetaData * g_pInjectMD;
