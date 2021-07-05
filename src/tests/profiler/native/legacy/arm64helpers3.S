
#include "unixasmmacros.inc"
#include "unixasmmacrosarm64.inc"
#include "arm64asmconstants.h"


// ------------------------------------------------------------------
#define PROFILE_ENTER    1
#define PROFILE_LEAVE    2
#define PROFILE_TAILCALL 4
#define SIZEOF__PROFILE_PLATFORM_SPECIFIC_DATA 256

// ------------------------------------------------------------------
.macro GenerateProfileHelper helper, callee
NESTED_ENTRY \helper, _TEXT, NoHandler
    // On entry:
    //   x10 = functionIDOrClientID
    //   x11 = profiledSp
    //   x12 = throwable
    //
    // On exit:
    //   Values of x0-x8, q0-q7, fp are preserved.
    //   Values of other volatile registers are not preserved.

    PROLOG_SAVE_REG_PAIR_INDEXED fp, lr, -SIZEOF__PROFILE_PLATFORM_SPECIFIC_DATA // Allocate space and save Fp, Pc.
    SAVE_ARGUMENT_REGISTERS sp, 16          // Save x8 and argument registers (x0-x7).
    str     xzr, [sp, 88]                   // Clear functionId.
    SAVE_FLOAT_ARGUMENT_REGISTERS sp, 96    // Save floating-point/SIMD registers (q0-q7).
    add     x12, fp, SIZEOF__PROFILE_PLATFORM_SPECIFIC_DATA // Compute probeSp - initial value of Sp on entry to the helper.
    stp     x12, x11, [sp, 224]             // Save probeSp, profiledSp.
    str     xzr, [sp, 240]                  // Clear hiddenArg.
    // mov     w12, \flags
    // stp     w12, wzr, [sp, 248]             // Save flags and clear unused field.

    mov     x0, x10
    mov     x1, sp
    bl      C_FUNC(\callee)

    RESTORE_ARGUMENT_REGISTERS sp, 16       // Restore x8 and argument registers.
    RESTORE_FLOAT_ARGUMENT_REGISTERS sp, 96 // Restore floating-point/SIMD registers.

    EPILOG_RESTORE_REG_PAIR_INDEXED fp, lr, SIZEOF__PROFILE_PLATFORM_SPECIFIC_DATA
    EPILOG_RETURN

NESTED_END \helper, _TEXT
.endmacro

GenerateProfileHelper FunctionEnter3Naked, FunctionEnter3Stub
GenerateProfileHelper FunctionLeave3Naked, FunctionLeave3Stub
GenerateProfileHelper FunctionTailCall3Naked, FunctionTailCall3Stub

