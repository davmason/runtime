; ==++==
;
;   Copyright (c) Microsoft Corporation.  All rights reserved.
;
; ==--==

include AsmMacros.inc
;include asmconstants.inc

extern  EnterStub:proc
extern  LeaveStub:proc
extern  TailcallStub:proc

SIZEOF_OUTGOING_ARGUMENT_HOMES  equ 8h*4
SIZEOF_FP_ARG_SPILL             equ 10h*4

SIZEOF_STACK_FRAME              equ   SIZEOF_OUTGOING_ARGUMENT_HOMES + \
                                      SIZEOF_FP_ARG_SPILL

OFFSETOF_FP_ARG_SPILL           equ   SIZEOF_OUTGOING_ARGUMENT_HOMES

push_non_arg_registers macro
        push            r11
        push            r10
        push            rax
        endm

pop_non_arg_registers macro
        pop             rax
        pop             r10
        pop             r11
        endm

save_arg_registers macro
        save_reg_postrsp    rcx,    0h
        save_reg_postrsp    rdx,    8h
        save_reg_postrsp    r8,    10h
        save_reg_postrsp    r9,    18h
        
        save_xmm128_postrsp xmm0, OFFSETOF_FP_ARG_SPILL
        save_xmm128_postrsp xmm1, OFFSETOF_FP_ARG_SPILL + 10h
        save_xmm128_postrsp xmm2, OFFSETOF_FP_ARG_SPILL + 20h
        save_xmm128_postrsp xmm3, OFFSETOF_FP_ARG_SPILL + 30h
        endm

restore_arg_registers macro
        ; restore fp argument registers
        movdqa          xmm0, [rsp + OFFSETOF_FP_ARG_SPILL      ]
        movdqa          xmm1, [rsp + OFFSETOF_FP_ARG_SPILL + 10h]
        movdqa          xmm2, [rsp + OFFSETOF_FP_ARG_SPILL + 20h]
        movdqa          xmm3, [rsp + OFFSETOF_FP_ARG_SPILL + 30h]
        
        ; restore integer argument registers
        mov             rcx, [rsp +  0h]
        mov             rdx, [rsp +  8h]
        mov             r8,  [rsp + 10h]
        mov             r9,  [rsp + 18h]
        endm


;EXTERN_C void EnterNaked(FunctionID functionId);
NESTED_ENTRY EnterNaked, _TEXT
        push_non_arg_registers
        alloc_stack     SIZEOF_STACK_FRAME
        
        save_arg_registers
    END_PROLOGUE

        call    EnterStub

        restore_arg_registers
        
        ; Begin epilogue
        add             rsp, SIZEOF_STACK_FRAME
        pop_non_arg_registers
        ret
NESTED_END EnterNaked, _TEXT

;EXTERN_C void LeaveNaked(FunctionID functionId);
NESTED_ENTRY LeaveNaked, _TEXT
        push_non_arg_registers
        alloc_stack     SIZEOF_STACK_FRAME
        
        save_arg_registers
    END_PROLOGUE

        call    LeaveStub

        restore_arg_registers

        ; Begin epilogue
        add             rsp, SIZEOF_STACK_FRAME
        pop_non_arg_registers
        ret
NESTED_END LeaveNaked, _TEXT

;EXTERN_C void TailcallNaked(FunctionID functionId);
NESTED_ENTRY TailcallNaked, _TEXT
        push_non_arg_registers
        alloc_stack     SIZEOF_STACK_FRAME
        
        save_arg_registers
    END_PROLOGUE

        call    TailcallStub

        restore_arg_registers

        ; Begin epilogue
        add             rsp, SIZEOF_STACK_FRAME
        pop_non_arg_registers
        ret
NESTED_END TailcallNaked, _TEXT



        end


