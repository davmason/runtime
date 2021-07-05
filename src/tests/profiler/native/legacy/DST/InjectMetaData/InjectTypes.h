#pragma once

#include "stdafx.h"

// This class tests if a new typedefs and methoddefs can be injected on REJIT and exercised by the new IL.
// The target program the profiler runs against is MDInjectTarget.exe 
// It is done in the following steps
//      Running "MDInjectTarget.exe NewTypes" under the test profiler executes TestAssemblyInjection.CallMethod first with regular JIT.
//      MDInjectTarget.ControlFunction causes REJIT to be triggered
//      REJIT on Profiler: Injects few simple typedefs/methoddefs from assembly MDInjectSource.dll
//      Second call to TestAssemblyInjection.CallMethod executes newly injected types/methods.
class InjectTypes : public InjectMetaData
{
public:
    InjectTypes(IPrfCom * pPrfCom, TestType testType);
    ~InjectTypes();
    virtual HRESULT Verify();
    virtual HRESULT GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl);

protected:
    HRESULT RewriteIL(ICorProfilerFunctionControl * pICorProfilerFunctionControl, ModuleID moduleID, mdMethodDef methodDef, mdMethodDef methodToCall);
    void InjectAssembly(ModuleID targetModuleId, const vector<const PLATFORM_WCHAR*> &types);
    HRESULT GetTestMethod(LPCWSTR pwzTypeName, LPCWSTR pwzMethodName, mdMemberRef* tokMethod);
};