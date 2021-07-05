
.intel_syntax noprefix
#include "unixasmmacros.inc"
#include "unixasmmacrosamd64.inc"
#include "unixasmconstants.h"

.equ SIZEOF_OUTGOING_ARGUMENT_HOMES,    0x08*4
.equ SIZEOF_FP_ARG_SPILL,               0x10*4
.equ SIZEOF_STACK_FRAME,                SIZEOF_OUTGOING_ARGUMENT_HOMES + SIZEOF_FP_ARG_SPILL
.equ OFFSETOF_FP_ARG_SPILL,             SIZEOF_OUTGOING_ARGUMENT_HOMES

.macro push_non_arg_registers
        push            r11
        push            r10
        push            rax
.endm

.macro pop_non_arg_registers 
        pop             rax
        pop             r10
        pop             r11
.endm

.macro save_arg_registers
        save_reg_postrsp    rcx,    0x00
        save_reg_postrsp    rdx,    0x08
        save_reg_postrsp    r8,     0x10
        save_reg_postrsp    r9,     0x18
        
        save_xmm128_postrsp xmm0, OFFSETOF_FP_ARG_SPILL
        save_xmm128_postrsp xmm1, OFFSETOF_FP_ARG_SPILL + 0x10
        save_xmm128_postrsp xmm2, OFFSETOF_FP_ARG_SPILL + 0x20
        save_xmm128_postrsp xmm3, OFFSETOF_FP_ARG_SPILL + 0x30
.endm

.macro restore_arg_registers macro
        # restore fp argument registers
        movdqa          xmm0, [rsp + OFFSETOF_FP_ARG_SPILL       ]
        movdqa          xmm1, [rsp + OFFSETOF_FP_ARG_SPILL + 0x10]
        movdqa          xmm2, [rsp + OFFSETOF_FP_ARG_SPILL + 0x20]
        movdqa          xmm3, [rsp + OFFSETOF_FP_ARG_SPILL + 0x30]
        
        # restore integer argument registers
        mov             rcx, [rsp +  0x00]
        mov             rdx, [rsp +  0x08]
        mov             r8,  [rsp +  0x10]
        mov             r9,  [rsp +  0x18]
.endm

NESTED_ENTRY EnterNaked, _TEXT, NoHandler
        push_non_arg_registers
        alloc_stack     SIZEOF_STACK_FRAME
        
        save_arg_registers
    END_PROLOGUE

        call    EXTERNAL_C_FUNC(EnterStub)

        restore_arg_registers
        
        # Begin epilogue
        add             rsp, SIZEOF_STACK_FRAME
        pop_non_arg_registers
        ret
NESTED_END EnterNaked, _TEXT

NESTED_ENTRY LeaveNaked, _TEXT, NoHandler
        push_non_arg_registers
        alloc_stack     SIZEOF_STACK_FRAME
        
        save_arg_registers
    END_PROLOGUE

        call    EXTERNAL_C_FUNC(LeaveStub)

        restore_arg_registers

        # Begin epilogue
        add             rsp, SIZEOF_STACK_FRAME
        pop_non_arg_registers
        ret
NESTED_END LeaveNaked, _TEXT

NESTED_ENTRY TailcallNaked, _TEXT, NoHandler
        push_non_arg_registers
        alloc_stack     SIZEOF_STACK_FRAME
        
        save_arg_registers
    END_PROLOGUE

        call    EXTERNAL_C_FUNC(TailcallStub)

        restore_arg_registers

        # Begin epilogue
        add             rsp, SIZEOF_STACK_FRAME
        pop_non_arg_registers
        ret
NESTED_END TailcallNaked, _TEXT
