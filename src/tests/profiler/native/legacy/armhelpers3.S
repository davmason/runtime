// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// ==++==
//

//
// ==--==
#include "unixasmmacros.inc"
#include "unixasmmacrosarm.inc"
#include "armasmconstants.h"

.syntax unified
.thumb

// LPVOID __stdcall GetCurrentIP(void)//
    LEAF_ENTRY GetCurrentIP, _TEXT
        mov     r0, lr
        bx      lr
    LEAF_END GetCurrentIP, _TEXT

// LPVOID __stdcall GetCurrentSP(void)//
    LEAF_ENTRY GetCurrentSP, _TEXT
        mov     r0, sp
        bx      lr
    LEAF_END GetCurrentSP, _TEXT

//
// EXTERN_C void FunctionEnter3Naked(FunctionIDOrClientID functionIDOrClientID);
//
NESTED_ENTRY FunctionEnter3Naked, _TEXT, NoHandler
    PROLOG_PUSH "{r4, r5, r7, r11, lr}"
    PROLOG_STACK_SAVE_OFFSET r7, #8

    // fields of PLATFORM_SPECIFIC_DATA, in reverse order

    // UINT32      r0;         // Keep r0 & r1 contiguous to make returning 64-bit results easier
    // UINT32      r1;
    // void       *R11;
    // void       *Pc;
    // union                   // Float arg registers as 32-bit (s0-s15) and 64-bit (d0-d7)
    // {
    //     UINT32  s[16];
    //     UINT64  d[8];
    // };
    // FunctionID  functionId;
    // void       *probeSp;    // stack pointer of managed function 
    // void       *profiledSp; // location of arguments on stack
    // LPVOID      hiddenArg;
    // UINT32      flags;
    movw        r4, #1
    push        { /* flags      */ r4 }
    movw        r4, #0
    push        { /* hiddenArg  */ r4 }
    add         r5, r11, #8
    push        { /* profiledSp */ r5 }
    add         r5, sp, #32
    push        { /* probeSp    */ r5 }
    push        { /* functionId */ r0 }
    vpush.64    { d0 - d7 }
    push        { lr }
    push        { r11 }
    push        { /* return value, r4 is NULL */ r4 }
    push        { /* return value, r4 is NULL */ r4 }
    mov         r1, sp
    bl          C_FUNC(FunctionEnter3Stub)
    EPILOG_STACK_RESTORE_OFFSET r7, #8
    EPILOG_POP "{r4, r5, r7, r11, pc}"
NESTED_END FunctionEnter3Naked, _TEXT

//
// EXTERN_C void FunctionLeave3Naked(FunctionIDOrClientID functionIDOrClientID);
//
NESTED_ENTRY FunctionLeave3Naked, _TEXT, NoHandler
    PROLOG_PUSH "{r1, r2, r4, r5, r7, r11, lr}"
    PROLOG_STACK_SAVE_OFFSET r7, #16

    // fields of PLATFORM_SPECIFIC_DATA, in reverse order

    // UINT32      r0;         // Keep r0 & r1 contiguous to make returning 64-bit results easier
    // UINT32      r1;
    // void       *R11;
    // void       *Pc;
    // union                   // Float arg registers as 32-bit (s0-s15) and 64-bit (d0-d7)
    // {
    //     UINT32  s[16];
    //     UINT64  d[8];
    // };
    // FunctionID  functionId;
    // void       *probeSp;    // stack pointer of managed function 
    // void       *profiledSp; // location of arguments on stack
    // LPVOID      hiddenArg;
    // UINT32      flags;
    movw        r4, #2
    push        { /* flags      */ r4 }
    movw        r4, #0
    push        { /* hiddenArg  */ r4 }
    add         r5, r11, #8
    push        { /* profiledSp */ r5 }
    add         r5, sp, #40
    push        { /* probeSp    */ r5 }
    push        { /* functionId */ r0 }
    vpush.64    { d0 - d7 }
    push        { lr }
    push        { r11 }
    push        { r1 }
    push        { r2 }
    mov         r1, sp
    bl          C_FUNC(FunctionLeave3Stub)
    EPILOG_STACK_RESTORE_OFFSET r7, #16
    EPILOG_POP "{r1, r2, r4, r5, r7, r11, pc}"
NESTED_END FunctionLeave3Naked, _TEXT

//
// EXTERN_C void FunctionTailCall3Naked(FunctionIDOrClientID functionIDOrClientID);
//
NESTED_ENTRY FunctionTailCall3Naked, _TEXT, NoHandler
    PROLOG_PUSH "{r1, r2, r4, r5, r7, r11, lr}"
    PROLOG_STACK_SAVE_OFFSET r7, #16

    // fields of PLATFORM_SPECIFIC_DATA, in reverse order

    // UINT32      r0;         // Keep r0 & r1 contiguous to make returning 64-bit results easier
    // UINT32      r1;
    // void       *R11;
    // void       *Pc;
    // union                   // Float arg registers as 32-bit (s0-s15) and 64-bit (d0-d7)
    // {
    //     UINT32  s[16];
    //     UINT64  d[8];
    // };
    // FunctionID  functionId;
    // void       *probeSp;    // stack pointer of managed function 
    // void       *profiledSp; // location of arguments on stack
    // LPVOID      hiddenArg;
    // UINT32      flags;
    movw        r4, #2
    push        { /* flags      */ r4 }
    movw        r4, #0
    push        { /* hiddenArg  */ r4 }
    add         r5, r11, #8
    push        { /* profiledSp */ r5 }
    add         r5, sp, #40
    push        { /* probeSp    */ r5 }
    push        { /* functionId */ r0 }
    vpush.64    { d0 - d7 }
    push        { lr }
    push        { r11 }
    push        { r1 }
    push        { r2 }
    mov         r1, sp
    bl          C_FUNC(FunctionTailCall3Stub)
    EPILOG_STACK_RESTORE_OFFSET r7, #16
    EPILOG_POP "{r1, r2, r4, r5, r7, r11, pc}"
NESTED_END FunctionTailCall3Naked, _TEXT