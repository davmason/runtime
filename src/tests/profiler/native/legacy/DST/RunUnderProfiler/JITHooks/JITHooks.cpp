// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// ======================================================================================
//
// JITHooks.cpp
//
// Implementation of ICorProfilerCallback2.
//
// ======================================================================================

#include "JITHooks.h" 

_COM_SMARTPTR_TYPEDEF(ICorProfilerInfo2, __uuidof(ICorProfilerInfo2));

void STDMETHODCALLTYPE Enter2Stub(FunctionID funcId, UINT_PTR clientData, COR_PRF_FRAME_INFO frameInfo, COR_PRF_FUNCTION_ARGUMENT_INFO *pArgInfo)
{
    return;
}

extern "C"
__declspec(dllexport)
void STDMETHODCALLTYPE Leave2Stub(FunctionID funcId, UINT_PTR clientData, COR_PRF_FRAME_INFO frameInfo, COR_PRF_FUNCTION_ARGUMENT_RANGE *pRetVal)
{
    return;
}

extern "C"
__declspec(dllexport)
void STDMETHODCALLTYPE Tailcall2Stub(FunctionID funcId, UINT_PTR clientData, COR_PRF_FRAME_INFO frameInfo)
{
    return;
}

#if defined(_X86_)

__declspec(naked) void Enter2Naked(FunctionID funcId, UINT_PTR clientData, COR_PRF_FRAME_INFO frame, COR_PRF_FUNCTION_ARGUMENT_INFO *argInfo)
{
    __asm
    {
        // Set up EBP Frame (easy debugging)
        push ebp
        mov ebp, esp

        // Make space for locals.
        sub esp, __LOCAL_SIZE

        // Save all registers.
        pushad

        // Check if there's anything on the FP stack.
        fstsw   ax
        and     ax, 3800h        // Check the top-of-fp-stack bits
        cmp     ax, 0            // If non-zero, we have something to save
        jnz     SaveFPReg
        push    0                // otherwise, mark that there is no float value
        jmp     NoSaveFPReg
SaveFPReg:
        sub     esp, 8            // Make room for the FP value
        fstp    qword ptr [esp] // Copy the FP value to the buffer as a double
        push    1                // mark that a float value is present
NoSaveFPReg:
    }

    Enter2Stub(funcId, clientData, frame, argInfo);

    __asm
    {
        // Now see if we have to restore any floating point registers
        cmp     [esp], 0            // Check the flag
        jz      NoRestoreFPRegs        // If zero, no FP regs
        fld     qword ptr [esp + 4]    // Restore FP regs
        add    esp, 12                // Move ESP past the storage space
        jmp   RestoreFPRegsDone
NoRestoreFPRegs:
        add     esp, 4                // Move ESP past the flag
RestoreFPRegsDone:

        // Restore other registers
        popad

        // Pop off locals
        add esp, __LOCAL_SIZE

        // Restore EBP
        // Turned off in release mode to save cycles
        pop ebp

        // stdcall: Callee cleans up args from stack on return
        ret SIZE funcId + SIZE clientData + SIZE frame + SIZE argInfo
    }
}

__declspec(naked) void Leave2Naked(FunctionID id, UINT_PTR clientData, COR_PRF_FRAME_INFO frame, COR_PRF_FUNCTION_ARGUMENT_RANGE* retvalRange)
{
    __asm
    {
        // Set up EBP Frame (easy debugging)
        push ebp
        mov ebp, esp

        // Make space for locals.
        sub esp, __LOCAL_SIZE

        // Save all registers.
        pushad

        // Check if there's anything on the FP stack.
        fstsw   ax
        and     ax, 3800h        // Check the top-of-fp-stack bits
        cmp     ax, 0            // If non-zero, we have something to save
        jnz     SaveFPReg
        push    0                // otherwise, mark that there is no float value
        jmp     NoSaveFPReg
SaveFPReg:
        sub     esp, 8            // Make room for the FP value
        fstp    qword ptr [esp] // Copy the FP value to the buffer as a double
        push    1                // mark that a float value is present
NoSaveFPReg:
    }

    Leave2Stub(id, clientData, frame, retvalRange);

    __asm
    {
        // Now see if we have to restore any floating point registers
        cmp     [esp], 0            // Check the flag
        jz      NoRestoreFPRegs        // If zero, no FP regs
        fld     qword ptr [esp + 4]    // Restore FP regs
        add    esp, 12                // Move ESP past the storage space
        jmp   RestoreFPRegsDone
NoRestoreFPRegs:
        add     esp, 4                // Move ESP past the flag
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

__declspec(naked) void Tailcall2Naked(FunctionID id, UINT_PTR clientData, COR_PRF_FRAME_INFO frame)
{
    __asm
    {
        // Set up EBP Frame (easy debugging)
        push ebp
        mov ebp, esp

        // Make space for locals.
        sub esp, __LOCAL_SIZE

        // Save all registers.
        pushad

        // Check if there's anything on the FP stack.
        fstsw   ax
        and     ax, 3800h        // Check the top-of-fp-stack bits
        cmp     ax, 0            // If non-zero, we have something to save
        jnz     SaveFPReg
        push    0                // otherwise, mark that there is no float value
        jmp     NoSaveFPReg
SaveFPReg:
        sub     esp, 8            // Make room for the FP value
        fstp    qword ptr [esp] // Copy the FP value to the buffer as a double
        push    1                // mark that a float value is present
NoSaveFPReg:
    }

    Tailcall2Stub(id, clientData, frame);

    __asm
    {
        // Now see if we have to restore any floating point registers
        cmp     [esp], 0            // Check the flag
        jz      NoRestoreFPRegs        // If zero, no FP regs
        fld     qword ptr [esp + 4]    // Restore FP regs
        add    esp, 12                // Move ESP past the storage space
        jmp   RestoreFPRegsDone
NoRestoreFPRegs:
        add     esp, 4                // Move ESP past the flag
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

#elif defined(_IA64_)
// use the stubs directly

#elif defined(_AMD64_) || defined (_ARM_)

    EXTERN_C VOID STDMETHODCALLTYPE Leave2Naked(FunctionID funcId, 
                                                UINT_PTR clientData, 
                                                COR_PRF_FRAME_INFO frameInfo, 
                                                COR_PRF_FUNCTION_ARGUMENT_RANGE *pRetVal);
    
    EXTERN_C VOID STDMETHODCALLTYPE Tailcall2Naked(FunctionID id, 
                                                   UINT_PTR clientData, 
                                                   COR_PRF_FRAME_INFO frame);
#else
void Enter2Naked(FunctionID funcId, UINT_PTR clientData, COR_PRF_FRAME_INFO frame, COR_PRF_FUNCTION_ARGUMENT_INFO *argInfo)
{
    _ASSERTE(!"@TODO: implement EnterNaked for your platform");
}

void Leave2Naked(FunctionID id, UINT_PTR clientData, COR_PRF_FRAME_INFO frame, COR_PRF_FUNCTION_ARGUMENT_RANGE* retvalRange)
{
    _ASSERTE(!"@TODO: implement LeaveNaked for your platform");
}

void Tailcall2Naked(FunctionID id, UINT_PTR clientData, COR_PRF_FRAME_INFO frame)
{
    _ASSERTE(!"@TODO: implement TailcallNaked for your platform");
}

#endif  // _X86_

HRESULT CJITHooks::Initialize(IUnknown * pProfilerInfoUnk)
{
    ICorProfilerInfo2Ptr pICP2 = pProfilerInfoUnk;
    pICP2->SetEventMask(COR_PRF_MONITOR_ENTERLEAVE | 
                        COR_PRF_ENABLE_FUNCTION_RETVAL | 
                        COR_PRF_ENABLE_FUNCTION_ARGS |
                        COR_PRF_ENABLE_FRAME_INFO);
            
    #if defined(_IA64_)

        pICP2->SetEnterLeaveFunctionHooks2(reinterpret_cast<FunctionEnter2 *>(&Enter2Stub),
                                           reinterpret_cast<FunctionLeave2 *>(&Leave2Stub),
                                           reinterpret_cast<FunctionTailcall2 *>(&Tailcall2Stub));
    #elif defined(_AMD64_) || defined(_ARM_)

        pICP2->SetEnterLeaveFunctionHooks2(reinterpret_cast<FunctionEnter2 *>(&Enter2Stub),
                                           reinterpret_cast<FunctionLeave2 *>(&Leave2Naked),
                                           reinterpret_cast<FunctionTailcall2 *>(&Tailcall2Naked));
    #else

        pICP2->SetEnterLeaveFunctionHooks2(reinterpret_cast<FunctionEnter2 *>(&Enter2Naked),
                                           reinterpret_cast<FunctionLeave2 *>(&Leave2Naked),
                                           reinterpret_cast<FunctionTailcall2 *>(&Tailcall2Naked));

    #endif

    return S_OK;
}

/***************************************************************************************
 ********************              DllMain/ClassFactory             ********************
 ***************************************************************************************/

class JITHooksModule : public CAtlDllModuleT< JITHooksModule >
{
public :
};

JITHooksModule _AtlModule;

extern "C" BOOL WINAPI DllMain( HINSTANCE /*hInstance*/, DWORD dwReason, LPVOID lpReserved )
{
    return  _AtlModule.DllMain(dwReason,lpReserved);
} // DllMain


// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow()
{
    return _AtlModule.DllCanUnloadNow();
}

// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _AtlModule.DllGetClassObject(rclsid, riid, ppv);
}

STDAPI DllRegisterServer()
{
    return _AtlModule.DllRegisterServer(FALSE);
}


STDAPI DllUnregisterServer()
{
    return _AtlModule.DllUnregisterServer(FALSE);
}