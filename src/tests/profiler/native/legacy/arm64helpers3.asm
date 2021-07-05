#include "ksarm64.h"
#include "arm64asmconstants.h"
#include "arm64asmmacros.h"

    TEXTAREA

 #define SIZEOF__PROFILE_PLATFORM_SPECIFIC_DATA 256

; ------------------------------------------------------------------
    MACRO
    DefineProfilerHelper $HelperName

        GBLS __ProfilerHelperFunc
        GBLS __ProfilerCppFunc
__ProfilerHelperFunc SETS "$HelperName":CC:"Naked"
__ProfilerCppFunc SETS "$HelperName":CC:"Stub"
        IMPORT $__ProfilerCppFunc                  ; The C++ helper which does most of the work

    NESTED_ENTRY $__ProfilerHelperFunc
        ; On entry:
        ;   x10 = functionIDOrClientID
        ;   x11 = profiledSp
        ;   x12 = throwable
        ;
        ; On exit:
        ;   Values of x0-x8, q0-q7, fp are preserved.
        ;   Values of other volatile registers are not preserved.

        PROLOG_SAVE_REG_PAIR fp, lr, -SIZEOF__PROFILE_PLATFORM_SPECIFIC_DATA! ; Allocate space and save Fp, Pc.
        SAVE_ARGUMENT_REGISTERS sp, 16          ; Save x8 and argument registers (x0-x7).
        str     xzr, [sp, #88]                  ; Clear functionId.
        SAVE_FLOAT_ARGUMENT_REGISTERS sp, 96    ; Save floating-point/SIMD registers (q0-q7).
        add     x12, fp, SIZEOF__PROFILE_PLATFORM_SPECIFIC_DATA ; Compute probeSp - initial value of Sp on entry to the helper.
        stp     x12, x11, [sp, #224]            ; Save probeSp, profiledSp.
        str     xzr, [sp, #240]                 ; Clear hiddenArg.

        mov     x0, x10
        mov     x1, sp
        bl $__ProfilerCppFunc

        RESTORE_ARGUMENT_REGISTERS sp, 16       ; Restore x8 and argument registers.
        RESTORE_FLOAT_ARGUMENT_REGISTERS sp, 96 ; Restore floating-point/SIMD registers.

        EPILOG_RESTORE_REG_PAIR fp, lr, SIZEOF__PROFILE_PLATFORM_SPECIFIC_DATA!
        EPILOG_RETURN

    NESTED_END

    MEND

    DefineProfilerHelper FunctionEnter3
    DefineProfilerHelper FunctionLeave3
    DefineProfilerHelper FunctionTailCall3

; Must be at very end of file
    END