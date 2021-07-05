;; ==++==
;;
;;   Copyright (c) Microsoft Corporation.  All rights reserved.
;;
;; ==--==
#include "ksarm.h"


    TEXTAREA


; ------------------------------------------------------------------
; Macro used to generate profiler helpers. 
; On exit:
;   All register values are preserved including volatile registers
;
        MACRO
            DefineProfilerHelper $HelperName

        GBLS __ProfilerHelperFunc
        GBLS __ProfilerCppFunc
__ProfilerHelperFunc SETS "$HelperName":CC:"Naked"
__ProfilerCppFunc SETS "$HelperName":CC:"Stub"

        NESTED_ENTRY $__ProfilerHelperFunc

        IMPORT $__ProfilerCppFunc                  ; The C++ helper which does most of the work

        PROLOG_PUSH         {r0-r3,r12,lr}  ; save volatile general purpose registers. remaining r1 & r2 are saved below.
        PROLOG_VPUSH        {d0-d7}         ; Spill floting point argument registers


        bl          $__ProfilerCppFunc 

        EPILOG_VPOP         {d0-d7}
        EPILOG_POP          {r0-r3,r12,lr}

        EPILOG_RETURN

        NESTED_END

        MEND

        DefineProfilerHelper Leave2
        DefineProfilerHelper Tailcall2

        DefineProfilerHelper FunctionEnter3
        DefineProfilerHelper FunctionLeave3
        DefineProfilerHelper FunctionTailCall3


; Must be at very end of file
        END