// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// LegacyCompat.h
//
// ProfilerCommon.h must be included before LegacyCompat.h
// 
// This is for backward compatability only, it should not be used in new development.
// 
// ======================================================================================

#ifndef __PROFILER_LEGACY_COMPAT_H__
#define __PROFILER_LEGACY_COMPAT_H__

// Global PrfCom pointer used by static global routines below
extern IPrfCom* g_pPrfCom_LegacyCompat;

// Replacement macros
#define _DISPLAY_OLD_( message ) OldDisplay message ;
#define _FAILURE_OLD_( message ) OldFailure message ;

// Legacy versions of current display macros using global PrfCom pointer
#define _LEGACY_DISPLAY_( message )    {                                                                            \
                                std::wstringstream temp;                                                            \
                                temp << message;                                                                    \
                                g_pPrfCom_LegacyCompat->Display(temp);                                              \
                            }

#define _LEGACY_FAILURE_( message )    {                                                                            \
                                std::wstringstream temp;                                                            \
                                temp << message;                                                                    \
                                g_pPrfCom_LegacyCompat->Error(temp);                                                \
                            }

// Functions to convert C format strings into one buffer to be passed to new output functions.
#ifdef _INCLUDE_LEGACY_FUNCTIONS_
VOID OldDisplay(const WCHAR *format, ...);
VOID OldFailure(const WCHAR *format, ...);

#else
extern VOID OldDisplay(const WCHAR *format, ...);
extern VOID OldFailure(const WCHAR *format, ...);
#endif //_INCLUDE_LEGACY_FUNCTIONS_

#endif // __PROFILER_LEGACY_COMPAT_H__