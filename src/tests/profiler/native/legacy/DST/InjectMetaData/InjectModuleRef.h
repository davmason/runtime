#pragma once

#include "stdafx.h"

// This class tests if a new moduleref can be injected on REJIT and exercised by the new IL by a PInvoke call.
// The target program the profiler runs against is MDInjectTarget.exe 
// It is done in the following steps
//      Running "MDInjectTarget.exe NewTypes" under the test profiler executes TestModuleRef.CallMethod first with regular JIT.
//      MDInjectTarget.ControlFunction causes REJIT to be triggered
//      REJIT on Profiler: Injects a PInvoke from unmanaged binary InjectMetaData.dll
//      Second call to TestModuleRef.CallMethod executes newly injected types/methods.
class InjectModuleRef : public InjectMetaData
{
protected:
    mdModuleRef m_tokModuleRef;
    bool m_fNativeFunctionCalled;

public:
    InjectModuleRef(IPrfCom * pPrfCom, TestType testType);
    ~InjectModuleRef();
    virtual HRESULT Verify();
    virtual HRESULT GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl);

protected:
    HRESULT RewriteIL(ICorProfilerFunctionControl * pICorProfilerFunctionControl, ModuleID moduleID, mdMethodDef methodDef, mdMethodDef methodToCall);
    HRESULT GetTestMethod(LPCWSTR pwzTypeName, LPCWSTR pwzMethodName, mdMemberRef* tokMethod);

    virtual int NativeFunction();

private:
    HRESULT AddPInvoke(mdTypeDef td, LPCWSTR wszName, mdModuleRef modrefTarget, mdMethodDef *pmdPInvoke);
};