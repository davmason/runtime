// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.


#include "ProfilerCommon.h"
#include "LegacyCompat.h"

IPrfCom * g_pPrfCom_LegacyCompat;

 #if defined(WIN32) || defined(WIN64)
VOID OldDisplay(const WCHAR *format, ...)
{
    WCHAR buff[STRING_LENGTH];

    va_list args;
    va_start( args, format );

    _vsnwprintf_s( buff, _countof(buff), _TRUNCATE,  format, args );

    va_end(args);

    _LEGACY_DISPLAY_(buff);
}

VOID OldFailure(const WCHAR *format, ...)
{
    WCHAR buff[STRING_LENGTH];

    va_list args;
    va_start( args, format );

    _vsnwprintf_s( buff, _countof(buff), _TRUNCATE,  format, args );

    va_end(args);

    _LEGACY_FAILURE_(buff);
}
#endif
