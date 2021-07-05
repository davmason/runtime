// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// ProfilerCommon.h
//
// Defines the interface and default implementation of the ProfilerCommon class.  This 
// class contains common routines used by all profiler tests such display and IT to name
// routines.
// 
// ======================================================================================

#ifndef __PROFILER_DRIVER_COMMON__
#define __PROFILER_DRIVER_COMMON__

#pragma region Includes

    // Windows includes
    #ifndef _WIN32_WINNT
    #define _WIN32_WINNT 0x0500
    #include <windows.h>
    #endif

    #ifndef DECLSPEC_EXPORT
    #define DECLSPEC_EXPORT __declspec(dllexport)
    #endif//DECLSPEC_EXPORT

    #ifndef DECLSPEC_IMPORT
    #define DECLSPEC_IMPORT __declspec(dllimport)
    #endif//DECLSPEC_IMPORT

    #ifndef EXTERN_C
    #define EXTERN_C extern "C"
    #endif//EXTERN_C

    #ifndef NAKED 
    #define NAKED __declspec(naked)
    #endif//NAKED

    // Runtime includes
    #include "cor.h"
    #include "corhdr.h"     // Needs to be included before corprof
    #include "corsym.h"     // ISymUnmanagedReader
    #include "corprof.h"    // Definitions for ICorProfilerCallback and friends
    #include "corerror.h"   // For error code names

#pragma endregion // Includes

#define MAX_STRING_LENGTH 1024

#define DISPLAY( message, ... )   {			    			\
		wchar_t buffer[MAX_STRING_LENGTH] = {0};                 \
        swprintf_s(buffer, MAX_STRING_LENGTH, message, __VA_ARGS__); \
		OutputDebugString(buffer);                          \
	}


#define FAILURE( message, ... )   {			    			\
		wchar_t buffer[MAX_STRING_LENGTH] = {0};                 \
        swprintf_s(buffer, MAX_STRING_LENGTH, message, __VA_ARGS__); \
		OutputDebugString(buffer);                          \
		throw buffer;										\
	}


#endif //__PROFILER_DRIVER_COMMON__

