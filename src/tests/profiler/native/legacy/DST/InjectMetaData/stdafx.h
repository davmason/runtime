// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER				// Allow use of features specific to Windows 95 and Windows NT 4 or later.
#define WINVER 0x0500		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif

#ifndef _WIN32_WINNT		// Allow use of features specific to Windows NT 4 or later.
#define _WIN32_WINNT 0x0500		// Change this to the appropriate value to target Windows 98 and Windows 2000 or later.
#endif						

#ifndef _WIN32_WINDOWS		// Allow use of features specific to Windows 98 or later.
#define _WIN32_WINDOWS 0x0410 // Change this to the appropriate value to target Windows Me or later.
#endif

#ifndef _WIN32_IE			// Allow use of features specific to IE 4.0 or later.
#define _WIN32_IE 0x0400	// Change this to the appropriate value to target IE 5.0 or later.
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#endif

#define DEFAULT_STRING_LENGTH 1000

#define MDINJECTSOURCE_WITHEXT              L"MDInjectSource.dll"
#define MDINJECTSOURCE_WITHOUTEXT           L"MDInjectSource"
#define MDINJECTTARGET_WITHEXT              L"MDInjectTarget.exe"
#define MDINJECTTARGET_NATIVEIMAGE_WITHEXT  L"MDInjectTarget.ni.exe"

#ifdef WINDOWS
#define INJECTMETADATA_DLL                  L"TestProfiler.dll"
#elif defined(LINUX)
#define INJECTMETADATA_DLL                  L"libTestProfiler.so"
#else
#define INJECTMETADATA_DLL                  L"libTestProfiler.dylib"
#endif


#define MDMASTER_TYPE               L"MDMaster"
#define VERIFY_INJECTASSEMBLYREF    L"Verify_InjectAssemblyRef"
#define MDSOURCESIMPLECLASS         L"MDSourceSimpleClass"
#define MDSOURCEGENERICCLASS        L"MDSourceGenericClass"
#define VERIFY_INJECTTYPES          L"Verify_InjectTypes"
#define VERIFY_INJECTGENERICTYPES   L"Verify_InjectGenericTypes"
#define CONTROLFUNCTION             L"ControlFunction"
#define CALLMETHOD                  L"CallMethod"
#define PRINTHELLOWORLD             L"PrintHelloWorld"

// Windows Header Files:
#include <windows.h>

// TODO: reference additional headers your program requires here
#include <assert.h>
#include <unordered_map>
#include <vector>


#include <assert.h>
#include "ProfilerCommon.h"
#include "Helpers.h"
#include "corhlpr.h"
#include "ILRewriter.h"
#include "InjectMetaData.h"
#include "InjectNewUserDefinedString.h"
#include "SigParser.h"
#include "MetadataSignatureBuilder.h"
#include "AssemblyInjector.h"
#include "InjectTypes.h"
#include "InjectGenericTypes.h"
#include "InjectModuleRef.h"
#include "InjectAssemblyRef.h"
#include "InjectMethodSpec.h"
#include "InjectTypeSpec.h"
#include "InjectExternalCall.h"

#define HEADERSIZE_TINY 1
#define HEADERSIZE_FAT 12
#define HEADERSIZE_TINY_MAX 64

