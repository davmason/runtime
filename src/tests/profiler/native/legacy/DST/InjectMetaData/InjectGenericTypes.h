#pragma once

#include "stdafx.h"

// This class tests if a new generic types can be injected on REJIT and exercised by the new IL.
// The target program the profiler runs against is MDInjectTarget.exe 
// It is done in the following steps
//      Running "MDInjectTarget.exe NewTypes" under the test profiler executes TestAssemblyInjection.CallMethod first with regular JIT.
//      MDInjectTarget.ControlFunction causes REJIT to be triggered
//      REJIT on Profiler: Injects few generic types and methods from assembly MDInjectSource.dll
//      Second call to TestAssemblyInjection.CallMethod exercises newly injected types/methods.
class InjectGenericTypes : public InjectTypes
{
public:
    InjectGenericTypes(IPrfCom * pPrfCom, TestType testType);
    virtual ~InjectGenericTypes();
    HRESULT Verify();
    HRESULT GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl);
};
