// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#pragma once

#include "stdafx.h"


// This class tests if a method can be added to an external assembly can be injected on REJIT and exercised by the new IL.
// The target program the profiler runs against is MDInjectTarget.exe 
// It is done in the following steps
//      When the module load is complete for MDInjectTarget.exe, REJIT request to TestMethodSpec.CallMethod is made.
//      A new method is injected in to MDInjectSource.exe and an assembly ref from MDInjectTarget to MDInjectSource is added
//      IL function body is changed to call the injected method. 
class InjectExternalCall : public InjectMetaData
{
public:
    InjectExternalCall(IPrfCom * pPrfCom, TestType testType);
    ~InjectExternalCall();
    virtual HRESULT Verify();
    virtual HRESULT GetReJITParameters(ModuleID moduleId, mdMethodDef methodId, ICorProfilerFunctionControl *pFunctionControl);
    virtual HRESULT ModuleLoadFinished(ModuleID moduleID, HRESULT hrStatus);
    virtual HRESULT JITCompilationStarted(FunctionID functionID, BOOL fIsSafeToBlock);
protected:
    HRESULT RewriteIL(ICorProfilerFunctionControl * pICorProfilerFunctionControl, ModuleID moduleID, mdMethodDef methodDef, mdMemberRef methodToCall);
    HRESULT InjectTypeInCoreLib(mdMemberRef *memberRef);
    HRESULT InjectTypeInSource(mdMemberRef *memberRef);
    HRESULT GetMemberRefFromAssemblyRef(mdAssemblyRef assemblyRef, mdMemberRef *memberRef);
    HRESULT GetAssemblyRef(COMPtrHolder<IMetaDataAssemblyImport> pImport, wstring refName, mdAssemblyRef *assemblyRef);
    BOOL MethodNameMatches(IMetaDataImport2 *pImport, ModuleID moduleID, mdMethodDef methodDef, wstring methodName);
private:
	ULONG GetILForSampleMethod(ModuleID moduleID);

    COMPtrHolder<IMetaDataImport2> m_pCoreLibImport;
    BOOL m_bExternalMethodJitted;
    BOOL m_bInjectCoreLib;
	ModuleID m_modidCoreLib;
    ModuleID m_modidSystemRuntime;
};