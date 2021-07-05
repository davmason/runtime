#pragma once

#include "stdafx.h"


// This class tests if a new assemblyref can be injected on REJIT and exercised by the new IL.
// The target program the profiler runs against is MDInjectTarget.exe 
// It is done in the following steps
//      Running "MDInjectTarget.exe NewTypes" under the test profiler executes TestAssemblyRef.CallMethod first with regular JIT.
//      MDInjectTarget.ControlFunction causes REJIT to be triggered
//      REJIT on Profiler: Injects a typeref and memberref from MDInjectSource.dll
//      Second call to TestAssemblyRef.CallMethod executes newly injected types/methods.
class InjectAssemblyRef : public InjectMetaData
{
protected:
    mdAssemblyRef m_tokmdSourceAssemblyRef;

public:
    InjectAssemblyRef(IPrfCom * pPrfCom, TestType testType);
    ~InjectAssemblyRef();
    virtual HRESULT Verify();
    virtual HRESULT GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl);

protected:
    HRESULT RewriteIL(ICorProfilerFunctionControl * pICorProfilerFunctionControl, ModuleID moduleID, mdMethodDef methodDef, mdMemberRef methodToCall);
    void InjectAssembly(WCHAR* assemblyToInject, ModuleID targetModuleId, const vector<WCHAR*> &types);
    HRESULT GetTestMethod(LPCWSTR pwzTypeName, LPCWSTR pwzMethodName, mdMemberRef* tokMethod);

    HRESULT AddAssemblyRef();
};