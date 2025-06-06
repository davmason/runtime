// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#ifndef STUBLINKERX86_H_
#define STUBLINKERX86_H_

#include "stublink.h"

class MetaSig;

extern PCODE GetPreStubEntryPoint();

//=======================================================================

#define X86_INSTR_CALL_REL32    0xE8        // call rel32
#define X86_INSTR_CALL_IND      0x15FF      // call dword ptr[addr32]
#define X86_INSTR_CALL_IND_EAX  0x10FF      // call dword ptr[eax]
#define X86_INSTR_JMP_REL32     0xE9        // jmp rel32
#define X86_INSTR_JMP_IND       0x25FF      // jmp dword ptr[addr32]
#define X86_INSTR_JMP_EAX       0xE0FF      // jmp eax
#define X86_INSTR_MOV_EAX_IMM32 0xB8        // mov eax, imm32
#define X86_INSTR_MOV_EAX_ECX_IND 0x018b    // mov eax, [ecx]
#define X86_INSTR_CMP_IND_ECX_IMM32 0x3981  // cmp [ecx], imm32

#define X86_INSTR_MOV_AL        0xB0        // mov al, imm8
#define X86_INSTR_JMP_REL8      0xEB        // jmp short rel8

#define X86_INSTR_NOP           0x90        // nop
#define X86_INSTR_NOP3_1        0x9090      // 1st word of 3-byte nop
#define X86_INSTR_NOP3_3        0x90        // 3rd byte of 3-byte nop
#define X86_INSTR_INT3          0xCC        // int 3
#define X86_INSTR_HLT           0xF4        // hlt

#define X86_INSTR_MOVAPS_R_RM   0x280F      // movaps xmm1, xmm2/mem128
#define X86_INSTR_MOVAPS_RM_R   0x290F      // movaps xmm1/mem128, xmm2
#define X86_INSTR_MOVLPS_R_RM   0x120F      // movlps xmm1, xmm2/mem128
#define X86_INSTR_MOVLPS_RM_R   0x130F      // movlps xmm1/mem128, xmm2
#define X86_INSTR_MOVUPS_R_RM   0x100F      // movups xmm1, xmm2/mem128
#define X86_INSTR_MOVUPS_RM_R   0x110F      // movups xmm1/mem128, xmm2
#define X86_INSTR_XORPS         0x570F      // xorps xmm1, xmm2/mem128

//----------------------------------------------------------------------
// Encodes X86 registers. The numbers are chosen to match Intel's opcode
// encoding.
//----------------------------------------------------------------------
enum X86Reg : UCHAR
{
    kEAX = 0,
    kECX = 1,
    kEDX = 2,
    kEBX = 3,
    // kESP intentionally omitted because of its irregular treatment in MOD/RM
    kEBP = 5,
    kESI = 6,
    kEDI = 7,

#ifdef TARGET_X86
    NumX86Regs = 8,
#endif // TARGET_X86

    kXMM0 = 0,
    kXMM1 = 1,
    kXMM2 = 2,
    kXMM3 = 3,
    kXMM4 = 4,
    kXMM5 = 5,
#if defined(TARGET_AMD64)
    kXMM6 = 6,
    kXMM7 = 7,
    kXMM8 = 8,
    kXMM9 = 9,
    kXMM10 = 10,
    kXMM11 = 11,
    kXMM12 = 12,
    kXMM13 = 13,
    kXMM14 = 14,
    kXMM15 = 15,
    // Integer registers commence here
    kRAX = 0,
    kRCX = 1,
    kRDX = 2,
    kRBX = 3,
    // kRSP intentionally omitted because of its irregular treatment in MOD/RM
    kRBP = 5,
    kRSI = 6,
    kRDI = 7,
    kR8  = 8,
    kR9  = 9,
    kR10 = 10,
    kR11 = 11,
    kR12 = 12,
    kR13 = 13,
    kR14 = 14,
    kR15 = 15,
    NumX86Regs = 16,

#endif // TARGET_AMD64

    // We use "push ecx" instead of "sub esp, sizeof(LPVOID)"
    kDummyPushReg = kECX
};


// Use this only if you are absolutely sure that the instruction format
// handles it. This is not declared as X86Reg so that users are forced
// to add a cast and think about what exactly they are doing.
const int kESP_Unsafe = 4;

//----------------------------------------------------------------------
// Encodes X86 conditional jumps. The numbers are chosen to match
// Intel's opcode encoding.
//----------------------------------------------------------------------
class X86CondCode {
    public:
        enum cc {
            kJA   = 0x7,
            kJAE  = 0x3,
            kJB   = 0x2,
            kJBE  = 0x6,
            kJC   = 0x2,
            kJE   = 0x4,
            kJZ   = 0x4,
            kJG   = 0xf,
            kJGE  = 0xd,
            kJL   = 0xc,
            kJLE  = 0xe,
            kJNA  = 0x6,
            kJNAE = 0x2,
            kJNB  = 0x3,
            kJNBE = 0x7,
            kJNC  = 0x3,
            kJNE  = 0x5,
            kJNG  = 0xe,
            kJNGE = 0xc,
            kJNL  = 0xd,
            kJNLE = 0xf,
            kJNO  = 0x1,
            kJNP  = 0xb,
            kJNS  = 0x9,
            kJNZ  = 0x5,
            kJO   = 0x0,
            kJP   = 0xa,
            kJPE  = 0xa,
            kJPO  = 0xb,
            kJS   = 0x8,
        };
};

//----------------------------------------------------------------------
// StubLinker with extensions for generating X86 code.
//----------------------------------------------------------------------
class StubLinkerCPU : public StubLinker
{
    public:

#ifdef TARGET_AMD64
        enum X86OperandSize
        {
            k32BitOp,
            k64BitOp,
        };
#endif

        VOID X86EmitAddReg(X86Reg reg, INT32 imm32);
        VOID X86EmitAddRegReg(X86Reg destreg, X86Reg srcReg);
        VOID X86EmitSubReg(X86Reg reg, INT32 imm32);
        VOID X86EmitSubRegReg(X86Reg destreg, X86Reg srcReg);

        VOID X86EmitMovRegReg(X86Reg destReg, X86Reg srcReg);
        VOID X86EmitMovSPReg(X86Reg srcReg);
        VOID X86EmitMovRegSP(X86Reg destReg);

        VOID X86EmitPushReg(X86Reg reg);
        VOID X86EmitPopReg(X86Reg reg);
        VOID X86EmitPushRegs(unsigned regSet);
        VOID X86EmitPopRegs(unsigned regSet);
        VOID X86EmitPushImm32(UINT value);
        VOID X86EmitPushImm32(CodeLabel &pTarget);
        VOID X86EmitPushImm8(BYTE value);
        VOID X86EmitPushImmPtr(LPVOID value BIT64_ARG(X86Reg tmpReg = kR10));

        VOID X86EmitCmpRegImm32(X86Reg reg, INT32 imm32); // cmp reg, imm32
        VOID X86EmitCmpRegIndexImm32(X86Reg reg, INT32 offs, INT32 imm32); // cmp [reg+offs], imm32
#ifdef TARGET_AMD64
        VOID X64EmitCmp32RegIndexImm32(X86Reg reg, INT32 offs, INT32 imm32); // cmp dword ptr [reg+offs], imm32

        VOID X64EmitMovXmmXmm(X86Reg destXmmreg, X86Reg srcXmmReg);
        VOID X64EmitMovdqaFromMem(X86Reg Xmmreg, X86Reg baseReg, int32_t ofs = 0);
        VOID X64EmitMovdqaToMem(X86Reg Xmmreg, X86Reg baseReg, int32_t ofs = 0);
        VOID X64EmitMovSDFromMem(X86Reg Xmmreg, X86Reg baseReg, int32_t ofs = 0);
        VOID X64EmitMovSDToMem(X86Reg Xmmreg, X86Reg baseReg, int32_t ofs = 0);
        VOID X64EmitMovSSFromMem(X86Reg Xmmreg, X86Reg baseReg, int32_t ofs = 0);
        VOID X64EmitMovSSToMem(X86Reg Xmmreg, X86Reg baseReg, int32_t ofs = 0);
        VOID X64EmitMovqRegXmm(X86Reg reg, X86Reg Xmmreg);
        VOID X64EmitMovqXmmReg(X86Reg Xmmreg, X86Reg reg);

        VOID X64EmitMovXmmWorker(BYTE prefix, BYTE opcode, X86Reg Xmmreg, X86Reg baseReg, int32_t ofs = 0);
        VOID X64EmitMovqWorker(BYTE opcode, X86Reg Xmmreg, X86Reg reg);
#endif

        VOID X86EmitZeroOutReg(X86Reg reg);
        VOID X86EmitJumpReg(X86Reg reg);

        VOID X86EmitOffsetModRM(BYTE opcode, X86Reg altreg, X86Reg indexreg, int32_t ofs);
        VOID X86EmitOffsetModRmSIB(BYTE opcode, X86Reg opcodeOrReg, X86Reg baseReg, X86Reg indexReg, int32_t scale, int32_t ofs);

        VOID X86EmitNearJump(CodeLabel *pTarget);
        VOID X86EmitCondJump(CodeLabel *pTarget, X86CondCode::cc condcode);
        VOID X86EmitCall(CodeLabel *target, int iArgBytes);
        VOID X86EmitReturn(WORD wArgBytes);

        VOID X86EmitCurrentThreadAllocContextFetch(X86Reg dstreg, unsigned preservedRegSet);

        VOID X86EmitIndexRegLoad(X86Reg dstreg, X86Reg srcreg, int32_t ofs = 0);
        VOID X86EmitIndexRegStore(X86Reg dstreg, int32_t ofs, X86Reg srcreg);

        VOID X86EmitIndexPush(X86Reg srcreg, int32_t ofs);
        VOID X86EmitBaseIndexPush(X86Reg baseReg, X86Reg indexReg, int32_t scale, int32_t ofs);
        VOID X86EmitIndexPop(X86Reg srcreg, int32_t ofs);

        VOID X86EmitSPIndexPush(int32_t ofs);
        VOID X86EmitSubEsp(INT32 imm32);
        VOID X86EmitAddEsp(INT32 imm32);
        VOID X86EmitEspOffset(BYTE opcode,
                              X86Reg altreg,
                              int32_t ofs
                    AMD64_ARG(X86OperandSize OperandSize = k64BitOp)
                              );

        // Emits the most efficient form of the operation:
        //
        //    opcode   altreg, [basereg + scaledreg*scale + ofs]
        //
        // or
        //
        //    opcode   [basereg + scaledreg*scale + ofs], altreg
        //
        // (the opcode determines which comes first.)
        //
        //
        // Limitations:
        //
        //    scale must be 0,1,2,4 or 8.
        //    if scale == 0, scaledreg is ignored.
        //    basereg and altreg may be equal to 4 (ESP) but scaledreg cannot
        //    for some opcodes, "altreg" may actually select an operation
        //      rather than a second register argument.
        //

        VOID X86EmitOp(WORD    opcode,
                       X86Reg  altreg,
                       X86Reg  basereg,
                       int32_t ofs = 0,
                       X86Reg  scaledreg = (X86Reg)0,
                       BYTE    scale = 0
             AMD64_ARG(X86OperandSize OperandSize = k32BitOp)
                       );

#ifdef TARGET_AMD64
        FORCEINLINE
        VOID X86EmitOp(WORD    opcode,
                       X86Reg  altreg,
                       X86Reg  basereg,
                       int32_t ofs,
                       X86OperandSize OperandSize
                       )
        {
            X86EmitOp(opcode, altreg, basereg, ofs, (X86Reg)0, 0, OperandSize);
        }
#endif // TARGET_AMD64

        // Emits
        //
        //    opcode altreg, modrmreg
        //
        // or
        //
        //    opcode modrmreg, altreg
        //
        // (the opcode determines which one comes first)
        //
        // For single-operand opcodes, "altreg" actually selects
        // an operation rather than a register.

        VOID X86EmitR2ROp(WORD opcode,
                          X86Reg altreg,
                          X86Reg modrmreg
                AMD64_ARG(X86OperandSize OperandSize = k64BitOp)
                          );

        VOID X86EmitRegLoad(X86Reg reg, UINT_PTR imm);

        VOID X86_64BitOperands ()
        {
            WRAPPER_NO_CONTRACT;
#ifdef TARGET_AMD64
            Emit8(0x48);
#endif
        }

#ifdef TARGET_X86
        VOID EmitUnboxMethodStub(MethodDesc* pRealMD);
#endif // TARGET_X86
        VOID EmitTailJumpToMethod(MethodDesc *pMD);
#ifdef TARGET_AMD64
        VOID EmitLoadMethodAddressIntoAX(MethodDesc *pMD);
#endif

#if defined(FEATURE_SHARE_GENERIC_CODE)
        VOID EmitInstantiatingMethodStub(MethodDesc* pSharedMD, void* extra);
#endif // FEATURE_SHARE_GENERIC_CODE
        VOID EmitComputedInstantiatingMethodStub(MethodDesc* pSharedMD, struct ShuffleEntry *pShuffleEntryArray, void* extraArg);

        //===========================================================================
        // Emits code to adjust for a static delegate target.
        VOID EmitShuffleThunk(struct ShuffleEntry *pShuffleEntryArray);

#ifdef _DEBUG
        VOID X86EmitDebugTrashReg(X86Reg reg);
#endif

    private:
        VOID X86EmitSubEspWorker(INT32 imm32);

    public:
        static void Init();

};

inline TADDR rel32Decode(/*PTR_INT32*/ TADDR pRel32)
{
    LIMITED_METHOD_CONTRACT;
    SUPPORTS_DAC;
    return pRel32 + 4 + *PTR_INT32(pRel32);
}

void rel32SetInterlocked(/*PINT32*/ PVOID pRel32, /*PINT32*/ PVOID pRel32RW, TADDR target, MethodDesc* pMD);
BOOL rel32SetInterlocked(/*PINT32*/ PVOID pRel32, /*PINT32*/ PVOID pRel32RW, TADDR target, TADDR expected, MethodDesc* pMD);

#endif  // STUBLINKERX86_H_
