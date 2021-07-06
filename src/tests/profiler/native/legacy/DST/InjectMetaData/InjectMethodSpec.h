// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#pragma once

#include "stdafx.h"


// This class tests if a new method spec can be injected on REJIT and exercised by the new IL.
// The target program the profiler runs against is MDInjectTarget.exe 
// It is done in the following steps
//      When the module load is complete for MDInjectTarget.exe, REJIT request to TestMethodSpec.CallMethod is made.
//      IL function body is changed to exercise the new methodspec. 
class InjectMethodSpec : public InjectMetaData
{
public:
    InjectMethodSpec(IPrfCom * pPrfCom, TestType testType);
    ~InjectMethodSpec();
    virtual HRESULT Verify();
    virtual HRESULT GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl);
    virtual HRESULT ModuleLoadFinished(ModuleID moduleID, HRESULT hrStatus);
    virtual HRESULT JITCompilationStarted(FunctionID functionID, BOOL fIsSafeToBlock);
protected:
    HRESULT RewriteIL(ICorProfilerFunctionControl * pICorProfilerFunctionControl, ModuleID moduleID, mdMethodDef methodDef, mdMemberRef methodToCall);
};