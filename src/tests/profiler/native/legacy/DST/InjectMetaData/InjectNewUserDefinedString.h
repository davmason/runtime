#pragma once

#include "stdafx.h"

// This class tests if a new user defined string can be injected on REJIT and exercised by the new IL.
// The target program the profiler runs against is MDInjectTarget.exe 
// It is done in the following steps
//      Running "MDInjectTarget.exe NewUserString" under the test profiler executes PrintHelloWorld first with regular JIT.
//      MDInjectTarget.ControlFunction causes REJIT to be triggered
//      REJIT on Profiler: Injects a new user defined string, modifies IL to print the new string instead.
//      Second call to PrintHelloWorld prints the newly injected string
class InjectNewUserDefinedString : public InjectMetaData
{
public:
    InjectNewUserDefinedString(IPrfCom * pPrfCom, TestType testType);
    ~InjectNewUserDefinedString();
    HRESULT Verify();
    virtual HRESULT JITCompilationStarted(FunctionID functionID, BOOL fIsSafeToBlock);
    HRESULT GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl);

protected:
    HRESULT RewriteIL(ICorProfilerFunctionControl * pICorProfilerFunctionControl, ModuleID moduleID, mdMethodDef methodDef);
};