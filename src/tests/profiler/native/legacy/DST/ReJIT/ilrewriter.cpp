#include "../../ProfilerCommon.h"
#include "ReJITScript.h"
#include "../../../ILRewriter.h"
// #include <corhlpr.cpp>


extern "C" HRESULT AddProbe(
    ILRewriter * pilr,
    ModuleID moduleID,
    mdMethodDef methodDef, 
    int nVersion,
    UINT iLocalVersion,
    mdToken mdProbeRef,
    ILInstr * pInsertProbeBeforeThisInstr)
{
    // Add a call before the instruction stream:
    // 
    // Replace
    // 
    //     ...
    //     pInsertProbeBeforeThisInstr
    //     ...
    // 
    // with
    // 
    //     ...
    // #ifdef _X86_
    //     ldc.i4 moduleID
    // #else
    //     ldc.i8 moduleID
    // #endif
    //     ldc.i4 mdMethodDefCur
    //     ldc.i4 VersionNumberCur
    //     stloc LocalVarUsedForVersion
    //     ldloc LocalVarUsedForVersion
    //     call MgdEnteredFunction32/64
    //     pInsertProbeBeforeThisInstr
    //     ...

    ILInstr * pNewInstr = NULL;

    //     ldc.i4/8 moduleID
    pNewInstr = pilr->NewILInstr();
#ifdef _WIN64
    pNewInstr->m_opcode = CEE_LDC_I8;
    pNewInstr->m_Arg64 = moduleID;
#else
    pNewInstr->m_opcode = CEE_LDC_I4;
    pNewInstr->m_Arg32 = moduleID;
#endif
    pilr->InsertBefore(pInsertProbeBeforeThisInstr, pNewInstr);

    //     ldc.i4 mdMethodDefCur
    pNewInstr = pilr->NewILInstr();
    pNewInstr->m_opcode = CEE_LDC_I4;
    pNewInstr->m_Arg32 = methodDef;
    pilr->InsertBefore(pInsertProbeBeforeThisInstr, pNewInstr);

    //     ldc.i4 VersionNumberCur
    pNewInstr = pilr->NewILInstr();
    pNewInstr->m_opcode = CEE_LDC_I4;
    pNewInstr->m_Arg32 = nVersion;
    pilr->InsertBefore(pInsertProbeBeforeThisInstr, pNewInstr);

    //     stloc LocalVarUsedForVersion
    pNewInstr = pilr->NewILInstr();
    pNewInstr->m_opcode = CEE_STLOC;
    pNewInstr->m_Arg16 = (INT16) iLocalVersion;
    pilr->InsertBefore(pInsertProbeBeforeThisInstr, pNewInstr);

    //     ldloc LocalVarUsedForVersion
    pNewInstr = pilr->NewILInstr();
    pNewInstr->m_opcode = CEE_LDLOC;
    pNewInstr->m_Arg16 = (INT16) iLocalVersion;
    pilr->InsertBefore(pInsertProbeBeforeThisInstr, pNewInstr);

    //     call MgdEnteredFunction32/64 (may be via memberRef or methodDef)
    pNewInstr = pilr->NewILInstr();
    pNewInstr->m_opcode = CEE_CALL;
    pNewInstr->m_Arg32 = mdProbeRef;
    pilr->InsertBefore(pInsertProbeBeforeThisInstr, pNewInstr);
    
    return S_OK;
}


extern "C" HRESULT AddEnterProbe(
    ILRewriter * pilr,
    ModuleID moduleID,
    mdMethodDef methodDef, 
    int nVersion,
    UINT iLocalVersion, 
    mdToken mdEnterProbeRef)
{
    ILInstr * pFirstOriginalInstr = pilr->GetILList()->m_pNext;

    return AddProbe(pilr, moduleID, methodDef, nVersion, iLocalVersion, mdEnterProbeRef, pFirstOriginalInstr);
}


extern "C" HRESULT AddExitProbe(
    ILRewriter * pilr,
    ModuleID moduleID,
    mdMethodDef methodDef, 
    int nVersion,
    UINT iLocalVersion, 
    mdToken mdExitProbeRef)
{
    HRESULT hr;
    BOOL fAtLeastOneProbeAdded = FALSE;

    // Find all RETs, and insert a call to the exit probe before each one.
    for (ILInstr * pInstr = pilr->GetILList()->m_pNext; pInstr != pilr->GetILList(); pInstr = pInstr->m_pNext)
    {
        switch (pInstr->m_opcode)
        {
        case CEE_RET:
        {
            // We want any branches or leaves that targeted the RET instruction to
            // actually target the epilog instructions we're adding. So turn the "RET"
            // into ["NOP", "RET"], and THEN add the epilog between the NOP & RET. That
            // ensures that any branches that went to the RET will now go to the NOP and
            // then execute our epilog.
            
            // RET->NOP
            pInstr->m_opcode = CEE_NOP;
            
            // Add the new RET after
            ILInstr * pNewRet = pilr->NewILInstr();
            pNewRet->m_opcode = CEE_RET;
            pilr->InsertAfter(pInstr, pNewRet);

            // Add now insert the epilog before the new RET
            hr = AddProbe(pilr, moduleID, methodDef, nVersion, iLocalVersion, mdExitProbeRef, pNewRet);
            if (FAILED(hr))
                return hr;
            fAtLeastOneProbeAdded = TRUE;
            
            // Advance pInstr after all this gunk so the for loop continues properly
            pInstr = pNewRet;
            break;
        }

        default:
            break;
        }
    }
    
    // TODO: this causes a failure when a method just has a throw, should it be omitted
    // or should there be a better check for methods that just throw?
    // if (!fAtLeastOneProbeAdded)
    //     return E_FAIL;

    return S_OK;
}


extern HRESULT RewriteIL(
    ICorProfilerInfo * pICorProfilerInfo,
    ICorProfilerFunctionControl * pICorProfilerFunctionControl,
    ModuleID moduleID,
    mdMethodDef methodDef,
    int nVersion,
    mdToken mdEnterProbeRef,
    mdToken mdExitProbeRef)
{
    HRESULT hr;
    ILRewriter rewriter(pICorProfilerInfo, pICorProfilerFunctionControl, moduleID, methodDef);

    IfFailRet(rewriter.Initialize());
    IfFailRet(rewriter.Import());
    if (nVersion != 0)
    {
        // Full deterministic test adds enter/exit probes
        _ASSERTE(mdEnterProbeRef != mdTokenNil);
        _ASSERTE(mdExitProbeRef != mdTokenNil);
        UINT iLocalVersion = rewriter.AddNewInt32Local();
        IfFailRet(AddEnterProbe(&rewriter, moduleID, methodDef, nVersion, iLocalVersion, mdEnterProbeRef));
        IfFailRet(AddExitProbe(&rewriter, moduleID, methodDef, nVersion, iLocalVersion, mdExitProbeRef));
    }
    IfFailRet(rewriter.Export());

    return S_OK;
}

extern HRESULT SetILForManagedHelper(
    ICorProfilerInfo * pICorProfilerInfo,
    ModuleID moduleID,
    mdMethodDef mdHelperToAdd,
    mdMethodDef mdIntPtrExplicitCast,
    mdMethodDef mdPInvokeToCall)
{
    HRESULT hr;
    ILRewriter rewriter(
        pICorProfilerInfo, 
        NULL, // no ICorProfilerFunctionControl for classic-style on-first-JIT instrumentation
        moduleID, 
        mdHelperToAdd);

    rewriter.InitializeTiny();

    ILInstr * pFirstOriginalInstr = rewriter.GetILList()->m_pNext;
    ILInstr * pNewInstr = NULL;

    // nop
    pNewInstr = rewriter.NewILInstr();
    pNewInstr->m_opcode = CEE_NOP;
    rewriter.InsertBefore(pFirstOriginalInstr, pNewInstr);

    // ldarg.0 (uint32/uint64 ModuleIDCur)
    pNewInstr = rewriter.NewILInstr();
    pNewInstr->m_opcode = CEE_LDARG_0;
    rewriter.InsertBefore(pFirstOriginalInstr, pNewInstr);

    // conv.u8 (cast ModuleIDCur to a managed U8)
    pNewInstr = rewriter.NewILInstr();
    pNewInstr->m_opcode = CEE_CONV_U8;
    rewriter.InsertBefore(pFirstOriginalInstr, pNewInstr);

    // call System.IntPtr::op_Explicit(int64) (convert ModuleIDCur to native
    // int)
    pNewInstr = rewriter.NewILInstr();
    pNewInstr->m_opcode = CEE_CALL;
    pNewInstr->m_Arg32 = mdIntPtrExplicitCast;
    rewriter.InsertBefore(pFirstOriginalInstr, pNewInstr);

    // ldarg.1 (uint32 methodDef of function being entered/exited)
    pNewInstr = rewriter.NewILInstr();
    pNewInstr->m_opcode = CEE_LDARG_1;
    rewriter.InsertBefore(pFirstOriginalInstr, pNewInstr);

    // ldarg.2 (int32 rejitted version number of function being entered/exited)
    pNewInstr = rewriter.NewILInstr();
    pNewInstr->m_opcode = CEE_LDARG_2;
    rewriter.InsertBefore(pFirstOriginalInstr, pNewInstr);

    // call the PInvoke, which will target the profiler's NtvEnter/ExitFunction
    pNewInstr = rewriter.NewILInstr();
    pNewInstr->m_opcode = CEE_CALL;
    pNewInstr->m_Arg32 = mdPInvokeToCall;
    rewriter.InsertBefore(pFirstOriginalInstr, pNewInstr);

    // nop
    pNewInstr = rewriter.NewILInstr();
    pNewInstr->m_opcode = CEE_NOP;
    rewriter.InsertBefore(pFirstOriginalInstr, pNewInstr);

    // ret
    pNewInstr = rewriter.NewILInstr();
    pNewInstr->m_opcode = CEE_RET;
    rewriter.InsertBefore(pFirstOriginalInstr, pNewInstr);

    IfFailRet(rewriter.Export());

    return S_OK;
}
