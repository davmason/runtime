; ==++==
;
;   Copyright (c) Microsoft Corporation.  All rights reserved.
;
; ==--==
;
; FILE: asmhelpers.asm
;
; <OWNER>shaney</OWNER> The *Profile* functions
;
; ======================================================================================

include AsmMacros.inc
include asmconstants.inc

extern  FunctionEnter3Stub:proc
extern  FunctionLeave3Stub:proc
extern  FunctionTailCall3Stub:proc


;
;    typedef struct _PROFILE_PLATFORM_SPECIFIC_DATA
;    {
;        FunctionID *functionId; // function ID comes in the r11 register
;        void       *rbp;
;        void       *probersp;
;        void       *ip;
;        void       *profiledRsp;
;        UINT64      rax;
;        LPVOID      hiddenArg;
;        UINT64      flt0;
;        UINT64      flt1;
;        UINT64      flt2;
;        UINT64      flt3;
;        UINT32      flags;
;    } PROFILE_PLATFORM_SPECIFIC_DATA, *PPROFILE_PLATFORM_SPECIFIC_DATA;
;
SIZEOF_PROFILE_PLATFORM_SPECIFIC_DATA   equ 8h*11 + 4h*2    ; includes fudge to make FP_SPILL right
SIZEOF_OUTGOING_ARGUMENT_HOMES          equ 8h*4
SIZEOF_FP_ARG_SPILL                     equ 10h*1

; Need to be careful to keep the stack 16byte aligned here, since we are pushing 3
; arguments that will align the stack and we just want to keep it aligned with our
; SIZEOF_STACK_FRAME

SIZEOF_STACK_FRAME_FUDGE                equ 0h

OFFSETOF_PLATFORM_SPECIFIC_DATA         equ SIZEOF_OUTGOING_ARGUMENT_HOMES

; we'll just spill into the PROFILE_PLATFORM_SPECIFIC_DATA structure
OFFSETOF_FP_ARG_SPILL                   equ SIZEOF_OUTGOING_ARGUMENT_HOMES + \
                                            SIZEOF_PROFILE_PLATFORM_SPECIFIC_DATA + \
                                            SIZEOF_STACK_FRAME_FUDGE

SIZEOF_STACK_FRAME                      equ SIZEOF_OUTGOING_ARGUMENT_HOMES + \
                                            SIZEOF_PROFILE_PLATFORM_SPECIFIC_DATA + \
                                            SIZEOF_MAX_FP_ARG_SPILL + \
                                            SIZEOF_STACK_FRAME_FUDGE

PROFILE_ENTER                           equ 1h
PROFILE_LEAVE                           equ 2h
PROFILE_TAILCALL                        equ 4h

; ***********************************************************
;   NOTE: 
;
;   Register preservation scheme:
;
;       Preserved:  
;           - all non-volatile registers
;           - rax
;           - xmm0
;
;       Not Preserved:
;           - integer argument registers (rcx, rdx, r8, r9)
;           - floating point argument registers (xmm1-3)
;           - volatile integer registers (r10, r11)
;           - volatile floating point registers (xmm4-5)
;
; ***********************************************************

; void JIT_ProfilerEnterLeaveTailcallStub(UINT_PTR ProfilerHandle)

;EXTERN_C void FunctionEnter3Naked(FunctionIDOrClientID functionIDOrClientID);
NESTED_ENTRY FunctionEnter3Naked, _TEXT
        push_nonvol_reg         rax

;       Upon entry : 
;           rcx = clientInfo

        lea                     rax, [rsp + 10h]    ; caller rsp
        mov                     r10, [rax - 8h]     ; return address

        alloc_stack             SIZEOF_STACK_FRAME 

        ; correctness of return value in structure doesn't matter for enter probe

        
        ; setup ProfilePlatformSpecificData structure
        xor                     r8, r8;
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA +  0h], r8     ; r8 is null      -- struct functionId field
        save_reg_postrsp        rbp, OFFSETOF_PLATFORM_SPECIFIC_DATA +    8h          ;                 -- struct rbp field
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 10h], rax    ; caller rsp      -- struct probeRsp field
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 18h], r10    ; return address  -- struct ip field
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 20h], r8     ; r8 is null      -- struct profiledRs field
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 28h], r8     ; r8 is null      -- struct rax field
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 30h], r8     ; r8 is null      -- struct hiddenArg field
        movsd                   real8 ptr [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 38h], xmm0    ;      -- struct flt0 field
        movsd                   real8 ptr [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 40h], xmm1    ;      -- struct flt1 field
        movsd                   real8 ptr [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 48h], xmm2    ;      -- struct flt2 field
        movsd                   real8 ptr [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 50h], xmm3    ;      -- struct flt3 field
        mov                     r10, PROFILE_ENTER
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 58h], r10d   ; flags    ;      -- struct flags field

        ; we need to be able to restore the fp return register
        save_xmm128_postrsp     xmm0, OFFSETOF_FP_ARG_SPILL +  0h
    END_PROLOGUE

        ; rcx already contains the clientInfo
        lea                     rdx, [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA]
        call                    FunctionEnter3Stub
    
        ; restore fp return register
        movdqa                  xmm0, [rsp + OFFSETOF_FP_ARG_SPILL +  0h]
        
        ; begin epilogue
        add                     rsp, SIZEOF_STACK_FRAME
        pop                     rax
        ret
NESTED_END FunctionEnter3Naked, _TEXT

;EXTERN_C void FunctionLeave3Naked(FunctionIDOrClientID functionIDOrClientID);
NESTED_ENTRY FunctionLeave3Naked, _TEXT
        push_nonvol_reg         rax

;       Upon entry : 
;           rcx = clientInfo

        ; need to be careful with rax here because it contains the return value which we want to harvest
        
        lea                     r10, [rsp + 10h]    ; caller rsp
        mov                     r11, [r10 - 8h]     ; return address

        alloc_stack             SIZEOF_STACK_FRAME 

        ; correctness of argument registers in structure doesn't matter for leave probe

        ; setup ProfilePlatformSpecificData structure
        xor                     r8, r8;
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA +  0h], r8     ; r8 is null      -- struct functionId field      
        save_reg_postrsp        rbp, OFFSETOF_PLATFORM_SPECIFIC_DATA +    8h          ;                 -- struct rbp field
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 10h], r10    ; caller rsp      -- struct probeRsp field
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 18h], r11    ; return address  -- struct ip field
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 20h], r8     ; r8 is null      -- struct profiledRs field
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 28h], rax    ; return value    -- struct rax field
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 30h], r8     ; r8 is null      -- struct hiddenArg field
        movsd                   real8 ptr [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 38h], xmm0    ;      -- struct flt0 field
        movsd                   real8 ptr [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 40h], xmm1    ;      -- struct flt1 field
        movsd                   real8 ptr [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 48h], xmm2    ;      -- struct flt2 field
        movsd                   real8 ptr [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 50h], xmm3    ;      -- struct flt3 field
        mov                     r10, PROFILE_LEAVE 
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 58h], r10d   ; flags           -- struct flags field

        ; we need to be able to restore the fp return register
        save_xmm128_postrsp     xmm0, OFFSETOF_FP_ARG_SPILL +  0h
    END_PROLOGUE

        ; rcx already contains the clientInfo
        lea                     rdx, [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA]
        call                    FunctionLeave3Stub
    
        ; restore fp return register
        movdqa                  xmm0, [rsp + OFFSETOF_FP_ARG_SPILL +  0h]
        
        ; begin epilogue
        add                     rsp, SIZEOF_STACK_FRAME
        pop                     rax
        ret
NESTED_END FunctionLeave3Naked, _TEXT

;EXTERN_C void FunctionTailCall3Naked(FunctionIDOrClientID functionIDOrClientID);
NESTED_ENTRY FunctionTailCall3Naked, _TEXT
        push_nonvol_reg         rax

;       Upon entry : 
;           rcx = clientInfo

        lea                     rax, [rsp + 10h]    ; caller rsp
        mov                     r11, [rax - 8h]     ; return address

        alloc_stack             SIZEOF_STACK_FRAME 

        ; correctness of return values and argument registers in structure 
        ; doesn't matter for tailcall probe


        ; setup ProfilePlatformSpecificData structure
        xor                     r8, r8;
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA +  0h], r8     ; r8 is null      -- struct functionId field
        save_reg_postrsp        rbp, OFFSETOF_PLATFORM_SPECIFIC_DATA +    8h          ;                 -- struct rbp field
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 10h], rax    ; caller rsp      -- struct probeRsp field 
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 18h], r11    ; return address  -- struct ip field
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 20h], r8     ; r8 is null      -- struct profiledRs field
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 28h], r8     ; r8 is null      -- struct rax field
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 30h], r8     ; r8 is null      -- struct hiddenArg field 
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 38h], r8     ; r8 is null      -- struct flt0 field
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 40h], r8     ; r8 is null      -- struct flt1 field 
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 48h], r8     ; r8 is null      -- struct flt2 field
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 50h], r8     ; r8 is null      -- struct flt3 field
        mov                     r10, PROFILE_TAILCALL
        mov                     [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA + 58h], r10d   ; flags           -- struct flags field

        ; we need to be able to restore the fp return register
        save_xmm128_postrsp     xmm0, OFFSETOF_FP_ARG_SPILL +  0h
    END_PROLOGUE

        ; rcx already contains the clientInfo
        lea                     rdx, [rsp + OFFSETOF_PLATFORM_SPECIFIC_DATA]
        call                    FunctionTailCall3Stub

        ; restore fp return register
        movdqa                  xmm0, [rsp + OFFSETOF_FP_ARG_SPILL +  0h]
        
        ; begin epilogue
        add                     rsp, SIZEOF_STACK_FRAME
        pop                     rax
        ret
NESTED_END FunctionTailCall3Naked, _TEXT



        end

