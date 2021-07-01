// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "gcprofiler.h"

GUID GCProfiler::GetClsid()
{
    // {BCD8186F-1EEC-47E9-AFA7-396F879382C3}
    GUID clsid = { 0xBCD8186F, 0x1EEC, 0x47E9, { 0xAF, 0xA7, 0x39, 0x6F, 0x87, 0x93, 0x82, 0xC3 } };
    return clsid;
}

HRESULT GCProfiler::Initialize(IUnknown* pICorProfilerInfoUnk)
{
    Profiler::Initialize(pICorProfilerInfoUnk);

    HRESULT hr = S_OK;
    if (FAILED(hr = pCorProfilerInfo->SetEventMask2(COR_PRF_MONITOR_GC, 0)))
    {
        _failures++;
        LogFailure(U("ICorProfilerInfo::SetEventMask2() failed hr={HR}"), hr);
        return hr;
    }

    return S_OK;
}

HRESULT GCProfiler::Shutdown()
{
    Profiler::Shutdown();

    if (_gcStarts == 0)
    {
        LogMessage(U("GCProfiler::Shutdown: FAIL: Expected GarbaseCollectionStarted to be called"));
    }
    else if (_gcFinishes == 0)
    {
        LogMessage(U("GCProfiler::Shutdown: FAIL: Expected GarbageCollectionFinished to be called"));
    }
    else if (_pohObjectsSeenRootReferences == 0 || _pohObjectsSeenObjectReferences == 0)
    {
        LogMessage(U("GCProfiler::Shutdown: FAIL: no POH objects seen. root references={INT} object references={INT}"),
            _pohObjectsSeenRootReferences.load(), _pohObjectsSeenObjectReferences.load());
    }
    else if(_failures == 0)
    {
        LogMessage(U("PROFILER TEST PASSES"));
    }
    else
    {
        // failures were printed earlier when _failures was incremented
    }
    fflush(stdout);

    return S_OK;
}

HRESULT GCProfiler::GarbageCollectionStarted(int cGenerations, BOOL generationCollected[], COR_PRF_GC_REASON reason)
{
    SHUTDOWNGUARD();

    _gcStarts++;
    if (_gcStarts - _gcFinishes > 2)
    {
        _failures++;
        LogFailure(U("GCProfiler::GarbageCollectionStarted: Expected GCStart <= GCFinish+2. GCStart={INT}, GCFinish={INT}"), (int)_gcStarts, (int)_gcFinishes);
    }

    return S_OK;
}

HRESULT GCProfiler::GarbageCollectionFinished()
{
    SHUTDOWNGUARD();

    _gcFinishes++;
    if (_gcStarts < _gcFinishes)
    {
        _failures++;
        LogFailure(U("GCProfiler::GarbageCollectionFinished: Expected GCStart >= GCFinish. Start={INT}, Finish={INT}"), (int)_gcStarts, (int)_gcFinishes);
    }

    _pohObjectsSeenObjectReferences += NumPOHObjectsSeen(_objectReferencesSeen);
    _pohObjectsSeenRootReferences += NumPOHObjectsSeen(_rootReferencesSeen);
    
    return S_OK;
}

HRESULT GCProfiler::ObjectReferences(ObjectID objectId, ClassID classId, ULONG cObjectRefs, ObjectID objectRefIds[])
{
    SHUTDOWNGUARD();

    HRESULT hr = S_OK;
    for (ULONG i = 0; i < cObjectRefs; ++i)
    {
        ObjectID obj = objectRefIds[i];
        if (obj != NULL)
        {
            _objectReferencesSeen.insert(obj);
        }
    }

    return S_OK;
}

HRESULT GCProfiler::RootReferences(ULONG cRootRefs, ObjectID rootRefIds[])
{
    SHUTDOWNGUARD();

    for (ULONG i = 0; i < cRootRefs; ++i)
    {
        ObjectID obj = rootRefIds[i];
        if (obj != NULL)
        {
            _rootReferencesSeen.insert(obj);
        }
    }

    return S_OK;
}

int GCProfiler::NumPOHObjectsSeen(std::unordered_set<ObjectID> objects)
{
    int count = 0;
    for (auto it = objects.begin(); it != objects.end(); ++it)
    {
        COR_PRF_GC_GENERATION_RANGE gen;
        ObjectID obj = *it;
        HRESULT hr = pCorProfilerInfo->GetObjectGeneration(obj, &gen);
        if (FAILED(hr))
        {
            LogFailure(U("GetObjectGeneration failed hr={HR}"), hr);
            return hr;
        }

        if (gen.generation == COR_PRF_GC_PINNED_OBJECT_HEAP)
        {
            count++;
        }

    }

    return count;
}
