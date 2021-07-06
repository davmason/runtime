// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#ifndef __IPROFILER_COMMUNICATION_LIBRARY__
#define __IPROFILER_COMMUNICATION_LIBRARY__

// Windows includes
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#include <windows.h>
#endif

#ifndef DECLSPEC_EXPORT
#define DECLSPEC_EXPORT __declspec(dllexport)
#endif//DECLSPEC_EXPORT

// C++ Standard Library
#include <iostream>
using namespace std;
using std::ios;


#ifndef PCL_READ_CALLBACK
typedef LPWSTR (* PCL_READ_CALLBACK)(LPWSTR, PINT32);
#endif // PCL_READ_CALLBACK


class DECLSPEC_NOVTABLE DECLSPEC_EXPORT IProfComLib
{
    public:

        virtual HRESULT STDMETHODCALLTYPE CreatePipe(int) = 0;
        virtual HRESULT STDMETHODCALLTYPE ClosePipe() = 0;
        virtual HRESULT STDMETHODCALLTYPE CleanPipe() = 0;
        virtual HRESULT STDMETHODCALLTYPE WriteTransaction(wstring inString, wstring& outString) = 0;
        virtual HRESULT STDMETHODCALLTYPE Write(wstring inString) = 0; 
        virtual HRESULT STDMETHODCALLTYPE Write(wstring inString, bool forceFlush) = 0; 
        virtual HRESULT STDMETHODCALLTYPE ReadTransaction(PCL_READ_CALLBACK pCallBack) = 0;
        virtual HRESULT STDMETHODCALLTYPE Read(wstring& outString) = 0;

    protected:

        HANDLE hPipe;
        wstring wstrPipeName;
        BOOL isServer;
        PCL_READ_CALLBACK defaultCallback;

    public:

        HANDLE hEvent;
};

class DECLSPEC_NOVTABLE DECLSPEC_EXPORT IProfInitLib
{
    public:

        virtual VOID STDMETHODCALLTYPE ReadInitStruct(PBYTE, DWORD) = 0;

        HANDLE hPipe;
        wstring wstrPipeName;
};


class DECLSPEC_EXPORT ProfComLib: public IProfComLib
{
    public:

        ProfComLib(wstring pName, bool server);

        virtual HRESULT STDMETHODCALLTYPE CreatePipe(int delay = 5);
        virtual HRESULT STDMETHODCALLTYPE ClosePipe();
        virtual HRESULT STDMETHODCALLTYPE CleanPipe();
        virtual HRESULT STDMETHODCALLTYPE WriteTransaction(wstring inString, wstring& outString);
        virtual HRESULT STDMETHODCALLTYPE Write(wstring inString); 
        virtual HRESULT STDMETHODCALLTYPE Write(wstring inString, bool forceFlush); 
        virtual HRESULT STDMETHODCALLTYPE ReadTransaction(PCL_READ_CALLBACK pCallBack);
        virtual HRESULT STDMETHODCALLTYPE Read(wstring& outString);
};

class DECLSPEC_EXPORT ProfInitLib: public IProfInitLib
{
    public:

        ProfInitLib();
        ProfInitLib(LPCWSTR);
        ~ProfInitLib();

        virtual VOID STDMETHODCALLTYPE ReadInitStruct(PBYTE buff, DWORD buffSize);
};


// class used to pass test init info to both launch and attach tests
// No users need to know about this, they can be ignorant.
struct ManagedToNativeProfTestInit
{
    public:
        UINT32 cbExtensionModule;
        UINT32 cbExtensionModuleOffset;
        UINT32 cbTestName;
        UINT32 cbTestNameOffset;
        UINT32 cbOutputFile;
        UINT32 cbOutputFileOffset;
        UINT32 cbBvtRoot;
        UINT32 cbBvtRootOffset;
        UINT32 cbExtRoot;
        UINT32 cbExtRootOffset;
        UINT32 cbSdkRoot;
        UINT32 cbSdkRootOffset;
        UINT32 cbTestTargetName;
        UINT32 cbTestTargetNameOffset;
        UINT32 cbStringLength;
        BOOL   isServer;
        BOOL   multiTarget;
        BOOL   ASPorService;
        BOOL   enableAsynch;
        BOOL   disableErrorDialogs;
        BOOL   ngenRequired;
        BOOL   debugLogging;
        UINT   pipeType;
        UINT   profFlags;
        WCHAR  TestStrings[];
};

struct ProfTestInit
{
    public:

        wstring extensionModule;
        wstring testName;
        wstring testSubArea;
        wstring outputFile;
        wstring bvtRoot;
        wstring extRoot;
        wstring sdkRoot;
        wstring testTargetName;
        BOOL isServer;
        BOOL multiTarget;
        BOOL ASPorService;
        BOOL enableAsynch;
        BOOL disableErrorDialogs;
        BOOL ngenRequired;
        BOOL debugLogging;
        UINT pipeType;
        UINT profFlags;

        ProfTestInit(ManagedToNativeProfTestInit * pMTNPTI)
        {
            extensionModule = wstring(&pMTNPTI->TestStrings[pMTNPTI->cbExtensionModuleOffset], &pMTNPTI->TestStrings[pMTNPTI->cbExtensionModuleOffset + pMTNPTI->cbExtensionModule]);
            testName = wstring(&pMTNPTI->TestStrings[pMTNPTI->cbTestNameOffset], &pMTNPTI->TestStrings[pMTNPTI->cbTestNameOffset + pMTNPTI->cbTestName]);
            outputFile = wstring(&pMTNPTI->TestStrings[pMTNPTI->cbOutputFileOffset], &pMTNPTI->TestStrings[pMTNPTI->cbOutputFileOffset + pMTNPTI->cbOutputFile]);
            bvtRoot = wstring(&pMTNPTI->TestStrings[pMTNPTI->cbBvtRootOffset], &pMTNPTI->TestStrings[pMTNPTI->cbBvtRootOffset + pMTNPTI->cbBvtRoot]);
            extRoot = wstring(&pMTNPTI->TestStrings[pMTNPTI->cbExtRootOffset], &pMTNPTI->TestStrings[pMTNPTI->cbExtRootOffset + pMTNPTI->cbExtRoot]);
            sdkRoot = wstring(&pMTNPTI->TestStrings[pMTNPTI->cbSdkRootOffset], &pMTNPTI->TestStrings[pMTNPTI->cbSdkRootOffset + pMTNPTI->cbSdkRoot]);
            testTargetName = wstring(&pMTNPTI->TestStrings[pMTNPTI->cbTestTargetNameOffset], &pMTNPTI->TestStrings[pMTNPTI->cbTestTargetNameOffset + pMTNPTI->cbTestTargetName]);

            isServer = pMTNPTI->isServer;
            multiTarget = pMTNPTI->multiTarget;
            ASPorService = pMTNPTI->ASPorService;
            enableAsynch = pMTNPTI->enableAsynch;
            disableErrorDialogs = pMTNPTI->disableErrorDialogs;
            ngenRequired = pMTNPTI->ngenRequired;
            debugLogging = pMTNPTI->debugLogging;
            pipeType = pMTNPTI->pipeType;
            profFlags = pMTNPTI->profFlags;
        }
};

#endif __IPROFILER_COMMUNICATION_LIBRARY__