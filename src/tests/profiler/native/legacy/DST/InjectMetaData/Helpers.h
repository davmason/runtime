#pragma once

#define SUPPRESS_WARNING_4127   \
    __pragma(warning(push))     \
    __pragma(warning(disable:4127)) /* conditional expression is constant*/

#define POP_WARNING_STATE       \
    __pragma(warning(pop))

#define WHILE_0             \
    SUPPRESS_WARNING_4127   \
        while(0)                \
    POP_WARNING_STATE       \

#undef IfFailRet
#undef IfFailRetHR
#undef IfNullRetOOM

#ifdef WINDOWS
#define IfFailRet(expr) do { HRESULT __hr = 0; if (FAILED(__hr = (expr))) { DebugBreak(); return __hr; } } WHILE_0
#else
#define IfFailRet(expr) do { HRESULT __hr = 0; if (FAILED(__hr = (expr))) { return __hr; } } WHILE_0
#endif

#define IfFailRetHR(expr, _customHR)   do { HRESULT __hr; if (FAILED(__hr = (expr))) { return _customHR; } } WHILE_0
#define IfNullRetOOM(expr) do { if ((expr) == NULL) { return E_OUTOFMEMORY; } } WHILE_0