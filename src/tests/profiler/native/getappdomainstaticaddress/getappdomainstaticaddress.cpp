// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "getappdomainstaticaddress.h"
#include <string>
#include <assert.h>
#include <inttypes.h>
#include <sstream>


using std::thread;
using std::shared_ptr;
using std::map;
using std::make_pair;
using std::mutex;
using std::lock_guard;
using std::wstring;
using std::vector;

// Prints a lot to the console for easier tracking
#define DEBUG_OUT false

GetAppDomainStaticAddress::GetAppDomainStaticAddress() :
    refCount(0),
    failures(0),
    successes(0),
    collectibleCount(0),
    nonCollectibleCount(0),
    jitEventCount(0),
    gcTriggerThread(),
    gcWaitEvent(),
    classADMap(),
    classADMapLock()
{

}

GetAppDomainStaticAddress::~GetAppDomainStaticAddress()
{

}

GUID GetAppDomainStaticAddress::GetClsid()
{
    // {604D76F0-2AF2-48E0-B196-80C972F6AFB7}
    GUID clsid = { 0x604D76F0, 0x2AF2, 0x48E0, {0xB1, 0x96, 0x80, 0xC9, 0x72, 0xF6, 0xAF, 0xB7 } };
    return clsid;
}

HRESULT GetAppDomainStaticAddress::Initialize(IUnknown *pICorProfilerInfoUnk)
{
    LogMessage(U("Initialize profiler!"));

    HRESULT hr = pICorProfilerInfoUnk->QueryInterface(IID_ICorProfilerInfo10, (void**)&pCorProfilerInfo);
    if (hr != S_OK)
    {
        LogFailure(U("Got HR {HR} from QI for ICorProfilerInfo4"), hr);
        ++failures;
        return E_FAIL;
    }

    pCorProfilerInfo->SetEventMask2(COR_PRF_MONITOR_GC |
                                   COR_PRF_MONITOR_CLASS_LOADS |
                                   COR_PRF_MONITOR_MODULE_LOADS |
                                   COR_PRF_MONITOR_JIT_COMPILATION |
                                   COR_PRF_DISABLE_ALL_NGEN_IMAGES, 0);

    auto gcTriggerLambda = [&]()
    {
        pCorProfilerInfo->InitializeCurrentThread();

        while (true)
        {
            gcWaitEvent.Wait();

            if (!IsRuntimeExecutingManagedCode())
            {
                if (DEBUG_OUT)
                {
                    LogMessage(U("Runtime has not started executing managed code yet."));
                }
                continue;
            }

            LogMessage(U("Forcing GC"));
            HRESULT hr = pCorProfilerInfo->ForceGC();
            if (FAILED(hr))
            {
                LogMessage(U("Error forcing GC... hr=0x{HR}"), hr);
                ++failures;
                continue;
            }
        }
    };

    gcTriggerThread = thread(gcTriggerLambda);

    return S_OK;
}

HRESULT GetAppDomainStaticAddress::Shutdown()
{
    Profiler::Shutdown();

    if (this->pCorProfilerInfo != nullptr)
    {
        this->pCorProfilerInfo->Release();
        this->pCorProfilerInfo = nullptr;
    }

    if(failures == 0 && successes > 0 && collectibleCount > 0 && nonCollectibleCount > 0)
    {
        LogMessage(U("PROFILER TEST PASSES"));
    }
    else
    {
        LogMessage(U("Test failed number of failures={INT} successes={INT} collectibleCount={INT} nonCollectibleCount={INT}"),
            failures.load(), successes.load(), collectibleCount.load(), nonCollectibleCount.load());
    }
    fflush(stdout);

    return S_OK;
}

HRESULT GetAppDomainStaticAddress::ModuleLoadFinished(ModuleID moduleId, HRESULT hrStatus)
{
    SHUTDOWNGUARD();

    constexpr size_t nameLen = 1024;
    WCHAR name[nameLen];
    HRESULT hr = pCorProfilerInfo->GetModuleInfo2(moduleId,
                                                 NULL,
                                                nameLen,
                                                NULL,
                                                name,
                                                NULL,
                                                NULL);
    if (FAILED(hr))
    {
        LogFailure(U("GetModuleInfo2 failed with hr={HR}"), hr);
        ++failures;
    }

    LogMessage(U("Module {HEX} ({STR}) loaded"), moduleId, name);
    
    LogMessage(U("Forcing GC due to module load"));
    gcWaitEvent.Signal();
    
    return S_OK;
}

HRESULT GetAppDomainStaticAddress::ModuleUnloadStarted(ModuleID moduleId)
{
    SHUTDOWNGUARD();

    LogMessage(U("Forcing GC due to module unload"));
    gcWaitEvent.Signal();

    {
        lock_guard<mutex> guard(classADMapLock);
        constexpr size_t nameLen = 1024;
        WCHAR name[nameLen];
        HRESULT hr = pCorProfilerInfo->GetModuleInfo2(moduleId,
                                                     NULL,
                                                    nameLen,
                                                    NULL,
                                                    name,
                                                    NULL,
                                                    NULL);
        if (FAILED(hr))
        {
            LogFailure(U("GetModuleInfo2 failed with hr={HR}"), hr);
            ++failures;
            return E_FAIL;
        }


        LogMessage(U("Module {HEX} ({STR}) unload started"), moduleId, name);

        for (auto it = classADMap.begin(); it != classADMap.end(); )
        {
            ClassID classId = it->first;

            ModuleID modId;
            hr = pCorProfilerInfo->GetClassIDInfo(classId, &modId, NULL);
            if (FAILED(hr))
            {
                LogFailure(U("Failed to get ClassIDInfo hr={HR}"), hr);
                ++failures;
                return E_FAIL;
            }

            if (modId == moduleId)
            {
                if (DEBUG_OUT)
                {
                    LogMessage(U("ClassID {HEX} being removed due to parent module unloading"), classId);
                }

                it = classADMap.erase(it);
                continue;
            }

            // Now check the generic arguments
            bool shouldEraseClassId = false;
            vector<ClassID> genericTypes = GetGenericTypeArgs(classId);
            for (auto genericIt = genericTypes.begin(); genericIt != genericTypes.end(); ++genericIt)
            {
                ClassID typeArg = *genericIt;
                ModuleID typeArgModId;

                if (DEBUG_OUT)
                {
                    LogMessage(U("Checking generic argument {HEX} of class {HEX}"), typeArg, classId);
                }

                hr = pCorProfilerInfo->GetClassIDInfo(typeArg, &typeArgModId, NULL);
                if (FAILED(hr))
                {
                    LogFailure(U("Failed to get ClassIDInfo hr={HR}"), hr);
                    ++failures;
                    return E_FAIL;
                }

                if (typeArgModId == moduleId)
                {
                    if (DEBUG_OUT)
                    {
                        LogMessage(U("ClassID {HEX} ({STR}) being removed due to generic argument {HEX} ({STR}) belonging to the parent module {HEX} unloading"),
                                classId, GetClassIDName(classId).ToCStr(), typeArg, GetClassIDName(typeArg).ToCStr(), typeArgModId);
                    }

                    shouldEraseClassId = true;
                    break;
                }
            }

            if (shouldEraseClassId)
            {
                it = classADMap.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    return S_OK;
}

HRESULT GetAppDomainStaticAddress::ClassLoadFinished(ClassID classId, HRESULT hrStatus)
{
    SHUTDOWNGUARD();

    HRESULT hr = S_OK;

    ThreadID threadId = NULL;
    AppDomainID appDomainId = NULL;
    CorElementType baseElemType;
    ClassID        baseClassId;
    ULONG          cRank;

    // We don't care about array classes, so skip them.

    hr = pCorProfilerInfo->IsArrayClass(
        classId,
        &baseElemType,
        &baseClassId,
        &cRank);
    if (hr == S_OK)
    {
        return S_OK;
    }


    hr = pCorProfilerInfo->GetCurrentThreadID(&threadId);
    if (FAILED(hr))
    {
        LogFailure(U("GetCurrentThreadID returned {HR}"), hr);
        ++failures;
        return hr;
    }

    hr = pCorProfilerInfo->GetThreadAppDomain(threadId, &appDomainId);
    if (FAILED(hr))
    {
        LogFailure(U("GetThreadAppDomain returned {HR} for ThreadID {HEX}"), hr, threadId);
        ++failures;
        return hr;
    }

    lock_guard<mutex> guard(classADMapLock);
    classADMap.insert(make_pair(classId, appDomainId));

    ModuleID modId;
    hr = pCorProfilerInfo->GetClassIDInfo2(classId,
                                          &modId,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL,
                                          NULL);
    if (FAILED(hr))
    {
        LogFailure(U("GetClassIDInfo2 returned {HR} for ClassID {HEX}"), hr, classId);
        ++failures;
    }

    wstring name = GetClassIDName(classId).ToWString();

    if (DEBUG_OUT)
    {
        LogMessage(U("Class {HEX} ({STR}) loaded from module {HEX}"), classId, name.c_str(), modId);
    }

    return hr;
}

HRESULT GetAppDomainStaticAddress::ClassUnloadStarted(ClassID classId)
{
    SHUTDOWNGUARD();

    lock_guard<mutex> guard(classADMapLock);

    mdTypeDef unloadClassToken;
    HRESULT hr = pCorProfilerInfo->GetClassIDInfo2(classId,
                                                  NULL,
                                                  &unloadClassToken,
                                                  NULL,
                                                  0,
                                                  NULL,
                                                  NULL);
    if (FAILED(hr))
    {
        LogFailure(U("GetClassIDInfo2 failed with hr={HR}"), hr);
        ++failures;
    }

    if (DEBUG_OUT)
    {
        LogMessage(U("Class {HEX} ({STR}) unload started"), classId, GetClassIDName(classId));
    }

    for (auto it = classADMap.begin(); it != classADMap.end(); ++it)
    {
        ClassID mapClass = it->first;
        mdTypeDef mapClassToken;
        hr = pCorProfilerInfo->GetClassIDInfo2(mapClass,
                                              NULL,
                                              &mapClassToken,
                                              NULL,
                                              0,
                                              NULL,
                                              NULL);
        if (mapClass == classId || mapClassToken == unloadClassToken)
        {
            it = classADMap.erase(it);
        }
    }

    return S_OK;
}

HRESULT GetAppDomainStaticAddress::JITCompilationFinished(FunctionID functionId, HRESULT hrStatus, BOOL fIsSafeToBlock)
{
    SHUTDOWNGUARD();

    ++jitEventCount;
    return S_OK;
}

HRESULT GetAppDomainStaticAddress::GarbageCollectionFinished()
{
    SHUTDOWNGUARD();

    HRESULT hr = S_OK;
    lock_guard<mutex> guard(classADMapLock);

    for (ClassAppDomainMap::iterator iCADM = classADMap.begin();
            iCADM != classADMap.end();
            iCADM++)
    {
        ClassID classId = iCADM->first;
        AppDomainID appDomainId = iCADM->second;

        if (DEBUG_OUT)
        {
            LogMessage(U("Calling GetClassIDInfo2 on classId {HEX}"), classId);
            fflush(stdout);
        }

        ModuleID classModuleId = NULL;
        hr = pCorProfilerInfo->GetClassIDInfo2(classId,
                                    &classModuleId,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);
        if (FAILED(hr))
        {
            LogFailure(U("GetClassIDInfo2 returned {HR} for ClassID{HEX}"), hr, classId);
            ++failures;
            continue;
        }

        COMPtrHolder<IMetaDataImport> pIMDImport;

        hr = pCorProfilerInfo->GetModuleMetaData(classModuleId,
                                        ofRead,
                                        IID_IMetaDataImport,
                                        (IUnknown **)&pIMDImport);
        if (hr == CORPROF_E_DATAINCOMPLETE)
        {
            // Module is being unloaded...
            continue;
        }
        else if (FAILED(hr))
        {
            LogFailure(U("GetModuleMetaData returned {HR} for ModuleID {HEX}"), hr, classModuleId);
            ++failures;
            continue;
        }

        HCORENUM hEnum = NULL;
        mdTypeDef token = NULL;
        mdFieldDef fieldTokens[SHORT_LENGTH];
        ULONG cTokens = NULL;

        if (DEBUG_OUT)
        {
            LogMessage(U("Calling GetClassIDInfo2 (again?) on classId {HEX}"), classId);
            fflush(stdout);
        }

        // Get class token to enum all field    s from MetaData.  (Needed for statics)
        hr = pCorProfilerInfo->GetClassIDInfo2(classId,
                                            NULL,
                                            &token,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL);
        if (hr == CORPROF_E_DATAINCOMPLETE)
        {
            // Class load not complete.  We can not inspect yet.
            continue;
        }
        else if (FAILED(hr))
        {
            LogFailure(U("GetClassIDInfo2returned {HR}"), hr);
            ++failures;
            continue;
        }

        // Enum all fields of the class from the MetaData
        hr = pIMDImport->EnumFields(&hEnum,
                                            token,
                                            fieldTokens,
                                            SHORT_LENGTH,
                                            &cTokens);
        if (FAILED(hr))
        {
            LogFailure(U("IMetaDataImport::EnumFields returned {HR}"), hr);
            ++failures;
            continue;
        }

        for (ULONG i = 0; i < cTokens; i++)
        {
            mdTypeDef fieldClassToken = NULL;
            WCHAR tokenName[256];
            ULONG nameLength = NULL;
            DWORD fieldAttributes = NULL;
            PCCOR_SIGNATURE pvSig = NULL;
            ULONG cbSig = NULL;
            DWORD corElementType = NULL;

            hr = pIMDImport->GetFieldProps(fieldTokens[i],
                                            &fieldClassToken,
                                            tokenName,
                                            256,
                                            &nameLength,
                                            &fieldAttributes,
                                            &pvSig,
                                            &cbSig,
                                            &corElementType,
                                            NULL,
                                            NULL);

            if (FAILED(hr))
            {
                LogFailure(U("GetFieldProps returned {HR} for Field {INT}"), hr, i);
                ++failures;
                continue;
            }

            if ((IsFdStatic(fieldAttributes)) && (!IsFdLiteral(fieldAttributes)))
            {
                COR_PRF_STATIC_TYPE fieldInfo = COR_PRF_FIELD_NOT_A_STATIC;
                hr = pCorProfilerInfo->GetStaticFieldInfo(classId, fieldTokens[i], &fieldInfo);
                if (FAILED(hr))
                {
                    LogFailure(U("GetStaticFieldInfo returned HR={HR} for field {HEX} ({STR})"), hr, fieldTokens[i], tokenName);
                    ++failures;
                    continue;
                }

                if (fieldInfo & COR_PRF_FIELD_APP_DOMAIN_STATIC)
                {
                    PVOID staticOffSet = NULL;

                    if (DEBUG_OUT)
                    {
                        LogMessage(U("Calling GetAppDomainStaticAddress on classId={HEX}"), classId);
                        fflush(stdout);
                    }

                    hr = pCorProfilerInfo->GetAppDomainStaticAddress(classId,
                                                fieldTokens[i],
                                                appDomainId,
                                                &staticOffSet);

                    if (FAILED(hr) && (hr != CORPROF_E_DATAINCOMPLETE))
                    {
                        LogFailure(U("GetAppDomainStaticAddress Failed HR {HR}"), hr);
                        ++failures;
                        continue;
                    }
                    else if (hr != CORPROF_E_DATAINCOMPLETE)
                    {
                        String moduleName = GetModuleIDName(classModuleId);
                        if (EndsWith(moduleName, U("unloadlibrary.dll")))
                        {
                            ++collectibleCount;
                        }
                        else
                        {
                            ++nonCollectibleCount;
                        }
                    }
                }
            }
        }
    }

    LogMessage(U("Garbage collection finished"));
    ++successes;
    return hr;
}

bool GetAppDomainStaticAddress::IsRuntimeExecutingManagedCode()
{
    return jitEventCount.load() > 0;
}

std::vector<ClassID> GetAppDomainStaticAddress::GetGenericTypeArgs(ClassID classId)
{
    HRESULT hr = S_OK;
    constexpr size_t typeIdArgsLen = 10;
    ClassID typeArgs[typeIdArgsLen];
    ULONG32 typeArgsCount;
    hr = pCorProfilerInfo->GetClassIDInfo2(classId,
                                          NULL,
                                          NULL,
                                          NULL,
                                          typeIdArgsLen,
                                          &typeArgsCount,
                                          typeArgs);
    if (FAILED(hr))
    {
        LogFailure(U("Error calling GetClassIDInfo2 hr={HR}"), hr);
        ++failures;
    }

    vector<ClassID> types;
    for (ULONG32 i = 0; i < typeArgsCount; ++i)
    {
        types.push_back(typeArgs[i]);
    }

    return types;
}
