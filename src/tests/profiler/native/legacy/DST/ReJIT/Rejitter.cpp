#include "ProfilerCommon.h"
#include "ReJITScript.h"

//---------------------------------------------------------------------------------------
// Exports that managed code from ProfilerHelper.dll will P/Invoke into
// 
// NOTE: Must keep these signatures in sync with the DllImports in ProfilerHelper.cs!
//---------------------------------------------------------------------------------------


// TODO: Will fail miserably with in-proc SxS CLRs.  Use something other than a global
// to get to the right SampleCallbackImpl


extern "C" void __cdecl NtvEnteredFunction(
    ModuleID moduleIDCur,
    mdMethodDef mdCur,
    int nVersionCur)
{
    g_pReJITScript->NtvEnteredFunction(moduleIDCur, mdCur, nVersionCur);
}

extern "C" void __cdecl NtvExitedFunction(
    ModuleID moduleIDCur,
    mdMethodDef mdCur,
    int nVersionCur)
{
    g_pReJITScript->NtvExitedFunction(moduleIDCur, mdCur, nVersionCur);
}

extern "C" void __cdecl NtvPreCall(
    mdMethodDef mdCur,
    FunctionID functionIDCur,
    ReJITID rejitIDCur,
    mdMethodDef mdTgt,
    FunctionID functionIDTgt)
{
}


