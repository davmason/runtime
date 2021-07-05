#pragma once

#include "stdafx.h"


// This class tests if a new type spec can be injected on REJIT and exercised by the new IL.
// The target program the profiler runs against is MDInjectTarget.exe 
// It is done in the following steps
//      When the module load is complete for MDInjectTarget.exe, REJIT request to TestTypeSpec.CallMethod is made.
//      IL function body is changed to exercise a method in the new typespec. 
class InjectTypeSpec : public InjectMethodSpec
{
public:
    InjectTypeSpec(IPrfCom * pPrfCom, TestType testType);
    ~InjectTypeSpec();
    virtual HRESULT Verify();
    virtual HRESULT GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl);
    virtual HRESULT ModuleLoadFinished(ModuleID moduleID, HRESULT hrStatus);
    virtual HRESULT JITCompilationStarted(FunctionID functionID, BOOL fIsSafeToBlock);
protected:
    HRESULT RewriteIL(ICorProfilerFunctionControl * pICorProfilerFunctionControl, ModuleID moduleID, mdMethodDef methodDef, mdMemberRef methodToCall);
};