
// ==++==
//
//   Copyright (c) Microsoft Corporation.  All rights reserved.
//
// ==--==
//
// TestProfiler.cpp
//
// Implementation of ICorProfilerCallback2.
//
// ======================================================================================
#include "TestProfiler.h"
#include <algorithm>
#include <cctype>

extern "C" void CEE_Satellite_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring &testName);
extern "C" void ELT_Satellite_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring &testName);
extern "C" void InjectMetaData_Satellite_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring &testName);
extern "C" void Instrumentation_Satellite_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring &testName);
extern "C" void ProfileHub_Satellite_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring &testName);
extern "C" void Regressions_Satellite_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring &testName);
extern "C" void ReJIT_Satellite_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring &testName);
extern "C" void UnitTests_Satellite_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring &testName);
extern "C" void GC_Satellite_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring &testName);
extern "C" void JIT_Satellite_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring &testName);
extern "C" void LTS_Satellite_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring &testName);
extern "C" void Tiered_Compilation_Satellite_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& testName);

#ifdef WINDOWS
extern "C" void AttachDetach_Satellite_Initialize(IPrfCom *pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring &testName);
#endif // WINDOWS

PMODULEMETHODTABLE pCallTable;
IPrfCom *g_satellite_pPrfCom;
extern IPrfCom *g_pPrfCom_LegacyCompat;

TestProfiler *TestProfiler::This = NULL;

// Static Enter/Leave/Tailcall methods call TestProfiler instance methods.  Post v1.1, no v1 callbacks
// require assembly.  They are called directly by the runtime as normal C functions.  Fast v2 x86 callbacks
// still require naked assembly to call the stubs which then call into instance methods.  On x64 the fast
// calls go through functions in the native assemblies; amd64helper.obj, Leave2x64.obj, and Tailcall2x64.obj.

#pragma region Enter(2) / Leave(2) / Tailcall(2)

EXTERN_C UINT_PTR STDMETHODCALLTYPE FunctionIDMapperStub(FunctionID functionId, BOOL *pbHookFunction)
{
    return TestProfiler::Instance()->FunctionIDMapper(functionId, pbHookFunction);
}

EXTERN_C UINT_PTR STDMETHODCALLTYPE FunctionIDMapper2Stub(FunctionID functionId, void *clientData, BOOL *pbHookFunction)
{
    return TestProfiler::Instance()->FunctionIDMapper2(functionId, clientData, pbHookFunction);
}

EXTERN_C VOID STDMETHODCALLTYPE EnterStub(FunctionID funcId)
{
    TestProfiler::Instance()->FunctionEnterCallBack(funcId);
}

EXTERN_C VOID STDMETHODCALLTYPE LeaveStub(FunctionID funcId)
{
    TestProfiler::Instance()->FunctionLeaveCallBack(funcId);
}

EXTERN_C VOID STDMETHODCALLTYPE TailcallStub(FunctionID funcId)
{
    TestProfiler::Instance()->FunctionTailcallCallBack(funcId);
}

VOID STDMETHODCALLTYPE Enter2Stub(FunctionID funcId, UINT_PTR clientData, COR_PRF_FRAME_INFO frameInfo, COR_PRF_FUNCTION_ARGUMENT_INFO *pArgInfo)
{
    TestProfiler::Instance()->FunctionEnter2CallBack(funcId, clientData, frameInfo, pArgInfo);
}

EXTERN_C
DECLSPEC_EXPORT
VOID STDMETHODCALLTYPE Leave2Stub(FunctionID funcId, UINT_PTR clientData, COR_PRF_FRAME_INFO frameInfo, COR_PRF_FUNCTION_ARGUMENT_RANGE *pRetVal)
{
    TestProfiler::Instance()->FunctionLeave2CallBack(funcId, clientData, frameInfo, pRetVal);
}

EXTERN_C
DECLSPEC_EXPORT
VOID STDMETHODCALLTYPE Tailcall2Stub(FunctionID funcId, UINT_PTR clientData, COR_PRF_FRAME_INFO frameInfo)
{
    TestProfiler::Instance()->FunctionTailcall2CallBack(funcId, clientData, frameInfo);
}

EXTERN_C VOID STDMETHODCALLTYPE FunctionEnter3Stub(FunctionIDOrClientID functionIDOrClientID)
{
    TestProfiler::Instance()->FunctionEnter3CallBack(functionIDOrClientID);
}

EXTERN_C VOID STDMETHODCALLTYPE FunctionLeave3Stub(FunctionIDOrClientID functionIDOrClientID)
{
    TestProfiler::Instance()->FunctionLeave3CallBack(functionIDOrClientID);
}

EXTERN_C VOID STDMETHODCALLTYPE FunctionTailCall3Stub(FunctionIDOrClientID functionIDOrClientID)
{
    TestProfiler::Instance()->FunctionTailCall3CallBack(functionIDOrClientID);
}

EXTERN_C VOID STDMETHODCALLTYPE FunctionEnter3WithInfoStub(FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo)
{
    TestProfiler::Instance()->FunctionEnter3WithInfoCallBack(functionIDOrClientID, eltInfo);
}

EXTERN_C VOID STDMETHODCALLTYPE FunctionLeave3WithInfoStub(FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo)
{
    TestProfiler::Instance()->FunctionLeave3WithInfoCallBack(functionIDOrClientID, eltInfo);
}

EXTERN_C VOID STDMETHODCALLTYPE FunctionTailCall3WithInfoStub(FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo)
{
    TestProfiler::Instance()->FunctionTailCall3WithInfoCallBack(functionIDOrClientID, eltInfo);
}

#if defined(_X86_)

VOID NAKED STDMETHODCALLTYPE Enter2Naked(FunctionID id, UINT_PTR clientData, COR_PRF_FRAME_INFO frame, COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo)
{
    __asm {
        // Set up EBP Frame (easy debugging)
        push ebp
        mov ebp, esp

        // Make space for locals.
        sub esp, __LOCAL_SIZE

        // Save all registers.
        pushad

        // Check if there's anything on the FP stack.
        fstsw   ax
        and     ax, 3800h // Check the top-of-fp-stack bits
        cmp     ax, 0 // If non-zero, we have something to save
        jnz     SaveFPReg
        push    0 // otherwise, mark that there is no float value
        jmp     NoSaveFPReg
SaveFPReg:
        sub     esp, 8 // Make room for the FP value
        fstp    qword ptr [esp] // Copy the FP value to the buffer as a double
        push    1 // mark that a float value is present
NoSaveFPReg:
    }

    Enter2Stub(id, clientData, frame, argumentInfo);

    __asm {
        cmp     [esp], 0 // Check the flag
        jz      NoRestoreFPRegs // If zero, no FP regs
        fld     qword ptr [esp + 4] // Restore FP regs
        add     esp, 12 // Move ESP past the storage space
        jmp     RestoreFPRegsDone
NoRestoreFPRegs:
        add     esp, 4 // Move ESP past the flag
RestoreFPRegsDone:

        // Restore other registers
        popad

        // Pop off locals
        add esp, __LOCAL_SIZE

        // Restore EBP
        pop ebp

        // stdcall: Callee cleans up args from stack on return
        ret SIZE id + SIZE clientData + SIZE frame + SIZE argumentInfo
    }
}

VOID NAKED STDMETHODCALLTYPE Leave2Naked(FunctionID id, UINT_PTR clientData, COR_PRF_FRAME_INFO frame, COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange)
{
    __asm {
        // Set up EBP Frame (easy debugging)
        push ebp
        mov ebp, esp

    // Make space for locals.
        sub esp, __LOCAL_SIZE

        // Save all registers.
        pushad

        // Check if there's anything on the FP stack.
        fstsw   ax
        and     ax, 3800h // Check the top-of-fp-stack bits
        cmp     ax, 0 // If non-zero, we have something to save
        jnz     SaveFPReg
        push    0 // otherwise, mark that there is no float value
        jmp     NoSaveFPReg
SaveFPReg:
        sub     esp, 8 // Make room for the FP value
        fstp    qword ptr [esp] // Copy the FP value to the buffer as a double
        push    1 // mark that a float value is present
NoSaveFPReg:
    }

    Leave2Stub(id, clientData, frame, retvalRange);

    __asm {
        cmp     [esp], 0 // Check the flag
        jz      NoRestoreFPRegs // If zero, no FP regs
        fld     qword ptr [esp + 4] // Restore FP regs
        add     esp, 12 // Move ESP past the storage space
        jmp     RestoreFPRegsDone
NoRestoreFPRegs:
        add     esp, 4 // Move ESP past the flag
RestoreFPRegsDone:

        // Restore other registers
        popad

        // Pop off locals
        add esp, __LOCAL_SIZE

        // Restore EBP
        pop ebp

        // stdcall: Callee cleans up args from stack on return
        ret SIZE id + SIZE clientData + SIZE frame + SIZE retvalRange
    }
}

VOID NAKED STDMETHODCALLTYPE Tailcall2Naked(FunctionID id, UINT_PTR clientData, COR_PRF_FRAME_INFO frame)
{
    __asm {
        // Set up EBP Frame (easy debugging)
        push ebp
        mov ebp, esp

        // Make space for locals.
        sub esp, __LOCAL_SIZE

        // Save all registers.
        pushad

        // Check if there's anything on the FP stack.
        fstsw   ax
        and     ax, 3800h // Check the top-of-fp-stack bits
        cmp     ax, 0 // If non-zero, we have something to save
        jnz     SaveFPReg
        push    0 // otherwise, mark that there is no float value
        jmp     NoSaveFPReg
SaveFPReg:
        sub     esp, 8 // Make room for the FP value
        fstp    qword ptr [esp] // Copy the FP value to the buffer as a double
        push    1 // mark that a float value is present
NoSaveFPReg:
    }

    Tailcall2Stub(id, clientData, frame);

    __asm {
        cmp     [esp], 0 // Check the flag
        jz      NoRestoreFPRegs // If zero, no FP regs
        fld     qword ptr [esp + 4] // Restore FP regs
        add     esp, 12 // Move ESP past the storage space
        jmp     RestoreFPRegsDone
NoRestoreFPRegs:
        add     esp, 4 // Move ESP past the flag
RestoreFPRegsDone:

        // Restore other registers
        popad

        // Pop off locals
        add esp, __LOCAL_SIZE

        // Restore EBP
        pop ebp

        // stdcall: Callee cleans up args from stack on return
        ret SIZE id + SIZE clientData + SIZE frame
    }
}

// C assembly helper for Fast Path Enter3
VOID NAKED STDMETHODCALLTYPE FunctionEnter3Naked(FunctionIDOrClientID functionIDOrClientID)
{
    __asm {
        // Set up EBP Frame (easy debugging)
        push ebp
        mov ebp, esp

        // Make space for locals.
        sub esp, __LOCAL_SIZE

        // Save all registers.
        pushad

        // Check if there's anything on the FP stack.
        fstsw   ax
        and     ax, 3800h // Check the top-of-fp-stack bits
        cmp     ax, 0 // If non-zero, we have something to save
        jnz     SaveFPReg
        push    0 // otherwise, mark that there is no float value
        jmp     NoSaveFPReg
SaveFPReg:
        sub     esp, 8 // Make room for the FP value
        fstp    qword ptr [esp] // Copy the FP value to the buffer as a double
        push    1 // mark that a float value is present
NoSaveFPReg:
    }

    FunctionEnter3Stub(functionIDOrClientID);

    __asm {
        cmp     [esp], 0 // Check the flag
        jz      NoRestoreFPRegs // If zero, no FP regs
        fld     qword ptr [esp + 4] // Restore FP regs
        add     esp, 12 // Move ESP past the storage space
        jmp     RestoreFPRegsDone
NoRestoreFPRegs:
        add     esp, 4 // Move ESP past the flag
RestoreFPRegsDone:

        // Restore other registers
        popad

        // Pop off locals
        add esp, __LOCAL_SIZE

        // Restore EBP
        pop ebp

        // stdcall: Callee cleans up args from stack on return
        ret SIZE functionIDOrClientID
    }
}

VOID NAKED STDMETHODCALLTYPE FunctionLeave3Naked(FunctionIDOrClientID functionIDOrClientID)
{
    __asm {
        // Set up EBP Frame (easy debugging)
        push ebp
        mov ebp, esp

        // Make space for locals.
        sub esp, __LOCAL_SIZE

        // Save all registers.
        pushad

        // Check if there's anything on the FP stack.
        fstsw   ax
        and     ax, 3800h // Check the top-of-fp-stack bits
        cmp     ax, 0 // If non-zero, we have something to save
        jnz     SaveFPReg
        push    0 // otherwise, mark that there is no float value
        jmp     NoSaveFPReg
SaveFPReg:
        sub     esp, 8 // Make room for the FP value
        fstp    qword ptr [esp] // Copy the FP value to the buffer as a double
        push    1 // mark that a float value is present
NoSaveFPReg:
    }

    FunctionLeave3Stub(functionIDOrClientID);

    __asm {
        cmp     [esp], 0 // Check the flag
        jz      NoRestoreFPRegs // If zero, no FP regs
        fld     qword ptr [esp + 4] // Restore FP regs
        add     esp, 12 // Move ESP past the storage space
        jmp     RestoreFPRegsDone
NoRestoreFPRegs:
        add     esp, 4 // Move ESP past the flag
RestoreFPRegsDone:

        // Restore other registers
        popad

        // Pop off locals
        add esp, __LOCAL_SIZE

        // Restore EBP
        pop ebp

        // stdcall: Callee cleans up args from stack on return
        ret SIZE functionIDOrClientID
    }
}

// Copied from FunctionTailCall2Naked and mofied the arguments
VOID NAKED STDMETHODCALLTYPE FunctionTailCall3Naked(FunctionIDOrClientID functionIDOrClientID)
{
    __asm {
        // Set up EBP Frame (easy debugging)
        push ebp
        mov ebp, esp

        // Make space for locals.
        sub esp, __LOCAL_SIZE

        // Save all registers.
        pushad

        // Check if there's anything on the FP stack.
        fstsw   ax
        and     ax, 3800h // Check the top-of-fp-stack bits
        cmp     ax, 0 // If non-zero, we have something to save
        jnz     SaveFPReg
        push    0 // otherwise, mark that there is no float value
        jmp     NoSaveFPReg
SaveFPReg:
        sub     esp, 8 // Make room for the FP value
        fstp    qword ptr [esp] // Copy the FP value to the buffer as a double
        push    1 // mark that a float value is present
NoSaveFPReg:
    }

    FunctionTailCall3Stub(functionIDOrClientID);

    __asm {
        cmp     [esp], 0 // Check the flag
        jz      NoRestoreFPRegs // If zero, no FP regs
        fld     qword ptr [esp + 4] // Restore FP regs
        add     esp, 12 // Move ESP past the storage space
        jmp     RestoreFPRegsDone
NoRestoreFPRegs:
        add     esp, 4 // Move ESP past the flag
RestoreFPRegsDone:

        // Restore other registers
        popad

        // Pop off locals
        add esp, __LOCAL_SIZE

        // Restore EBP
        pop ebp

        // stdcall: Callee cleans up args from stack on return
        ret SIZE functionIDOrClientID
    }
}

#elif defined(_AMD64_) || defined(_ARM_) || defined (__ARM_ARCH_ISA_A64) || defined (_M_ARM64)

EXTERN_C VOID STDMETHODCALLTYPE EnterNaked(FunctionID id);
EXTERN_C VOID STDMETHODCALLTYPE LeaveNaked(FunctionID id);
EXTERN_C VOID STDMETHODCALLTYPE TailcallNaked(FunctionID id);

EXTERN_C VOID STDMETHODCALLTYPE Leave2Naked(FunctionID funcId,
                                            UINT_PTR clientData,
                                            COR_PRF_FRAME_INFO frameInfo,
                                            COR_PRF_FUNCTION_ARGUMENT_RANGE *pRetVal);

EXTERN_C VOID STDMETHODCALLTYPE Tailcall2Naked(FunctionID id,
                                               UINT_PTR clientData,
                                               COR_PRF_FRAME_INFO frame);
EXTERN_C VOID STDMETHODCALLTYPE FunctionEnter3Naked(FunctionIDOrClientID functionIDOrClientID);
EXTERN_C VOID STDMETHODCALLTYPE FunctionLeave3Naked(FunctionIDOrClientID functionIDOrClientID);
EXTERN_C VOID STDMETHODCALLTYPE FunctionTailCall3Naked(FunctionIDOrClientID functionIDOrClientID);

#elif defined(_IA64_) 
EXTERN_C VOID STDMETHODCALLTYPE FunctionEnter3Naked(FunctionIDOrClientID functionIDOrClientID);
EXTERN_C VOID STDMETHODCALLTYPE FunctionLeave3Naked(FunctionIDOrClientID functionIDOrClientID);
EXTERN_C VOID STDMETHODCALLTYPE FunctionTailCall3Naked(FunctionIDOrClientID functionIDOrClientID);
#else
VOID STDMETHODCALLTYPE FunctionEnter3Naked(FunctionIDOrClientID functionIDOrClientID)
{
    _ASSERTE(!"@TODO: implement FunctionEnter3Naked for your platform");
}
VOID STDMETHODCALLTYPE FunctionLeave3Naked(FunctionIDOrClientID functionIDOrClientID)
{
    _ASSERTE(!"@TODO: implement FunctionLeave3Naked for your platform");
}
VOID STDMETHODCALLTYPE FunctionTailCall3Naked(FunctionIDOrClientID functionIDOrClientID)
{
    _ASSERTE(!"@TODO: implement FunctionTailCall3Naked for your platform");
}

#endif // _X86_

#pragma endregion // Enter(2)/Leave(2)/Tailcall(2)

// ----------------------------------------------------------------------------
//
// TestProfiler
//
// ----------------------------------------------------------------------------

TestProfiler::TestProfiler()
{
// Save instance pointer for use in static functions
#ifdef WINCORESYS
    m_cRef = 0;
#endif
    TestProfiler::This = this;
    m_profilerSatellite = NULL;

    memset(&m_methodTable, '\0', sizeof(MODULEMETHODTABLE));

    m_TestAppdomainId = 0;
#ifdef WINCORESYS
    AddRef();
#endif
}

TestProfiler::~TestProfiler()
{
    TestProfiler::This = NULL;
}

HRESULT TestProfiler::CommonInit(IUnknown *pProfilerInfoUnk)
{
    ShutdownGuard::Initialize();

    wstring wszSatelliteModule;
    wstring wszTestName;
    DWORD dwRet = 0;
    HRESULT hr = S_OK;

#ifdef _MSC_VER
    // First order of business is to redirect all assert info to the error stream.
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif // _MSC_VER

    // What satellite module should we load?
    wszSatelliteModule = ReadEnvironmentVariable(ENVVAR_BPD_SATELLITEMODULE);
    if (wszSatelliteModule.size() == 0)
        return E_UNEXPECTED;

    // What test do we execute?
    wszTestName = ReadEnvironmentVariable(ENVVAR_BPD_TESTNAME);
    if (wszTestName.size() == 0)
        return E_UNEXPECTED;

    // Save the ICorProfilerInfo interface
    hr = pProfilerInfoUnk->QueryInterface(IID_ICorProfilerInfo3, (void **)&m_pProfilerInfo);
    if (hr != S_OK)
    {
        FAILURE(L"QueryInterface() failed.");
        return E_FAIL;
    }
    // Create PrfCommon class for utility routines and structures used by BPD and satellites
    PPRFCOM = new PrfCommon(m_pProfilerInfo);

    // Load satellite module.
    if (!LoadSatelliteModule(wszSatelliteModule, wszTestName))
    {
        FAILURE(L"LoadSatelliteModule() failed.");
        return E_FAIL;
    }

    DISPLAY(L"\nProfilerEventMask: " << HEX(m_methodTable.FLAGS) << L"\n");
    hr = PINFO->SetEventMask(m_methodTable.FLAGS | COR_PRF_MONITOR_APPDOMAIN_LOADS);

    if (S_OK != hr)
    {
        FAILURE(L"ICorProfilerInfo::SetEventMask() failed with 0x" << HEX(hr) << L"\n");
        return E_FAIL;
    }

    // Keep track of Initialize callbacks received
    ++(PPRFCOM->m_Startup);

    return S_OK;
}

HRESULT TestProfiler::SetELTHooks()
{
    HRESULT hr = S_OK;

    // Set the FunctionIDMapper
    if (m_methodTable.FUNCTIONIDMAPPER != NULL)
    {
        MUST_PASS(PINFO->SetFunctionIDMapper(&FunctionIDMapperStub));
    }

    // Set the FunctionIDMapper2
    if (m_methodTable.FUNCTIONIDMAPPER2 != NULL)
    {
        // pass the pointer to the profiler instance for the second argument

        MUST_PASS(PINFO->SetFunctionIDMapper2(&FunctionIDMapper2Stub, m_methodTable.TEST_POINTER));
    }

    // Set Enter/Leave/Tailcall hooks if callbacks were requested
    if (m_methodTable.FUNCTIONENTER != NULL ||
        m_methodTable.FUNCTIONLEAVE != NULL ||
        m_methodTable.FUNCTIONTAILCALL != NULL)
    {
        MUST_PASS(PINFO->SetEnterLeaveFunctionHooks(m_methodTable.FUNCTIONENTER == NULL ? NULL : &EnterStub,
                                                    m_methodTable.FUNCTIONLEAVE == NULL ? NULL : &LeaveStub,
                                                    m_methodTable.FUNCTIONTAILCALL == NULL ? NULL : &TailcallStub));
    }

    if (m_methodTable.FUNCTIONENTER2 != NULL ||
        m_methodTable.FUNCTIONLEAVE2 != NULL ||
        m_methodTable.FUNCTIONTAILCALL2 != NULL)
    {
        bool fFastEnter2 = FALSE;
        bool fFastLeave2 = FALSE;
        bool fFastTail2 = FALSE;

        FunctionEnter2 *pFE2 = NULL;
        FunctionLeave2 *pFL2 = NULL;
        FunctionTailcall2 *pFTC2 = NULL;

        m_methodTable.FLAGS &(COR_PRF_ENABLE_STACK_SNAPSHOT | COR_PRF_ENABLE_FRAME_INFO | COR_PRF_ENABLE_FUNCTION_ARGS) ? fFastEnter2 = FALSE : fFastEnter2 = TRUE;
        m_methodTable.FLAGS &(COR_PRF_ENABLE_STACK_SNAPSHOT | COR_PRF_ENABLE_FRAME_INFO | COR_PRF_ENABLE_FUNCTION_RETVAL) ? fFastLeave2 = FALSE : fFastLeave2 = TRUE;
        m_methodTable.FLAGS &(COR_PRF_ENABLE_STACK_SNAPSHOT | COR_PRF_ENABLE_FRAME_INFO) ? fFastTail2 = FALSE : fFastTail2 = TRUE;

        pFE2 = reinterpret_cast<FunctionEnter2 *>(&Enter2Stub);
        pFL2 = reinterpret_cast<FunctionLeave2 *>(&Leave2Stub);
        pFTC2 = reinterpret_cast<FunctionTailcall2 *>(&Tailcall2Stub);

        MUST_PASS(PINFO->SetEnterLeaveFunctionHooks2(m_methodTable.FUNCTIONENTER2 ? pFE2 : NULL,
                                                     m_methodTable.FUNCTIONLEAVE2 ? pFL2 : NULL,
                                                     m_methodTable.FUNCTIONTAILCALL2 ? pFTC2 : NULL));
    }
    if (m_methodTable.FUNCTIONENTER3 != NULL ||
        m_methodTable.FUNCTIONLEAVE3 != NULL ||
        m_methodTable.FUNCTIONTAILCALL3 != NULL)
    {
        FunctionEnter3 *pFE3 = reinterpret_cast<FunctionEnter3 *>(&FunctionEnter3Naked);
        FunctionLeave3 *pFL3 = reinterpret_cast<FunctionLeave3 *>(&FunctionLeave3Naked);
        FunctionTailcall3 *pFTC3 = reinterpret_cast<FunctionTailcall3 *>(&FunctionTailCall3Naked);
        MUST_PASS(PINFO->SetEnterLeaveFunctionHooks3(m_methodTable.FUNCTIONENTER3 ? pFE3 : NULL,
                                                     m_methodTable.FUNCTIONLEAVE3 ? pFL3 : NULL,
                                                     m_methodTable.FUNCTIONTAILCALL3 ? pFTC3 : NULL));
    }

    if (m_methodTable.FUNCTIONENTER3WITHINFO != NULL ||
        m_methodTable.FUNCTIONLEAVE3WITHINFO != NULL ||
        m_methodTable.FUNCTIONTAILCALL3WITHINFO != NULL)
    {
        FunctionEnter3WithInfo *pFEWI3 = reinterpret_cast<FunctionEnter3WithInfo *>(&FunctionEnter3WithInfoStub);
        FunctionLeave3WithInfo *pFLWI3 = reinterpret_cast<FunctionLeave3WithInfo *>(&FunctionLeave3WithInfoStub);
        FunctionTailcall3WithInfo *pFTCWI3 = reinterpret_cast<FunctionTailcall3WithInfo *>(&FunctionTailCall3WithInfoStub);
        MUST_PASS(PINFO->SetEnterLeaveFunctionHooks3WithInfo(m_methodTable.FUNCTIONENTER3WITHINFO ? pFEWI3 : NULL,
                                                             m_methodTable.FUNCTIONLEAVE3WITHINFO ? pFLWI3 : NULL,
                                                             m_methodTable.FUNCTIONTAILCALL3WITHINFO ? pFTCWI3 : NULL));
    }

    return S_OK;
}

HRESULT TestProfiler::Initialize(IUnknown *pProfilerInfoUnk)
{
    HRESULT hr = CommonInit(pProfilerInfoUnk);
    if (hr != S_OK)
    {
        TestProfiler::This = NULL;
        DISPLAY(L"TestProfiler::Initialize exits with" << HEX(hr));
        return E_FAIL;
    }

    // Set the ELT hooks and FunctionID mapper
    hr = SetELTHooks();

    return hr;
}

HRESULT STDMETHODCALLTYPE TestProfiler::InitializeForAttach(IUnknown *pCorProfilerInfoUnk,
                                                            void *pvClientData,
                                                            UINT cbClientData)
{
    HRESULT hr = CommonInit(pCorProfilerInfoUnk);
    if (hr != S_OK)
    {
        TestProfiler::This = NULL;
        //if PPRFCOM is allocated by BPD then delete it
        if (m_methodTable.IPPRFCOM == NULL)
        {
            if (PPRFCOM != NULL)
            {
                delete PPRFCOM;
                PPRFCOM = NULL;
            }
        }
        // Can't use DISPLAY because we just freed PPRFCOM
        printf("TestProfiler::InitializeForAttach exits with %x\n", hr);
        return E_FAIL;
    }

    // Set the ELT hooks and FunctionID mapper
    hr = SetELTHooks();

    return hr;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ProfilerDetachSucceeded()
{
    printf("ProfilerDetachSucceeded!");
    ++(PPRFCOM->m_ProfilerDetachSucceeded);
    return VerificationRoutine();
}

HRESULT STDMETHODCALLTYPE TestProfiler::Shutdown()
{
    // Wait for any in progress ELT hooks to finish
    ShutdownGuard::WaitForInProgressHooks();

    HRESULT hr = S_OK;
    ++(PPRFCOM->m_Shutdown);
    hr = VerificationRoutine();

    m_methodTable.FUNCTIONIDMAPPER = NULL;
    m_methodTable.FUNCTIONIDMAPPER2 = NULL;
    m_methodTable.FUNCTIONENTER = NULL;
    m_methodTable.FUNCTIONENTER2 = NULL;
    m_methodTable.FUNCTIONENTER3 = NULL;
    m_methodTable.FUNCTIONENTER3WITHINFO = NULL;
    m_methodTable.VERIFY = NULL;

    m_methodTable.APPDOMAINCREATIONSTARTED = NULL;
    m_methodTable.APPDOMAINCREATIONFINISHED = NULL;
    m_methodTable.APPDOMAINSHUTDOWNSTARTED = NULL;
    m_methodTable.APPDOMAINSHUTDOWNFINISHED = NULL;
    m_methodTable.ASSEMBLYLOADSTARTED = NULL;
    m_methodTable.ASSEMBLYLOADFINISHED = NULL;
    m_methodTable.ASSEMBLYUNLOADSTARTED = NULL;
    m_methodTable.ASSEMBLYUNLOADFINISHED = NULL;
    m_methodTable.MODULELOADSTARTED = NULL;
    m_methodTable.MODULELOADFINISHED = NULL;
    m_methodTable.MODULEUNLOADSTARTED = NULL;
    m_methodTable.MODULEUNLOADFINISHED = NULL;
    m_methodTable.MODULEATTACHEDTOASSEMBLY = NULL;
    m_methodTable.CLASSLOADSTARTED = NULL;
    m_methodTable.CLASSLOADFINISHED = NULL;
    m_methodTable.CLASSUNLOADSTARTED = NULL;
    m_methodTable.CLASSUNLOADFINISHED = NULL;
    m_methodTable.FUNCTIONUNLOADSTARTED = NULL;
    m_methodTable.JITCOMPILATIONSTARTED = NULL;
    m_methodTable.JITCOMPILATIONFINISHED = NULL;
    m_methodTable.JITCACHEDFUNCTIONSEARCHSTARTED = NULL;
    m_methodTable.JITCACHEDFUNCTIONSEARCHFINISHED = NULL;
    m_methodTable.JITFUNCTIONPITCHED = NULL;
    m_methodTable.JITINLINING = NULL;
    m_methodTable.THREADCREATED = NULL;
    m_methodTable.THREADDESTROYED = NULL;
    m_methodTable.THREADASSIGNEDTOOSTHREAD = NULL;
    m_methodTable.THREADNAMECHANGED = NULL;
    m_methodTable.REMOTINGCLIENTINVOCATIONSTARTED = NULL;
    m_methodTable.REMOTINGCLIENTSENDINGMESSAGE = NULL;
    m_methodTable.REMOTINGCLIENTRECEIVINGREPLY = NULL;
    m_methodTable.REMOTINGCLIENTINVOCATIONFINISHED = NULL;
    m_methodTable.REMOTINGSERVERRECEIVINGMESSAGE = NULL;
    m_methodTable.REMOTINGSERVERINVOCATIONSTARTED = NULL;
    m_methodTable.REMOTINGSERVERINVOCATIONRETURNED = NULL;
    m_methodTable.REMOTINGSERVERSENDINGREPLY = NULL;
    m_methodTable.UNMANAGEDTOMANAGEDTRANSITION = NULL;
    m_methodTable.MANAGEDTOUNMANAGEDTRANSITION = NULL;
    m_methodTable.RUNTIMESUSPENDSTARTED = NULL;
    m_methodTable.RUNTIMESUSPENDFINISHED = NULL;
    m_methodTable.RUNTIMESUSPENDABORTED = NULL;
    m_methodTable.RUNTIMERESUMESTARTED = NULL;
    m_methodTable.RUNTIMERESUMEFINISHED = NULL;
    m_methodTable.RUNTIMETHREADSUSPENDED = NULL;
    m_methodTable.RUNTIMETHREADRESUMED = NULL;
    m_methodTable.MOVEDREFERENCES = NULL;
    m_methodTable.MOVEDREFERENCES2 = NULL;
    m_methodTable.OBJECTALLOCATED = NULL;
    m_methodTable.OBJECTSALLOCATEDBYCLASS = NULL;
    m_methodTable.OBJECTREFERENCES = NULL;
    m_methodTable.ROOTREFERENCES = NULL;
    m_methodTable.EXCEPTIONTHROWN = NULL;
    m_methodTable.EXCEPTIONSEARCHFUNCTIONENTER = NULL;
    m_methodTable.EXCEPTIONSEARCHFUNCTIONLEAVE = NULL;
    m_methodTable.EXCEPTIONSEARCHFILTERENTER = NULL;
    m_methodTable.EXCEPTIONSEARCHFILTERLEAVE = NULL;
    m_methodTable.EXCEPTIONSEARCHCATCHERFOUND = NULL;
    m_methodTable.EXCEPTIONOSHANDLERENTER = NULL;
    m_methodTable.EXCEPTIONOSHANDLERLEAVE = NULL;
    m_methodTable.EXCEPTIONUNWINDFUNCTIONENTER = NULL;
    m_methodTable.EXCEPTIONUNWINDFUNCTIONLEAVE = NULL;
    m_methodTable.EXCEPTIONUNWINDFINALLYENTER = NULL;
    m_methodTable.EXCEPTIONUNWINDFINALLYLEAVE = NULL;
    m_methodTable.EXCEPTIONCATCHERENTER = NULL;
    m_methodTable.EXCEPTIONCATCHERLEAVE = NULL;
    m_methodTable.EXCEPTIONCLRCATCHERFOUND = NULL;
    m_methodTable.EXCEPTIONCLRCATCHEREXECUTE = NULL;
    m_methodTable.COMCLASSICVTABLECREATED = NULL;
    m_methodTable.COMCLASSICVTABLEDESTROYED = NULL;
    m_methodTable.GARBAGECOLLECTIONSTARTED = NULL;
    m_methodTable.GARBAGECOLLECTIONFINISHED = NULL;
    m_methodTable.FINALIZEABLEOBJECTQUEUED = NULL;
    m_methodTable.ROOTREFERENCES2 = NULL;
    m_methodTable.HANDLECREATED = NULL;
    m_methodTable.HANDLEDESTROYED = NULL;
    m_methodTable.SURVIVINGREFERENCES = NULL;
    m_methodTable.SURVIVINGREFERENCES2 = NULL;
    m_methodTable.PROFILERATTACHCOMPLETE = NULL;
    m_methodTable.PROFILERDETACHSUCCEEDED = NULL;
    m_methodTable.FUNCTIONENTER = NULL;
    m_methodTable.FUNCTIONLEAVE = NULL;
    m_methodTable.FUNCTIONTAILCALL = NULL;
    m_methodTable.FUNCTIONENTER2 = NULL;
    m_methodTable.FUNCTIONLEAVE2 = NULL;
    m_methodTable.FUNCTIONTAILCALL2 = NULL;
    m_methodTable.FUNCTIONENTER3 = NULL;
    m_methodTable.FUNCTIONLEAVE3 = NULL;
    m_methodTable.FUNCTIONTAILCALL3 = NULL;
    m_methodTable.FUNCTIONENTER3WITHINFO = NULL;
    m_methodTable.FUNCTIONLEAVE3WITHINFO = NULL;
    m_methodTable.FUNCTIONTAILCALL3WITHINFO = NULL;

    delete PPRFCOM;

    return hr;
}

HRESULT TestProfiler::VerificationRoutine()
{
    HRESULT hr = S_OK;

    DISPLAY(L"********************");
    DISPLAY(L"********************");

    DISPLAY(L"API m_Startup                                 " << PPRFCOM->m_Startup);
    DISPLAY(L"API m_Shutdown                                " << PPRFCOM->m_Shutdown);
    DISPLAY(L"API m_ProfilerAttachComplete                  " << PPRFCOM->m_ProfilerAttachComplete);
    DISPLAY(L"API m_ProfilerDetachSucceeded                 " << PPRFCOM->m_ProfilerDetachSucceeded);

    // threads
    DISPLAY(L"API m_ThreadCreated                           " << PPRFCOM->m_ThreadCreated);
    DISPLAY(L"API m_ThreadDestroyed                         " << PPRFCOM->m_ThreadDestroyed);
    DISPLAY(L"API m_ThreadAssignedToOSThread                " << PPRFCOM->m_ThreadAssignedToOSThread);
    DISPLAY(L"API m_ThreadNameChanged                       " << PPRFCOM->m_ThreadNameChanged);

    // appdomains
    DISPLAY(L"API m_AppDomainCreationStarted                " << PPRFCOM->m_AppDomainCreationStarted);
    DISPLAY(L"API m_AppDomainCreationFinished               " << PPRFCOM->m_AppDomainCreationFinished);
    DISPLAY(L"API m_AppDomainShutdownStarted                " << PPRFCOM->m_AppDomainShutdownStarted);
    DISPLAY(L"API m_AppDomainShutdownFinished               " << PPRFCOM->m_AppDomainShutdownFinished);

    // assemblies
    DISPLAY(L"API m_AssemblyLoadStarted                     " << PPRFCOM->m_AssemblyLoadStarted);
    DISPLAY(L"API m_AssemblyLoadFinished                    " << PPRFCOM->m_AssemblyLoadFinished);
    DISPLAY(L"API m_AssemblyUnloadStarted                   " << PPRFCOM->m_AssemblyUnloadStarted);
    DISPLAY(L"API m_AssemblyUnloadFinished                  " << PPRFCOM->m_AssemblyUnloadFinished);

    // modules
    DISPLAY(L"API m_ModuleLoadStarted                       " << PPRFCOM->m_ModuleLoadStarted);
    DISPLAY(L"API m_ModuleLoadFinished                      " << PPRFCOM->m_ModuleLoadFinished);
    DISPLAY(L"API m_ModuleUnloadStarted                     " << PPRFCOM->m_ModuleUnloadStarted);
    DISPLAY(L"API m_ModuleUnloadFinished                    " << PPRFCOM->m_ModuleUnloadFinished);
    DISPLAY(L"API m_ModuleAttachedToAssembly                " << PPRFCOM->m_ModuleAttachedToAssembly);

    // classes
    DISPLAY(L"API m_ClassLoadStarted                        " << PPRFCOM->m_ClassLoadStarted);
    DISPLAY(L"API m_ClassLoadFinished                       " << PPRFCOM->m_ClassLoadFinished);
    DISPLAY(L"API m_ClassUnloadStarted                      " << PPRFCOM->m_ClassUnloadStarted);
    DISPLAY(L"API m_ClassUnloadFinished                     " << PPRFCOM->m_ClassUnloadFinished);
    DISPLAY(L"API m_FunctionUnloadStarted                   " << PPRFCOM->m_FunctionUnloadStarted);

    // JIT
    DISPLAY(L"API m_JITCompilationStarted                   " << PPRFCOM->m_JITCompilationStarted);
    DISPLAY(L"API m_JITCompilationFinished                  " << PPRFCOM->m_JITCompilationFinished);
    DISPLAY(L"API m_JITCachedFunctionSearchStarted          " << PPRFCOM->m_JITCachedFunctionSearchStarted);
    DISPLAY(L"API m_JITCachedFunctionSearchFinished         " << PPRFCOM->m_JITCachedFunctionSearchFinished);
    DISPLAY(L"API m_JITFunctionPitched                      " << PPRFCOM->m_JITFunctionPitched);
    DISPLAY(L"API m_JITInlining                             " << PPRFCOM->m_JITInlining);

    // exceptions
    DISPLAY(L"API m_ExceptionThrown                         " << PPRFCOM->m_ExceptionThrown);

    DISPLAY(L"API m_ExceptionSearchFunctionEnter            " << PPRFCOM->m_ExceptionSearchFunctionEnter);
    DISPLAY(L"API m_ExceptionSearchFunctionLeave            " << PPRFCOM->m_ExceptionSearchFunctionLeave);
    DISPLAY(L"API m_ExceptionSearchFilterEnter              " << PPRFCOM->m_ExceptionSearchFilterEnter);
    DISPLAY(L"API m_ExceptionSearchFilterLeave              " << PPRFCOM->m_ExceptionSearchFilterLeave);

    DISPLAY(L"API m_ExceptionSearchCatcherFound             " << PPRFCOM->m_ExceptionSearchCatcherFound);
    DISPLAY(L"API m_ExceptionCLRCatcherFound                " << PPRFCOM->m_ExceptionCLRCatcherFound);
    DISPLAY(L"API m_ExceptionCLRCatcherExecute              " << PPRFCOM->m_ExceptionCLRCatcherExecute);

    DISPLAY(L"API m_ExceptionOSHandlerEnter                 " << PPRFCOM->m_ExceptionOSHandlerEnter);
    DISPLAY(L"API m_ExceptionOSHandlerLeave                 " << PPRFCOM->m_ExceptionOSHandlerLeave);

    DISPLAY(L"API m_ExceptionUnwindFunctionEnter            " << PPRFCOM->m_ExceptionUnwindFunctionEnter);
    DISPLAY(L"API m_ExceptionUnwindFunctionLeave            " << PPRFCOM->m_ExceptionUnwindFunctionLeave);
    DISPLAY(L"API m_ExceptionUnwindFinallyEnter             " << PPRFCOM->m_ExceptionUnwindFinallyEnter);
    DISPLAY(L"API m_ExceptionUnwindFinallyLeave             " << PPRFCOM->m_ExceptionUnwindFinallyLeave);
    DISPLAY(L"API m_ExceptionCatcherEnter                   " << PPRFCOM->m_ExceptionCatcherEnter);
    DISPLAY(L"API m_ExceptionCatcherLeave                   " << PPRFCOM->m_ExceptionCatcherLeave);

    // transitions
    DISPLAY(L"API m_ManagedToUnmanagedTransition            " << PPRFCOM->m_ManagedToUnmanagedTransition);
    DISPLAY(L"API m_UnmanagedToManagedTransition            " << PPRFCOM->m_UnmanagedToManagedTransition);

    // ccw
    DISPLAY(L"API m_COMClassicVTableCreated                 " << PPRFCOM->m_COMClassicVTableCreated);
    DISPLAY(L"API m_COMClassicVTableDestroyed               " << PPRFCOM->m_COMClassicVTableDestroyed);

    // suspend
    DISPLAY(L"API m_RuntimeSuspendStarted                   " << PPRFCOM->m_RuntimeSuspendStarted);
    DISPLAY(L"API m_RuntimeSuspendFinished                  " << PPRFCOM->m_RuntimeSuspendFinished);
    DISPLAY(L"API m_RuntimeSuspendAborted                   " << PPRFCOM->m_RuntimeSuspendAborted);
    DISPLAY(L"API m_RuntimeResumeStarted                    " << PPRFCOM->m_RuntimeResumeStarted);
    DISPLAY(L"API m_RuntimeResumeFinished                   " << PPRFCOM->m_RuntimeResumeFinished);
    DISPLAY(L"API m_RuntimeThreadSuspended                  " << PPRFCOM->m_RuntimeThreadSuspended);
    DISPLAY(L"API m_RuntimeThreadResumed                    " << PPRFCOM->m_RuntimeThreadResumed);

    // gc
    DISPLAY(L"API m_MovedReferences                         " << PPRFCOM->m_MovedReferences);
    DISPLAY(L"API m_ObjectAllocated                         " << PPRFCOM->m_ObjectAllocated);
    DISPLAY(L"API m_ObjectReferences                        " << PPRFCOM->m_ObjectReferences);
    DISPLAY(L"API m_RootReferences                          " << PPRFCOM->m_RootReferences);
    DISPLAY(L"API m_ObjectsAllocatedByClass                 " << PPRFCOM->m_ObjectsAllocatedByClass);

    // remoting
    DISPLAY(L"API m_RemotingClientInvocationStarted         " << PPRFCOM->m_RemotingClientInvocationStarted);
    DISPLAY(L"API m_RemotingClientInvocationFinished        " << PPRFCOM->m_RemotingClientInvocationFinished);
    DISPLAY(L"API m_RemotingClientSendingMessage            " << PPRFCOM->m_RemotingClientSendingMessage);
    DISPLAY(L"API m_RemotingClientReceivingReply            " << PPRFCOM->m_RemotingClientReceivingReply);

    DISPLAY(L"API m_RemotingServerInvocationStarted         " << PPRFCOM->m_RemotingServerInvocationStarted);
    DISPLAY(L"API m_RemotingServerInvocationReturned        " << PPRFCOM->m_RemotingServerInvocationReturned);
    DISPLAY(L"API m_RemotingServerSendingReply              " << PPRFCOM->m_RemotingServerSendingReply);
    DISPLAY(L"API m_RemotingServerReceivingMessage          " << PPRFCOM->m_RemotingServerReceivingMessage);

    // suspension counter array
    //DWORD m_dwSuspensionCounters[(DWORD)COR_PRF_SUSPEND_FOR_GC_PREP+1]);
    DISPLAY(L"API m_ForceGCEventCounter                     " << PPRFCOM->m_ForceGCEventCounter);
    DISPLAY(L"API m_dwForceGCSucceeded                      " << PPRFCOM->m_dwForceGCSucceeded);

    // enter-leave counters
    DISPLAY(L"API m_FunctionEnter                           " << PPRFCOM->m_FunctionEnter);
    DISPLAY(L"API m_FunctionLeave                           " << PPRFCOM->m_FunctionLeave);
    DISPLAY(L"API m_FunctionTailcall                        " << PPRFCOM->m_FunctionTailcall);

    DISPLAY(L"API m_FunctionEnter2                          " << PPRFCOM->m_FunctionEnter2);
    DISPLAY(L"API m_FunctionLeave2                          " << PPRFCOM->m_FunctionLeave2);
    DISPLAY(L"API m_FunctionTailcall2                       " << PPRFCOM->m_FunctionTailcall2);

    DISPLAY(L"API m_FunctionEnter3WithInfo                  " << PPRFCOM->m_FunctionEnter3WithInfo);    // counter for m_FunctionEnter3WithInfo (slowpath)
    DISPLAY(L"API m_FunctionLeave3WithInfo                  " << PPRFCOM->m_FunctionLeave3WithInfo);    // counter for m_FunctionLeave3WithInfo (slowpath)
    DISPLAY(L"API m_FunctionTailCall3WithInfo               " << PPRFCOM->m_FunctionTailCall3WithInfo); // counter for m_FunctionTailcall3WithInfo (slowpath)

    DISPLAY(L"API m_FunctionEnter3                          " << PPRFCOM->m_FunctionEnter3);    // counter for m_FunctionEnter3 (fastpath)
    DISPLAY(L"API m_FunctionLeave3                          " << PPRFCOM->m_FunctionLeave3);    // counter for m_FunctionLeave3 (fastpath)
    DISPLAY(L"API m_FunctionTailCall3                       " << PPRFCOM->m_FunctionTailCall3); // counter for m_FunctionTailcall3 (fastpath)

    DISPLAY(L"API m_FunctionIDMapper                        " << PPRFCOM->m_FunctionIDMapper);  // TODO we dont track this/use this. Do we need it?
    DISPLAY(L"API m_FunctionIDMapper2                       " << PPRFCOM->m_FunctionIDMapper2); // TODO do we need a counter for this. See m_FunctionMapper for relevance.

    DISPLAY(L"API m_GarbageCollectionStarted                " << PPRFCOM->m_GarbageCollectionStarted);
    DISPLAY(L"API m_GarbageCollectionFinished               " << PPRFCOM->m_GarbageCollectionFinished);
    DISPLAY(L"API m_FinalizeableObjectQueued                " << PPRFCOM->m_FinalizeableObjectQueued);
    DISPLAY(L"API m_RootReferences2                         " << PPRFCOM->m_RootReferences2);
    DISPLAY(L"API m_HandleCreated                           " << PPRFCOM->m_HandleCreated);
    DISPLAY(L"API m_HandleDestroyed                         " << PPRFCOM->m_HandleDestroyed);
    DISPLAY(L"API m_SurvivingReferences                     " << PPRFCOM->m_SurvivingReferences);

    DISPLAY(L"********************");
    DISPLAY(L"********************\n\n");

    // Call the verification function
    if (m_methodTable.VERIFY != NULL)
    {
        if (m_methodTable.VERIFY(PPRFCOM) != S_OK)
        {
            DISPLAY(L"");
            DISPLAY(L"TEST FAILED (profiler returned failure HR from VERIFY)");
            hr = E_FAIL;
        }
        else if (PPRFCOM->m_ulError > 0)
        {
            DISPLAY(L"");
            DISPLAY(L"TEST FAILED (m_ulError > 0)");
            DISPLAY(L"Search \"ERROR: Test Failure\" in the log file for details.");
            hr = E_FAIL;
        }
        else
        {
            DISPLAY(L"");
            DISPLAY(L"TEST PASSED");
        }
    }

    return hr;
}

BOOL TestProfiler::LoadSatelliteModule(wstring target, wstring strTestName)
{
    std::transform(target.begin(), target.end(), target.begin(), ::towlower);

    DISPLAY("Loading test module " << target);

    memset(&m_methodTable, '\0', sizeof(MODULEMETHODTABLE));

    if (target == L"cee")
    {
        CEE_Satellite_Initialize(PPRFCOM, &m_methodTable, strTestName);
    }
    else if (target == L"jit")
    {
        JIT_Satellite_Initialize(PPRFCOM, &m_methodTable, strTestName);
    }
    else if (target == L"rejit")
    {
        ReJIT_Satellite_Initialize(PPRFCOM, &m_methodTable, strTestName);
    }
    else if (target == L"gc")
    {
        GC_Satellite_Initialize(PPRFCOM, &m_methodTable, strTestName);
    }
    else if (target == L"profilehub")
    {
        ProfileHub_Satellite_Initialize(PPRFCOM, &m_methodTable, strTestName);
    }
    else if (target == L"lts")
    {
        LTS_Satellite_Initialize(PPRFCOM, &m_methodTable, strTestName);
    }
    else if (target == L"regressions")
    {
        Regressions_Satellite_Initialize(PPRFCOM, &m_methodTable, strTestName);
    }
    else if (target == L"unittests")
    {
        UnitTests_Satellite_Initialize(PPRFCOM, &m_methodTable, strTestName);
    }
    else if (target == L"instrumentation")
    {
        Instrumentation_Satellite_Initialize(PPRFCOM, &m_methodTable, strTestName);
    }
    else if (target == L"elt")
    {
        ELT_Satellite_Initialize(PPRFCOM, &m_methodTable, strTestName);
    }
    else if (target == L"injectmetadata")
    {
        InjectMetaData_Satellite_Initialize(PPRFCOM, &m_methodTable, strTestName);
    }
    else if (target == L"tieredcompile")
    {
        Tiered_Compilation_Satellite_Initialize(PPRFCOM, &m_methodTable, strTestName);
    }
#ifdef WINDOWS
    else if (target == L"attachdetach")
    {
        AttachDetach_Satellite_Initialize(PPRFCOM, &m_methodTable, strTestName);
    }
#endif // WINDOWS
    else
    {
        FAILURE(L"Test module [" << target << L"] not recognized!");
    }
  
    // Set EventMask flags returned from this satellite module.
    // TODO What does this even mean? Why are we setting it back to itself? Commenting it here.
    //m_methodTable.FLAGS = m_methodTable.FLAGS;

    // If the satellite implements its own IPrfCom, free our copy and use theirs. They must have already
    // called IPrfCom::FreeOutputFiles() or the test run will be hosed.
    if (m_methodTable.IPPRFCOM != NULL)
    {
        delete PPRFCOM;
        PPRFCOM = m_methodTable.IPPRFCOM;
    }

    if (m_methodTable.TEST_POINTER != NULL)
    {
        PPRFCOM->m_pTestClassInstance = m_methodTable.TEST_POINTER;
    }

    return TRUE;
}

/*******************************************************************************
***
*** ICorProfilerCallback && ICorProfilerCallback2 callbacks below
***
*******************************************************************************/

// Logic of standard ICorProfilerCallback implementation:
//
//  Check if running an attach test without an attached profiler, if so return S_OK.
//  Check if the current test wants to receive this callback.  If so, deliver the callback to the
//      satellite module. When a callback is delivered to a satellite module, a pointer to the current
//      IPrfCom implementation is also given.
//  Finally, return the HR from the satellite module if it executed, else return S_OK.
//
//  Some callbacks also implement debugging logging or synchronization for hijacking tests.

HRESULT STDMETHODCALLTYPE TestProfiler::AppDomainCreationStarted(AppDomainID appDomainId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_AppDomainCreationStarted);
    if (m_methodTable.APPDOMAINCREATIONSTARTED != NULL)
    {
        return m_methodTable.APPDOMAINCREATIONSTARTED(PPRFCOM, appDomainId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::AppDomainCreationFinished(AppDomainID appDomainId,
                                                                  HRESULT hrStatus)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

    PROFILER_WCHAR appDomainNameTemp[STRING_LENGTH];
    PLATFORM_WCHAR appDomainName[STRING_LENGTH];
    ULONG nameLength = 0;
    ProcessID procId = 0;

    MUST_PASS(PINFO->GetAppDomainInfo(appDomainId,
                                      STRING_LENGTH,
                                      &nameLength,
                                      appDomainNameTemp,
                                      &procId));
    ConvertProfilerWCharToPlatformWChar(appDomainName, STRING_LENGTH, appDomainNameTemp, STRING_LENGTH);

    if (wcscmp(appDomainName, L"Silverlight AppDomain") == 0 && m_TestAppdomainId == 0)
    {
        m_TestAppdomainId = appDomainId;
    }

    DISPLAY(L"AppDomainCreation: " << appDomainId << " " << appDomainName);

    ++(PPRFCOM->m_AppDomainCreationFinished);
    if (m_methodTable.APPDOMAINCREATIONFINISHED != NULL)
    {
        return m_methodTable.APPDOMAINCREATIONFINISHED(PPRFCOM, appDomainId, hrStatus);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::AppDomainShutdownStarted(AppDomainID appDomainId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_AppDomainShutdownStarted);
    if (m_methodTable.APPDOMAINSHUTDOWNSTARTED != NULL)
    {
        return m_methodTable.APPDOMAINSHUTDOWNSTARTED(PPRFCOM, appDomainId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::AppDomainShutdownFinished(AppDomainID appDomainId,
                                                                  HRESULT hrStatus)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    lock_guard<recursive_mutex> guard(PPRFCOM->m_criticalSection);

    HRESULT hr = S_OK;

    ++(PPRFCOM->m_AppDomainShutdownFinished);
    if (m_methodTable.APPDOMAINSHUTDOWNFINISHED != NULL)
    {
        hr = m_methodTable.APPDOMAINSHUTDOWNFINISHED(PPRFCOM, appDomainId, hrStatus);
    }

    if (m_TestAppdomainId == appDomainId)
    {
        DISPLAY(L"Browser Silverlight AppDomain ShutdownFinished : " << appDomainId);
        Shutdown();
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE TestProfiler::AssemblyLoadStarted(AssemblyID assemblyId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_AssemblyLoadStarted);
    if (m_methodTable.ASSEMBLYLOADSTARTED != NULL)
    {
        return m_methodTable.ASSEMBLYLOADSTARTED(PPRFCOM, assemblyId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::AssemblyLoadFinished(AssemblyID assemblyId,
                                                             HRESULT hrStatus)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_AssemblyLoadFinished);
    if (m_methodTable.ASSEMBLYLOADFINISHED != NULL)
    {
        return m_methodTable.ASSEMBLYLOADFINISHED(PPRFCOM, assemblyId, hrStatus);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::AssemblyUnloadStarted(AssemblyID assemblyId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_AssemblyUnloadStarted);
    if (m_methodTable.ASSEMBLYUNLOADSTARTED != NULL)
    {
        return m_methodTable.ASSEMBLYUNLOADSTARTED(PPRFCOM, assemblyId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::AssemblyUnloadFinished(AssemblyID assemblyId,
                                                               HRESULT hrStatus)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_AssemblyUnloadFinished);
    if (m_methodTable.ASSEMBLYUNLOADFINISHED != NULL)
    {
        return m_methodTable.ASSEMBLYUNLOADFINISHED(PPRFCOM, assemblyId, hrStatus);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ModuleLoadStarted(ModuleID moduleId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ModuleLoadStarted);
    if (m_methodTable.MODULELOADSTARTED != NULL)
    {
        return m_methodTable.MODULELOADSTARTED(PPRFCOM, moduleId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ModuleLoadFinished(ModuleID moduleId,
                                                           HRESULT hrStatus)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ModuleLoadFinished);
    if (m_methodTable.MODULELOADFINISHED != NULL)
    {
        return m_methodTable.MODULELOADFINISHED(PPRFCOM, moduleId, hrStatus);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ModuleUnloadStarted(ModuleID moduleId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ModuleUnloadStarted);
    if (m_methodTable.MODULEUNLOADSTARTED != NULL)
    {
        return m_methodTable.MODULEUNLOADSTARTED(PPRFCOM, moduleId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ModuleUnloadFinished(ModuleID moduleId,
                                                             HRESULT hrStatus)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ModuleUnloadFinished);
    if (m_methodTable.MODULEUNLOADFINISHED != NULL)
    {
        return m_methodTable.MODULEUNLOADFINISHED(PPRFCOM, moduleId, hrStatus);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ModuleAttachedToAssembly(ModuleID moduleId,
                                                                 AssemblyID AssemblyId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ModuleAttachedToAssembly);
    if (m_methodTable.MODULEATTACHEDTOASSEMBLY != NULL)
    {
        return m_methodTable.MODULEATTACHEDTOASSEMBLY(PPRFCOM, moduleId, AssemblyId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ClassLoadStarted(ClassID classId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ClassLoadStarted);
    if (m_methodTable.CLASSLOADSTARTED != NULL)
    {
        return m_methodTable.CLASSLOADSTARTED(PPRFCOM, classId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ClassLoadFinished(ClassID classId,
                                                          HRESULT hrStatus)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ClassLoadFinished);
    if (m_methodTable.CLASSLOADFINISHED != NULL)
    {
        return m_methodTable.CLASSLOADFINISHED(PPRFCOM, classId, hrStatus);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ClassUnloadStarted(ClassID classId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ClassUnloadStarted);
    if (m_methodTable.CLASSUNLOADSTARTED != NULL)
    {
        return m_methodTable.CLASSUNLOADSTARTED(PPRFCOM, classId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ClassUnloadFinished(ClassID classId,
                                                            HRESULT hrStatus)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ClassUnloadFinished);
    if (m_methodTable.CLASSUNLOADFINISHED != NULL)
    {
        return m_methodTable.CLASSUNLOADFINISHED(PPRFCOM, classId, hrStatus);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::FunctionUnloadStarted(FunctionID functionId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_FunctionUnloadStarted);
    if (m_methodTable.FUNCTIONUNLOADSTARTED != NULL)
    {
        return m_methodTable.FUNCTIONUNLOADSTARTED(PPRFCOM, functionId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::JITCompilationStarted(FunctionID functionId,
                                                              BOOL fIsSafeToBlock)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_JITCompilationStarted);
    if (m_methodTable.JITCOMPILATIONSTARTED != NULL)
    {
        return m_methodTable.JITCOMPILATIONSTARTED(PPRFCOM, functionId, fIsSafeToBlock);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::JITCompilationFinished(FunctionID functionId,
                                                               HRESULT hrStatus,
                                                               BOOL fIsSafeToBlock)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_JITCompilationFinished);
    if (m_methodTable.JITCOMPILATIONFINISHED != NULL)
    {
        return m_methodTable.JITCOMPILATIONFINISHED(PPRFCOM, functionId, hrStatus, fIsSafeToBlock);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::JITCachedFunctionSearchStarted(FunctionID functionId,
                                                                       BOOL *pbUseCachedFunction)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_JITCachedFunctionSearchStarted);
    if (m_methodTable.JITCACHEDFUNCTIONSEARCHSTARTED != NULL)
    {
        return m_methodTable.JITCACHEDFUNCTIONSEARCHSTARTED(PPRFCOM, functionId, pbUseCachedFunction);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::JITCachedFunctionSearchFinished(FunctionID functionId,
                                                                        COR_PRF_JIT_CACHE result)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_JITCachedFunctionSearchFinished);
    if (m_methodTable.JITCACHEDFUNCTIONSEARCHFINISHED != NULL)
    {
        return m_methodTable.JITCACHEDFUNCTIONSEARCHFINISHED(PPRFCOM, functionId, result);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::JITFunctionPitched(FunctionID functionId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_JITFunctionPitched);
    if (m_methodTable.JITFUNCTIONPITCHED != NULL)
    {
        return m_methodTable.JITFUNCTIONPITCHED(PPRFCOM, functionId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::JITInlining(FunctionID callerId,
                                                    FunctionID calleeId,
                                                    BOOL *pfShouldInline)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_JITInlining);
    if (m_methodTable.JITINLINING != NULL)
    {
        return m_methodTable.JITINLINING(PPRFCOM, callerId, calleeId, pfShouldInline);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ThreadCreated(ThreadID threadID)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    AsyncGuard guard(PPRFCOM);

    ++(PPRFCOM->m_ThreadCreated);
    if (m_methodTable.THREADCREATED != NULL)
    {
        return m_methodTable.THREADCREATED(PPRFCOM, threadID);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ThreadDestroyed(ThreadID threadID)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    AsyncGuard guard(PPRFCOM);

    ++(PPRFCOM->m_ThreadDestroyed);
    if (m_methodTable.THREADDESTROYED != NULL)
    {
        return m_methodTable.THREADDESTROYED(PPRFCOM, threadID);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ThreadAssignedToOSThread(ThreadID managedThreadId,
                                                                 DWORD osThreadId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ThreadAssignedToOSThread);
    if (m_methodTable.THREADASSIGNEDTOOSTHREAD != NULL)
    {
        return m_methodTable.THREADASSIGNEDTOOSTHREAD(PPRFCOM, managedThreadId, osThreadId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ThreadNameChanged(ThreadID managedThreadId,
                                                          ULONG cchName,
                                                          WCHAR name[])
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    AsyncGuard guard(PPRFCOM);

    ++(PPRFCOM->m_ThreadNameChanged);
    if (m_methodTable.THREADNAMECHANGED != NULL)
    {
        return m_methodTable.THREADNAMECHANGED(PPRFCOM, managedThreadId, cchName, name);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::RemotingClientInvocationStarted()
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_RemotingClientInvocationStarted);
    if (m_methodTable.REMOTINGCLIENTINVOCATIONSTARTED != NULL)
    {
        return m_methodTable.REMOTINGCLIENTINVOCATIONSTARTED(PPRFCOM);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::RemotingClientSendingMessage(GUID *pCookie,
                                                                     BOOL fIsAsync)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_RemotingClientSendingMessage);
    if (m_methodTable.REMOTINGCLIENTSENDINGMESSAGE != NULL)
    {
        return m_methodTable.REMOTINGCLIENTSENDINGMESSAGE(PPRFCOM, pCookie, fIsAsync);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::RemotingClientReceivingReply(GUID *pCookie,
                                                                     BOOL fIsAsync)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_RemotingClientReceivingReply);
    if (m_methodTable.REMOTINGCLIENTRECEIVINGREPLY != NULL)
    {
        return m_methodTable.REMOTINGCLIENTRECEIVINGREPLY(PPRFCOM, pCookie, fIsAsync);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::RemotingClientInvocationFinished()
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_RemotingClientInvocationFinished);
    if (m_methodTable.REMOTINGCLIENTINVOCATIONFINISHED != NULL)
    {
        return m_methodTable.REMOTINGCLIENTINVOCATIONFINISHED(PPRFCOM);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::RemotingServerReceivingMessage(GUID *pCookie,
                                                                       BOOL fIsAsync)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_RemotingServerReceivingMessage);
    if (m_methodTable.REMOTINGSERVERRECEIVINGMESSAGE != NULL)
    {
        return m_methodTable.REMOTINGSERVERRECEIVINGMESSAGE(PPRFCOM, pCookie, fIsAsync);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::RemotingServerInvocationStarted()
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_RemotingServerInvocationStarted);
    if (m_methodTable.REMOTINGSERVERINVOCATIONSTARTED != NULL)
    {
        return m_methodTable.REMOTINGSERVERINVOCATIONSTARTED(PPRFCOM);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::RemotingServerInvocationReturned()
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_RemotingServerInvocationReturned);
    if (m_methodTable.REMOTINGSERVERINVOCATIONRETURNED != NULL)
    {
        return m_methodTable.REMOTINGSERVERINVOCATIONRETURNED(PPRFCOM);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::RemotingServerSendingReply(GUID *pCookie,
                                                                   BOOL fIsAsync)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_RemotingServerSendingReply);
    if (m_methodTable.REMOTINGSERVERSENDINGREPLY != NULL)
    {
        return m_methodTable.REMOTINGSERVERSENDINGREPLY(PPRFCOM, pCookie, fIsAsync);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::UnmanagedToManagedTransition(FunctionID functionId,
                                                                     COR_PRF_TRANSITION_REASON reason)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_UnmanagedToManagedTransition);
    if (m_methodTable.UNMANAGEDTOMANAGEDTRANSITION != NULL)
    {
        return m_methodTable.UNMANAGEDTOMANAGEDTRANSITION(PPRFCOM, functionId, reason);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ManagedToUnmanagedTransition(FunctionID functionId,
                                                                     COR_PRF_TRANSITION_REASON reason)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ManagedToUnmanagedTransition);
    if (m_methodTable.MANAGEDTOUNMANAGEDTRANSITION != NULL)
    {
        return m_methodTable.MANAGEDTOUNMANAGEDTRANSITION(PPRFCOM, functionId, reason);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::RuntimeSuspendStarted(COR_PRF_SUSPEND_REASON suspendReason)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_RuntimeSuspendStarted);
    if (m_methodTable.RUNTIMESUSPENDSTARTED != NULL)
    {
        return m_methodTable.RUNTIMESUSPENDSTARTED(PPRFCOM, suspendReason);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::RuntimeSuspendFinished()
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_RuntimeSuspendFinished);
    if (m_methodTable.RUNTIMESUSPENDFINISHED != NULL)
    {
        return m_methodTable.RUNTIMESUSPENDFINISHED(PPRFCOM);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::RuntimeSuspendAborted()
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_RuntimeSuspendAborted);
    if (m_methodTable.RUNTIMESUSPENDABORTED != NULL)
    {
        return m_methodTable.RUNTIMESUSPENDABORTED(PPRFCOM);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::RuntimeResumeStarted()
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_RuntimeResumeStarted);
    if (m_methodTable.RUNTIMERESUMESTARTED != NULL)
    {
        return m_methodTable.RUNTIMERESUMESTARTED(PPRFCOM);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::RuntimeResumeFinished()
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_RuntimeResumeFinished);
    if (m_methodTable.RUNTIMERESUMEFINISHED != NULL)
    {
        return m_methodTable.RUNTIMERESUMEFINISHED(PPRFCOM);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::RuntimeThreadSuspended(ThreadID threadID)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_RuntimeThreadSuspended);
    if (m_methodTable.RUNTIMETHREADSUSPENDED != NULL)
    {
        return m_methodTable.RUNTIMETHREADSUSPENDED(PPRFCOM, threadID);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::RuntimeThreadResumed(ThreadID threadID)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_RuntimeThreadResumed);
    if (m_methodTable.RUNTIMETHREADRESUMED != NULL)
    {
        return m_methodTable.RUNTIMETHREADRESUMED(PPRFCOM, threadID);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::MovedReferences(ULONG cMovedObjectIDRanges,
                                                        ObjectID oldObjectIDRangeStart[],
                                                        ObjectID newObjectIDRangeStart[],
                                                        ULONG cObjectIDRangeLength[])
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_MovedReferences);
    if (m_methodTable.MOVEDREFERENCES != NULL)
    {
        return m_methodTable.MOVEDREFERENCES(PPRFCOM, cMovedObjectIDRanges, oldObjectIDRangeStart, newObjectIDRangeStart, cObjectIDRangeLength);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::MovedReferences2(ULONG cMovedObjectIDRanges,
                                                         ObjectID oldObjectIDRangeStart[],
                                                         ObjectID newObjectIDRangeStart[],
                                                         SIZE_T cObjectIDRangeLength[])
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_MovedReferences2);
    if (m_methodTable.MOVEDREFERENCES2 != NULL)
    {
        return m_methodTable.MOVEDREFERENCES2(PPRFCOM, cMovedObjectIDRanges, oldObjectIDRangeStart, newObjectIDRangeStart, cObjectIDRangeLength);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ObjectAllocated(ObjectID objectId,
                                                        ClassID classId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ObjectAllocated);
    if (m_methodTable.OBJECTALLOCATED != NULL)
    {
        return m_methodTable.OBJECTALLOCATED(PPRFCOM, objectId, classId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ObjectsAllocatedByClass(ULONG cClassCount,
                                                                ClassID classIds[],
                                                                ULONG cObjects[])
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ObjectsAllocatedByClass);
    if (m_methodTable.OBJECTSALLOCATEDBYCLASS != NULL)
    {
        return m_methodTable.OBJECTSALLOCATEDBYCLASS(PPRFCOM, cClassCount, classIds, cObjects);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ObjectReferences(ObjectID objectId,
                                                         ClassID classId,
                                                         ULONG cObjectRefs,
                                                         ObjectID objectRefIds[])
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ObjectReferences);
    if (m_methodTable.OBJECTREFERENCES != NULL)
    {
        return m_methodTable.OBJECTREFERENCES(PPRFCOM, objectId, classId, cObjectRefs, objectRefIds);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::RootReferences(ULONG cRootRefs,
                                                       ObjectID rootRefIds[])
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_RootReferences);
    if (m_methodTable.ROOTREFERENCES != NULL)
    {
        return m_methodTable.ROOTREFERENCES(PPRFCOM, cRootRefs, rootRefIds);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ExceptionThrown(ObjectID thrownObjectId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ExceptionThrown);
    if (m_methodTable.EXCEPTIONTHROWN != NULL)
    {
        return m_methodTable.EXCEPTIONTHROWN(PPRFCOM, thrownObjectId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ExceptionSearchFunctionEnter(FunctionID functionId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ExceptionSearchFunctionEnter);
    if (m_methodTable.EXCEPTIONSEARCHFUNCTIONENTER != NULL)
    {
        return m_methodTable.EXCEPTIONSEARCHFUNCTIONENTER(PPRFCOM, functionId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ExceptionSearchFunctionLeave()
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ExceptionSearchFunctionLeave);
    if (m_methodTable.EXCEPTIONSEARCHFUNCTIONLEAVE != NULL)
    {
        return m_methodTable.EXCEPTIONSEARCHFUNCTIONLEAVE(PPRFCOM);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ExceptionSearchFilterEnter(FunctionID functionId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ExceptionSearchFilterEnter);
    if (m_methodTable.EXCEPTIONSEARCHFILTERENTER != NULL)
    {
        return m_methodTable.EXCEPTIONSEARCHFILTERENTER(PPRFCOM, functionId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ExceptionSearchFilterLeave()
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ExceptionSearchFilterLeave);
    if (m_methodTable.EXCEPTIONSEARCHFILTERLEAVE != NULL)
    {
        return m_methodTable.EXCEPTIONSEARCHFILTERLEAVE(PPRFCOM);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ExceptionSearchCatcherFound(FunctionID functionId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ExceptionSearchCatcherFound);
    if (m_methodTable.EXCEPTIONSEARCHCATCHERFOUND != NULL)
    {
        return m_methodTable.EXCEPTIONSEARCHCATCHERFOUND(PPRFCOM, functionId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ExceptionOSHandlerEnter(UINT_PTR unused)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ExceptionOSHandlerEnter);
    if (m_methodTable.EXCEPTIONOSHANDLERENTER != NULL)
    {
        return m_methodTable.EXCEPTIONOSHANDLERENTER(PPRFCOM, unused);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ExceptionOSHandlerLeave(UINT_PTR unused)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ExceptionOSHandlerLeave);
    if (m_methodTable.EXCEPTIONOSHANDLERLEAVE != NULL)
    {
        return m_methodTable.EXCEPTIONOSHANDLERLEAVE(PPRFCOM, unused);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ExceptionUnwindFunctionEnter(FunctionID functionId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ExceptionUnwindFunctionEnter);
    if (m_methodTable.EXCEPTIONUNWINDFUNCTIONENTER != NULL)
    {
        return m_methodTable.EXCEPTIONUNWINDFUNCTIONENTER(PPRFCOM, functionId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ExceptionUnwindFunctionLeave()
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    AsyncGuard guard(PPRFCOM);

    ++(PPRFCOM->m_ExceptionUnwindFunctionLeave);
    if (m_methodTable.EXCEPTIONUNWINDFUNCTIONLEAVE != NULL)
    {
        return m_methodTable.EXCEPTIONUNWINDFUNCTIONLEAVE(PPRFCOM);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ExceptionUnwindFinallyEnter(FunctionID functionId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ExceptionUnwindFinallyEnter);
    if (m_methodTable.EXCEPTIONUNWINDFINALLYENTER != NULL)
    {
        return m_methodTable.EXCEPTIONUNWINDFINALLYENTER(PPRFCOM, functionId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ExceptionUnwindFinallyLeave()
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ExceptionUnwindFinallyLeave);
    if (m_methodTable.EXCEPTIONUNWINDFINALLYLEAVE != NULL)
    {
        return m_methodTable.EXCEPTIONUNWINDFINALLYLEAVE(PPRFCOM);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ExceptionCatcherEnter(FunctionID functionId,
                                                              ObjectID objectId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ExceptionCatcherEnter);
    if (m_methodTable.EXCEPTIONCATCHERENTER != NULL)
    {
        return m_methodTable.EXCEPTIONCATCHERENTER(PPRFCOM, functionId, objectId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ExceptionCatcherLeave()
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ExceptionCatcherLeave);
    if (m_methodTable.EXCEPTIONCATCHERLEAVE != NULL)
    {
        return m_methodTable.EXCEPTIONCATCHERLEAVE(PPRFCOM);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ExceptionCLRCatcherFound()
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ExceptionCLRCatcherFound);
    if (m_methodTable.EXCEPTIONCLRCATCHERFOUND != NULL)
    {
        return m_methodTable.EXCEPTIONCLRCATCHERFOUND(PPRFCOM);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ExceptionCLRCatcherExecute()
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ExceptionCLRCatcherExecute);
    if (m_methodTable.EXCEPTIONCLRCATCHEREXECUTE != NULL)
    {
        return m_methodTable.EXCEPTIONCLRCATCHEREXECUTE(PPRFCOM);
    }

    return S_OK;
}
HRESULT STDMETHODCALLTYPE TestProfiler::COMClassicVTableCreated(ClassID wrappedClassID,
                                                                REFGUID implementedIID,
                                                                VOID *pVTable,
                                                                ULONG cSlots)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_COMClassicVTableCreated);
    if (m_methodTable.COMCLASSICVTABLECREATED != NULL)
    {
        return m_methodTable.COMCLASSICVTABLECREATED(PPRFCOM, wrappedClassID, implementedIID, pVTable, cSlots);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::COMClassicVTableDestroyed(ClassID wrappedClassID,
                                                                  REFGUID implementedIID,
                                                                  VOID *pVTable)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_COMClassicVTableDestroyed);
    if (m_methodTable.COMCLASSICVTABLEDESTROYED != NULL)
    {
        return m_methodTable.COMCLASSICVTABLEDESTROYED(PPRFCOM, wrappedClassID, implementedIID, pVTable);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::GarbageCollectionStarted(int cGenerations,
                                                                 BOOL generationCollected[],
                                                                 COR_PRF_GC_REASON reason)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_GarbageCollectionStarted);
    if (m_methodTable.GARBAGECOLLECTIONSTARTED != NULL)
    {
        return m_methodTable.GARBAGECOLLECTIONSTARTED(PPRFCOM, cGenerations, generationCollected, reason);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::GarbageCollectionFinished()
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_GarbageCollectionFinished);
    if (m_methodTable.GARBAGECOLLECTIONFINISHED != NULL)
    {
        return m_methodTable.GARBAGECOLLECTIONFINISHED(PPRFCOM);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::FinalizeableObjectQueued(DWORD finalizerFlags,
                                                                 ObjectID objectID)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_FinalizeableObjectQueued);
    if (m_methodTable.FINALIZEABLEOBJECTQUEUED != NULL)
    {
        return m_methodTable.FINALIZEABLEOBJECTQUEUED(PPRFCOM, finalizerFlags, objectID);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::RootReferences2(ULONG cRootRefs,
                                                        ObjectID rootRefIds[],
                                                        COR_PRF_GC_ROOT_KIND rootKinds[],
                                                        COR_PRF_GC_ROOT_FLAGS rootFlags[],
                                                        UINT_PTR rootIds[])
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_RootReferences2);
    if (m_methodTable.ROOTREFERENCES2 != NULL)
    {
        return m_methodTable.ROOTREFERENCES2(PPRFCOM, cRootRefs, rootRefIds, rootKinds, rootFlags, rootIds);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::HandleCreated(GCHandleID handleId,
                                                      ObjectID initialObjectId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_HandleCreated);
    if (m_methodTable.HANDLECREATED != NULL)
    {
        return m_methodTable.HANDLECREATED(PPRFCOM, handleId, initialObjectId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::HandleDestroyed(GCHandleID handleId)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_HandleDestroyed);
    if (m_methodTable.HANDLEDESTROYED != NULL)
    {
        return m_methodTable.HANDLEDESTROYED(PPRFCOM, handleId);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::SurvivingReferences(ULONG cSurvivingObjectIDRanges,
                                                            ObjectID objectIDRangeStart[],
                                                            ULONG cObjectIDRangeLength[])
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_SurvivingReferences);
    if (m_methodTable.SURVIVINGREFERENCES != NULL)
    {
        return m_methodTable.SURVIVINGREFERENCES(PPRFCOM, cSurvivingObjectIDRanges, objectIDRangeStart, cObjectIDRangeLength);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::SurvivingReferences2(ULONG cSurvivingObjectIDRanges,
                                                             ObjectID objectIDRangeStart[],
                                                             SIZE_T cObjectIDRangeLength[])
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_SurvivingReferences2);
    if (m_methodTable.SURVIVINGREFERENCES2 != NULL)
    {
        return m_methodTable.SURVIVINGREFERENCES2(PPRFCOM, cSurvivingObjectIDRanges, objectIDRangeStart, cObjectIDRangeLength);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ProfilerAttachComplete()
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    ++(PPRFCOM->m_ProfilerAttachComplete);
    if (m_methodTable.PROFILERATTACHCOMPLETE != NULL)
    {
        return m_methodTable.PROFILERATTACHCOMPLETE(PPRFCOM);
    }

    return S_OK;
}

UINT_PTR TestProfiler::FunctionIDMapper(FunctionID functionId, BOOL *pbHookFunction)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return functionId;
    }

    if (m_methodTable.FUNCTIONIDMAPPER != NULL)
    {
        return m_methodTable.FUNCTIONIDMAPPER(functionId, pbHookFunction);
    }

    return functionId;
}

UINT_PTR TestProfiler::FunctionIDMapper2(FunctionID functionId, void *clientData, BOOL *pbHookFunction)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return functionId;
    }

    if (m_methodTable.FUNCTIONIDMAPPER2 != NULL)
    {
        return m_methodTable.FUNCTIONIDMAPPER2(functionId, clientData, pbHookFunction);
    }

    return functionId;
}

VOID TestProfiler::FunctionEnterCallBack(FunctionID funcID)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return;
    }

    AsyncGuard guard(PPRFCOM);

    ++(PPRFCOM->m_FunctionEnter);
    if (m_methodTable.FUNCTIONENTER != NULL)
    {
        m_methodTable.FUNCTIONENTER(PPRFCOM, funcID);
    }
}

VOID TestProfiler::FunctionLeaveCallBack(FunctionID funcID)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return;
    }

    AsyncGuard guard(PPRFCOM);

    ++(PPRFCOM->m_FunctionLeave);
    if (m_methodTable.FUNCTIONLEAVE != NULL)
    {
        m_methodTable.FUNCTIONLEAVE(PPRFCOM, funcID);
    }
}

VOID TestProfiler::FunctionTailcallCallBack(FunctionID funcID)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return;
    }

    AsyncGuard guard(PPRFCOM);

    // It should be updating m_FunctionTailcall and not m_FunctionTailcall2 since this is ELT1 callback and not ELT2 callback

    ++(PPRFCOM->m_FunctionTailcall);
    if (m_methodTable.FUNCTIONTAILCALL != NULL)
    {
        m_methodTable.FUNCTIONTAILCALL(PPRFCOM, funcID);
    }
}

VOID TestProfiler::FunctionEnter2CallBack(FunctionID funcID,
                                          UINT_PTR mappedFuncID,
                                          COR_PRF_FRAME_INFO frame,
                                          COR_PRF_FUNCTION_ARGUMENT_INFO *argumentInfo)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return;
    }

    AsyncGuard guard(PPRFCOM);

    ++(PPRFCOM->m_FunctionEnter2);
    if (m_methodTable.FUNCTIONENTER2 != NULL)
    {
        m_methodTable.FUNCTIONENTER2(PPRFCOM, funcID, mappedFuncID, frame, argumentInfo);
    }
}

VOID TestProfiler::FunctionLeave2CallBack(FunctionID funcID,
                                          UINT_PTR mappedFuncID,
                                          COR_PRF_FRAME_INFO frame,
                                          COR_PRF_FUNCTION_ARGUMENT_RANGE *retvalRange)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return;
    }

    AsyncGuard guard(PPRFCOM);

    ++(PPRFCOM->m_FunctionLeave2);
    if (m_methodTable.FUNCTIONLEAVE2 != NULL)
    {
        m_methodTable.FUNCTIONLEAVE2(PPRFCOM, funcID, mappedFuncID, frame, retvalRange);
    }
}

VOID TestProfiler::FunctionTailcall2CallBack(FunctionID funcID,
                                             UINT_PTR mappedFuncID,
                                             COR_PRF_FRAME_INFO frame)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return;
    }

    AsyncGuard guard(PPRFCOM);

    ++(PPRFCOM->m_FunctionTailcall2);
    if (m_methodTable.FUNCTIONTAILCALL2 != NULL)
    {
        m_methodTable.FUNCTIONTAILCALL2(PPRFCOM, funcID, mappedFuncID, frame);
    }
}

VOID TestProfiler::FunctionEnter3CallBack(FunctionIDOrClientID functionIDOrClientID)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return;
    }
    
    AsyncGuard guard(PPRFCOM);

    ++(PPRFCOM->m_FunctionEnter3);
    if (m_methodTable.FUNCTIONENTER3 != NULL)
    {
        m_methodTable.FUNCTIONENTER3(PPRFCOM, functionIDOrClientID);
    }
}

VOID TestProfiler::FunctionLeave3CallBack(FunctionIDOrClientID functionIDOrClientID)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return;
    }

    AsyncGuard guard(PPRFCOM);

    ++(PPRFCOM->m_FunctionLeave3);
    if (m_methodTable.FUNCTIONLEAVE3 != NULL)
    {
        m_methodTable.FUNCTIONLEAVE3(PPRFCOM, functionIDOrClientID);
    }
}

VOID TestProfiler::FunctionTailCall3CallBack(FunctionIDOrClientID functionIDOrClientID)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return;
    }

    AsyncGuard guard(PPRFCOM);

    ++(PPRFCOM->m_FunctionTailCall3);
    if (m_methodTable.FUNCTIONTAILCALL3 != NULL)
    {
        m_methodTable.FUNCTIONTAILCALL3(PPRFCOM, functionIDOrClientID);
    }
}

VOID TestProfiler::FunctionEnter3WithInfoCallBack(FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return;
    }

    AsyncGuard guard(PPRFCOM);

    ++(PPRFCOM->m_FunctionEnter3WithInfo);
    if (m_methodTable.FUNCTIONENTER3WITHINFO != NULL)
    {
        m_methodTable.FUNCTIONENTER3WITHINFO(PPRFCOM, functionIDOrClientID, eltInfo);
    }
}

VOID TestProfiler::FunctionLeave3WithInfoCallBack(FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return;
    }

    AsyncGuard guard(PPRFCOM);

    ++(PPRFCOM->m_FunctionLeave3WithInfo);
    if (m_methodTable.FUNCTIONLEAVE3WITHINFO != NULL)
    {
        m_methodTable.FUNCTIONLEAVE3WITHINFO(PPRFCOM, functionIDOrClientID, eltInfo);
    }
}

VOID TestProfiler::FunctionTailCall3WithInfoCallBack(FunctionIDOrClientID functionIDOrClientID, COR_PRF_ELT_INFO eltInfo)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return;
    }

    AsyncGuard guard(PPRFCOM);

    ++(PPRFCOM->m_FunctionTailCall3WithInfo);
    if (m_methodTable.FUNCTIONTAILCALL3WITHINFO != NULL)
    {
        m_methodTable.FUNCTIONTAILCALL3WITHINFO(PPRFCOM, functionIDOrClientID, eltInfo);
    }
}

HRESULT STDMETHODCALLTYPE TestProfiler::ReJITCompilationStarted(
    FunctionID functionId,
    ReJITID rejitId,
    BOOL fIsSafeToBlock)
{
    if (m_methodTable.REJITCOMPILATIONSTARTED != NULL)
        return m_methodTable.REJITCOMPILATIONSTARTED(
            PPRFCOM,
            functionId,
            rejitId,
            fIsSafeToBlock);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::GetReJITParameters(
    ModuleID moduleId,
    mdMethodDef methodId,
    ICorProfilerFunctionControl *pFunctionControl)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    if (m_methodTable.GETREJITPARAMETERS != NULL)
    {
        return m_methodTable.GETREJITPARAMETERS(
            PPRFCOM,
            moduleId,
            methodId,
            pFunctionControl);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ReJITCompilationFinished(
    FunctionID functionId,
    ReJITID rejitId,
    HRESULT hrStatus,
    BOOL fIsSafeToBlock)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    if (m_methodTable.REJITCOMPILATIONFINISHED != NULL)
    {
        return m_methodTable.REJITCOMPILATIONFINISHED(
            PPRFCOM,
            functionId,
            rejitId,
            hrStatus,
            fIsSafeToBlock);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE TestProfiler::ReJITError(
    ModuleID moduleId,
    mdMethodDef methodId,
    FunctionID functionId,
    HRESULT hrStatus)
{
    ShutdownGuard();
    if (ShutdownGuard::HasShutdownStarted())
    {
        return S_OK;
    }

    if (m_methodTable.REJITERROR != NULL)
    {
        return m_methodTable.REJITERROR(
            PPRFCOM,
            moduleId,
            methodId,
            functionId,
            hrStatus);
    }
    
    return S_OK;
}

/***************************************************************************************
********************                                               ********************
********************              DllMain/ClassFactory             ********************
********************                                               ********************
***************************************************************************************/

// <TODO>
//  1.  In previous tests, the verification step was performed from inside the
//      TestProfiler's Shutdown method. However if we request a profiler
//      detach, we never get a shutdown callback notification. Instead we get
//      a ProfilerDetachSucceeded callback. We should modify code so that
//      verification step is called from ProfilerDetachSucceeded callback in case
//      of profiler detach testing.
//  2.  As of now the evacuation phase has not yet been implemented. This means we
//      currently do not get a ProfilerDetachSucceeded callback. Hence I've inserted
//      this ugly hack to call the verification step ourself).
//  3.  At this point, attachdetach.dll (which implements the verification step)
//      is not "guaranteed" to be loaded in the managed app's address space. We
//      cannot make any assumptions about the unload order of the Dlls. However we'll
//      have to live with this ugly hack till DavBr implements the Evacuation phase.
// </TODO>

//extern "C" BOOL WINAPI DllMain( HINSTANCE , DWORD dwReason, LPVOID lpReserved )
//{
//    TestProfiler * pBPD = NULL;
//
//    // save off the instance handle for later use
//    switch ( dwReason )
//    {
//    case DLL_PROCESS_ATTACH:
//        break;
//
//    case DLL_PROCESS_DETACH:
//        // From MSDN:
//        //     If fdwReason is DLL_PROCESS_DETACH, lpvReserved is NULL if FreeLibrary has been called or
//        //     the DLL load failed and non-NULL if the process is terminating.
//        //
//        // If lpReserved == NULL Then:
//        //     We failed loading OR FreeLibrary was called. (Which happens in the normal shutdown case)
//        //     So there's no reason to call shutdown.
//        //
//        // ElseIf lpReserved != NULL Then:
//        //     The process is terminating.
//        //     So make sure TestProfiler::Shutdown() has been called.
//
//        if (lpReserved != NULL)
//        {
//            pBPD = TestProfiler::Instance();
//            if (pBPD != NULL)
//            {
//                if (!pBPD->IsShutdown())
//                    pBPD->Shutdown();
//            }
//        }
//
//        break;
//
//    default:
//        break;
//    }
//
//    return  S_OK;
//} // DllMain

//// Used to determine whether the DLL can be unloaded by OLE
//STDAPI DllCanUnloadNow()
//{
//    // Is this ok?
//    return S_OK;
//}
//
//// Returns a class factory to create an object of the requested type
//STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
//{
//    return E_NOTIMPL;
//}
//
//STDAPI DllRegisterServer()
//{
//    return E_NOTIMPL;
//}
//
//
//STDAPI DllUnregisterServer()
//{
//    return E_NOTIMPL;
//}

HRESULT __stdcall TestProfiler::QueryInterface(const IID &iid, void **ppv)
{
    if (iid == IID_IUnknown)
    {
        printf("In TestProfiler::QueryInterface Matched IUnknown\n");
        *ppv = static_cast<ICorProfilerCallback4 *>(this); // original implementation
    }
    else if (iid == IID_ICorProfilerCallback4)
    {
        printf("In TestProfiler::QueryInterface Matched ICorProfilerCallback4\n");
        *ppv = static_cast<ICorProfilerCallback4 *>(this);
    }
    else if (iid == IID_ICorProfilerCallback3)
    {
        printf("In TestProfiler::QueryInterface Matched ICorProfilerCallback3\n");
        *ppv = static_cast<ICorProfilerCallback3 *>(this);
    }
    else if (iid == IID_ICorProfilerCallback2)
    {
        printf("In TestProfiler::QueryInterface Matched ICorProfilerCallback2\n");
        *ppv = static_cast<ICorProfilerCallback2 *>(this);
    }
    else if (iid == IID_ICorProfilerCallback)
    {
        printf("In TestProfiler::QueryInterface Matched ICorProfilerCallback1\n");
        *ppv = static_cast<ICorProfilerCallback *>(this);
    }
    else
    {
        *ppv = NULL;
        printf("In TestProfiler::QueryInterface, returning E_NOINTERFACE\n");
        return E_NOINTERFACE;
    }
    printf("Leave TestProfiler::QueryInterface (Success)\n");
    reinterpret_cast<IUnknown *>(*ppv)->AddRef();
    return S_OK;
}

ULONG __stdcall TestProfiler::AddRef()
{
    return ++(m_cRef);
}

ULONG __stdcall TestProfiler::Release()
{
    --(m_cRef);
    if (0 == m_cRef)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

// End of File
