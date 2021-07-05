#include "ProfilerCommon.h"
extern PMODULEMETHODTABLE pCallTable; 
extern IPrfCom * g_satellite_pPrfCom;

#pragma region init_declarations

    // VSWhidbey Regressions (CLR v2)
    void VSW_104809_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void VSW_215179_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void VSW_240582_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void VSW_256310_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void VSW_261255_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void VSW_261256_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void VSW_261257_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void VSW_309065_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void VSW_455322_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void VSW_517637_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void VSW_522028_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void VSW_576374_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void VSW_578286_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void VSW_579621_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void VSW_589248_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);

    // DevDivBugs Regressions (CLR v4 Arrowhead)
    void DDB_13114_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);
    void DDB_32429_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);

    void DEV11_211123_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);

    // Degubber Test
    void JITDisableOpt_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);

    //GC FinalizerQueue
    void FinalizerQueue_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);

    // Finalizer Thread Callback Test
    void FinalizerThreadCallback_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable);

#pragma endregion


#pragma region legacy_support

    #ifndef _INCLUDE_LEGACY_FUNCTIONS_
    #define _INCLUDE_LEGACY_FUNCTIONS_
    #include "LegacyCompat.h"
    extern IPrfCom* g_pPrfCom_LegacyCompat;
    #endif

#pragma endregion

extern "C" void Regressions_Satellite_Initialize(IPrfCom * pPrfCom, PMODULEMETHODTABLE pModuleMethodTable, const wstring& testName)
{

    #pragma region legacy_prfcom
        g_pPrfCom_LegacyCompat = pPrfCom;
    #pragma endregion

    DISPLAY(L"Initialize Regressions Module, running test : " << testName);

         if (testName == L"VSW_104809") VSW_104809_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"VSW_215179") VSW_215179_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"VSW_240582") VSW_240582_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"VSW_256310") VSW_256310_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"VSW_261255") VSW_261255_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"VSW_261256") VSW_261256_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"VSW_261257") VSW_261257_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"VSW_309065") VSW_309065_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"VSW_455322") VSW_455322_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"VSW_517637") VSW_517637_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"VSW_522028") VSW_522028_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"VSW_576374") VSW_576374_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"VSW_578286") VSW_578286_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"VSW_579621") VSW_579621_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"VSW_589248") VSW_589248_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"DDB_13114")  DDB_13114_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"DDB_32429")  DDB_32429_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"DEV11_211123") DEV11_211123_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"FinalizerQueue") FinalizerQueue_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"FinalizerThreadCallback") FinalizerThreadCallback_Initialize(pPrfCom, pModuleMethodTable);
    else if (testName == L"JITFlags") 
    {
        // For Debugger JIT Flags test.
        DISPLAY(L"Initialize JIT Flags extension\n")
        pModuleMethodTable->FLAGS = COR_PRF_DISABLE_OPTIMIZATIONS;
    }
    
    else FAILURE(L"Test name not recognized! " << testName);

    pCallTable = pModuleMethodTable; 
    g_satellite_pPrfCom = pPrfCom;

    return;
}
