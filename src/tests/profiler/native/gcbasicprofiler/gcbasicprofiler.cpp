// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "gcbasicprofiler.h"

GUID GCBasicProfiler::GetClsid()
{
	// {A040B953-EDE7-42D9-9077-AA69BB2BE024}
	GUID clsid = { 0xa040b953, 0xede7, 0x42d9,{ 0x90, 0x77, 0xaa, 0x69, 0xbb, 0x2b, 0xe0, 0x24 } };
	return clsid;
}

HRESULT GCBasicProfiler::Initialize(IUnknown* pICorProfilerInfoUnk)
{
    Profiler::Initialize(pICorProfilerInfoUnk);

    HRESULT hr = S_OK;
    if (FAILED(hr = pCorProfilerInfo->SetEventMask2(0, 0x10)))
    {
        _failures++;
        LogFailure(U("ICorProfilerInfo::SetEventMask2() failed hr={HR}"), hr);
        return hr;
    }

    return S_OK;
}

HRESULT GCBasicProfiler::Shutdown()
{
    Profiler::Shutdown();

    if (_gcStarts == 0)
    {
        LogMessage(U("GCBasicProfiler::Shutdown: FAIL: Expected GarbaseCollectionStarted to be called"));
    }
    else if (_gcFinishes == 0)
    {
        LogMessage(U("GCBasicProfiler::Shutdown: FAIL: Expected GarbageCollectionFinished to be called"));
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

// filler for empty generations: x86 - 0xc; ia64 - 0x18
#define GEN_FILLER 0x18

HRESULT GCBasicProfiler::GarbageCollectionStarted(int cGenerations, BOOL generationCollected[], COR_PRF_GC_REASON reason)
{
    SHUTDOWNGUARD();

    _gcStarts++;
    if (_gcStarts - _gcFinishes > 2)
    {
        _failures++;
        LogFailure(U("GCBasicProfiler::GarbageCollectionStarted: Expected GCStart <= GCFinish+2. GCStart={INT}, GCFinish={INT}"), (int)_gcStarts, (int)_gcFinishes);
        return S_OK;
    }


    if(_gcStarts == 1)
    {
        ULONG nObjectRanges;
        const ULONG cRanges = 32;
        COR_PRF_GC_GENERATION_RANGE objectRanges[cRanges];

        HRESULT hr = pCorProfilerInfo->GetGenerationBounds(cRanges, &nObjectRanges, objectRanges);
        if (FAILED(hr))
        {
            _failures++;
            LogFailure(U("GCBasicProfiler::GarbageCollectionStarted: GetGenerationBounds hr={HR}"), hr);
            return S_OK;
        }

        // Assuming an initial gc heap like this.
        //
        // Generation: 3	Start: 1ee1000	Length: 2030	Reserved: fff000	// big objects
        // Generation: 2	Start: 7a8d0bbc Length: 1fd1c	Reserved: 1fd1c		// frozen objects (per module)
        // Generation: 2	Start: 3120004	Length: c		Reserved: c			// debug build only, internal
        // Generation: 2	Start: 5ba29b7c	Length: 465f0	Reserved: 465f0		// frozen objects (per module)
        // Generation: 2	Start: ee1000	Length: c		Reserved: c			// empty, with filler
        // Generation: 1	Start: ee100c	Length: c		Reserved: c			// empty, with filler
        // Generation: 0	Start: ee1018	Length: 15b8	Reserved: ffefe8	// new objects

        // loop through generations from 0 to 3
        for (int i = nObjectRanges - 1; i >= 0; i--)
        {
            switch (objectRanges[i].generation)
            {
            case 0:
            case 3:
            case 4:
                // no useful verification
                break;
            case 1:
                if (objectRanges[i].rangeLength > GEN_FILLER)
                {
                    _failures++;
                    LogFailure(U("GCBasicProfiler::GarbageCollectionStarted: Expected initial gen1 rangeLength <= {HEX} rangeLength={HEX}"),
                        GEN_FILLER, (void*)objectRanges[i].rangeLength);
                }
                if (objectRanges[i].rangeLengthReserved > GEN_FILLER)
                {
                    _failures++;
                    LogFailure(U("GCBasicProfiler::GarbageCollectionStarted: Expected initial gen1 rangeLengthReserved <= {HEX} rangeLengthReserved={HEX}"),
                        GEN_FILLER, (void*)objectRanges[i].rangeLengthReserved);
                }
                break;
            case 2:
                break;
            default:
                _failures++;
                LogFailure(U("GCBasicProfiler::GarbageCollectionStarted: FAIL: invalid generation: {INT}"), objectRanges[i].generation);
            }
        }
    }
    return S_OK;
}

HRESULT GCBasicProfiler::GarbageCollectionFinished()
{
    SHUTDOWNGUARD();

    _gcFinishes++;
    if (_gcStarts < _gcFinishes)
    {
        _failures++;
        LogFailure(U("GCBasicProfiler::GarbageCollectionFinished: Expected GCStart >= GCFinish. Start={INT}, Finish={INT}"), (int)_gcStarts, (int)_gcFinishes);
    }
    return S_OK;
}
