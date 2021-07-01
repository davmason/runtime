// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

#include "eventpipereadingprofiler.h"

using std::mutex;
using std::lock_guard;

GUID EventPipeReadingProfiler::GetClsid()
{
    // 9E7F78E2-B3BE-410B-AA8D-E210E4C757A4
    GUID clsid = { 0x9E7F78E2, 0xB3BE, 0x410B, { 0xAA, 0x8D, 0xE2, 0x10, 0xE4, 0xC7, 0x57, 0xA4 } };
    return clsid;
}

HRESULT EventPipeReadingProfiler::Initialize(IUnknown* pICorProfilerInfoUnk)
{
    Profiler::Initialize(pICorProfilerInfoUnk);

    LogMessage(U("EventPipeReadingProfiler::Initialize"));

    HRESULT hr = S_OK;
    if (FAILED(hr = pICorProfilerInfoUnk->QueryInterface(__uuidof(ICorProfilerInfo12), (void **)&_pCorProfilerInfo12)))
    {
        LogFailure(U("failed to QI for ICorProfilerInfo12."));
        _failures++;
        return hr;
    }

    if (FAILED(hr = _pCorProfilerInfo12->SetEventMask2(0, COR_PRF_HIGH_MONITOR_EVENT_PIPE)))
    {
        LogFailure(U("ICorProfilerInfo::SetEventMask2() failed hr={HR}"), hr);
        _failures++;
        return hr;
    }

    COR_PRF_EVENTPIPE_PROVIDER_CONFIG providers[] = {
        { U("EventPipeTestEventSource"), 0xFFFFFFFFFFFFFFFF, 5, NULL }
    };

    hr = _pCorProfilerInfo12->EventPipeStartSession(sizeof(providers) / sizeof(providers[0]),
                                                    providers,
                                                    false,
                                                    &_session);
    if (FAILED(hr))
    {
        LogFailure(U("Failed to start event pipe session with hr={HR}"), hr);
        _failures++;
        return hr;
    }

    LogMessage(U("Started event pipe session!"));

    return S_OK;
}

HRESULT EventPipeReadingProfiler::Shutdown()
{
    Profiler::Shutdown();

    if(_failures == 0 && _events.load() == 3)
    {
        LogMessage(U("PROFILER TEST PASSES"));
    }
    else
    {
        // failures were printed earlier when _failures was incremented
        LogMessage(U("EventPipe profiler test failed failures={INT} events={INT}"), _failures.load(), _events.load());
    }
    fflush(stdout);

    return S_OK;
}

HRESULT EventPipeReadingProfiler::EventPipeEventDelivered(
    EVENTPIPE_PROVIDER provider,
    DWORD eventId,
    DWORD eventVersion,
    ULONG cbMetadataBlob,
    LPCBYTE metadataBlob,
    ULONG cbEventData,
    LPCBYTE eventData,
    LPCGUID pActivityId,
    LPCGUID pRelatedActivityId,
    ThreadID eventThread,
    ULONG numStackFrames,
    UINT_PTR stackFrames[])
{
    SHUTDOWNGUARD();

    String name = GetOrAddProviderName(provider);
    LogMessage(U("EventPipeReadingProfiler saw event {STR}"), name);

    EventPipeMetadataInstance metadata = GetOrAddMetadata(metadataBlob, cbMetadataBlob);

    if (metadata.name == U("MyEvent")
        && ValidateMyEvent(metadata, provider, eventId, eventVersion, cbMetadataBlob, metadataBlob, cbEventData, eventData, pActivityId, pRelatedActivityId, eventThread, numStackFrames, stackFrames))
    {
        _events++;
    }
    else if (metadata.name == U("MyArrayEvent")
             && ValidateMyArrayEvent(metadata, provider, eventId, eventVersion, cbMetadataBlob, metadataBlob, cbEventData, eventData, pActivityId, pRelatedActivityId, eventThread, numStackFrames, stackFrames))
    {
        _events++;
    }
    else if (metadata.name == U("KeyValueEvent")
             && ValidateKeyValueEvent(metadata, provider, eventId, eventVersion, cbMetadataBlob, metadataBlob, cbEventData, eventData, pActivityId, pRelatedActivityId, eventThread, numStackFrames, stackFrames))
    {
        _events++;
    }

    return S_OK;
}

HRESULT EventPipeReadingProfiler::EventPipeProviderCreated(EVENTPIPE_PROVIDER provider)
{
    SHUTDOWNGUARD();

    String name = GetOrAddProviderName(provider);
    LogMessage(U("CorProfiler::EventPipeProviderCreated provider={STR}"), name);

    return S_OK;
}

String EventPipeReadingProfiler::GetOrAddProviderName(EVENTPIPE_PROVIDER provider)
{
    lock_guard<mutex> guard(_cacheLock);

    auto it = _providerNameCache.find(provider);
    if (it == _providerNameCache.end())
    {
        WCHAR nameBuffer[LONG_LENGTH];
        ULONG nameCount;
        HRESULT hr = _pCorProfilerInfo12->EventPipeGetProviderInfo(provider,
                                                                   LONG_LENGTH,
                                                                   &nameCount,
                                                                   nameBuffer);
        if (FAILED(hr))
        {
            LogFailure(U("EventPipeGetProviderInfo failed with hr={HR}"), hr);
            return String(U("GetProviderInfo failed"));
        }

        _providerNameCache.insert({provider, String(nameBuffer)});

        it = _providerNameCache.find(provider);
        assert(it != _providerNameCache.end());
    }

    return it->second;
}

EventPipeMetadataInstance EventPipeReadingProfiler::GetOrAddMetadata(LPCBYTE pMetadata, ULONG cbMetadata)
{
    lock_guard<mutex> guard(_cacheLock);

    auto it = _metadataCache.find(pMetadata);
    if (it == _metadataCache.end())
    {
        EventPipeMetadataReader reader;
        EventPipeMetadataInstance parsedMetadata = reader.Parse(pMetadata, cbMetadata);
        _metadataCache.insert({pMetadata, parsedMetadata});

        it = _metadataCache.find(pMetadata);
        assert(it != _metadataCache.end());
    }

    return it->second;
}

bool EventPipeReadingProfiler::ValidateMyEvent(
    EventPipeMetadataInstance metadata,
    EVENTPIPE_PROVIDER provider,
    DWORD eventId,
    DWORD eventVersion,
    ULONG cbMetadataBlob,
    LPCBYTE metadataBlob,
    ULONG cbEventData,
    LPCBYTE eventData,
    LPCGUID pActivityId,
    LPCGUID pRelatedActivityId,
    ThreadID eventThread,
    ULONG numStackFrames,
    UINT_PTR stackFrames[])
{
    if (metadata.parameters.size() != 1)
    {
        _failures++;
        LogFailure(U("MyEvent expected param size 1, saw {INT}"), (int)metadata.parameters.size());
        return false;
    }

    EventPipeDataDescriptor param = metadata.parameters[0];
    if (param.name != U("i")
        || param.type != EventPipeTypeCode::Int32)
    {
        _failures++;
        LogFailure(U("MyEvent expected param name=i type=Int32, saw name={STR} type={INT}"),
            param.name, param.type);
        return false;
    }

    ULONG offset = 0;
    INT32 data = ReadFromBuffer<INT32>(eventData, cbEventData, &offset);
    if (data != 12)
    {
        _failures++;
        LogFailure(U("MyEvent expected data=12, saw {INT}"), data);
        return false;
    }

    return true;
}

bool EventPipeReadingProfiler::ValidateMyArrayEvent(
    EventPipeMetadataInstance metadata,
    EVENTPIPE_PROVIDER provider,
    DWORD eventId,
    DWORD eventVersion,
    ULONG cbMetadataBlob,
    LPCBYTE metadataBlob,
    ULONG cbEventData,
    LPCBYTE eventData,
    LPCGUID pActivityId,
    LPCGUID pRelatedActivityId,
    ThreadID eventThread,
    ULONG numStackFrames,
    UINT_PTR stackFrames[])
{
    if (metadata.parameters.size() != 3)
    {
        _failures++;
        LogFailure(U("MyArrayEvent expected param size 3, saw {INT}"), (int)metadata.parameters.size());
        return false;
    }

    EventPipeDataDescriptor param0 = metadata.parameters[0];
    if (param0.name != U("ch")
        || param0.type != EventPipeTypeCode::Char)
    {
        _failures++;
        LogFailure(U("MyArrayEvent expected param 0 name=ch type=Char, saw name={STR} type={INT}"), 
            param0.name, param0.type);
        return false;
    }

    ULONG offset = 0;
    WCHAR ch = ReadFromBuffer<WCHAR>(eventData, cbEventData, &offset);
    if (ch != 'd')
    {
        _failures++;
        LogFailure(U("MyArrayEvent expected param 0 value=d, saw {CHAR}"), ch);
        return false;
    }

    EventPipeDataDescriptor param1 = metadata.parameters[1];
    if (param1.name != U("intArray")
        || param1.type != EventPipeTypeCode::ArrayType
        || param1.elementType->type != EventPipeTypeCode::Int32)
    {
        _failures++;
        LogFailure(U("MyArrayEvent expected param 1 name=intArray type=Int32, saw name={STR} type={INT}"), 
            param1.name, param1.elementType->type);
        return false;
    }

    UINT16 arrayLength = ReadFromBuffer<UINT16>(eventData, cbEventData, &offset);
    if (arrayLength != 120)
    {
        _failures++;
        LogFailure(U("MyArrayEvent expected array length 120, saw {INT}"), arrayLength);
        return false;
    }

    for (int i = 0; i < arrayLength; ++i)
    {
        INT32 data = ReadFromBuffer<INT32>(eventData, cbEventData, &offset);
        if (data != i)
        {
            _failures++;
            LogFailure(U("MyArrayEvent expected array index {INT} value {INT}, saw {INT}"), i, i, data);
            return false;
        }
    }

    EventPipeDataDescriptor param2 = metadata.parameters[2];
    if (param2.name != U("str")
        || param2.type != EventPipeTypeCode::String)
    {
        _failures++;
        LogFailure(U("MyArrayEvent expected param 2 name=str type=String, saw name={STR} type={INT}"), 
            param2.name, param2.type);
        return false;
    }

    WCHAR *stringValue = ReadFromBuffer<WCHAR *>(eventData, cbEventData, &offset);
    if (String(U("Hello from EventPipeTestEventSource!")) != stringValue)
    {
        _failures++;
        LogFailure(U("MyArrayEvent expected param2 value=\"Hello from EventPipeTestEventSource!\", saw {STR}"),
            stringValue);
        return false;
    }

    return true;
}

bool EventPipeReadingProfiler::ValidateKeyValueEvent(
    EventPipeMetadataInstance metadata,
    EVENTPIPE_PROVIDER provider,
    DWORD eventId,
    DWORD eventVersion,
    ULONG cbMetadataBlob,
    LPCBYTE metadataBlob,
    ULONG cbEventData,
    LPCBYTE eventData,
    LPCGUID pActivityId,
    LPCGUID pRelatedActivityId,
    ThreadID eventThread,
    ULONG numStackFrames,
    UINT_PTR stackFrames[])
{
    if (metadata.parameters.size() != 3)
    {
        _failures++;
        LogFailure(U("KeyValueEvent expected param size 3, saw {INT}"), (int)metadata.parameters.size());
        return false;
    }

    EventPipeDataDescriptor param0 = metadata.parameters[0];
    if (param0.name != U("SourceName")
        || param0.type != EventPipeTypeCode::String)
    {
        _failures++;
        LogFailure(U("KeyValueEvent expected param 0 name=SourceName type=String, saw name={STR} type={INT}"), 
            param0.name, param0.type);
        return false;
    }

    ULONG offset = 0;
    WCHAR *str = ReadFromBuffer<WCHAR *>(eventData, cbEventData, &offset);
    if (String(U("Source")) != str)
    {
        _failures++;
        LogFailure(U("MyArrayEvent expected param 0 value=\"Source\", saw {STR}"), 
            str);
        return false;
    }

    EventPipeDataDescriptor param1 = metadata.parameters[1];
    if (param1.name != U("EventName")
        || param1.type != EventPipeTypeCode::String)
    {
        _failures++;
        LogFailure(U("KeyValueEvent expected param 1 name=EventName type=String, saw name={STR} type={INT}"), 
            param1.name, param1.type);
        return false;
    }

    WCHAR *event = ReadFromBuffer<WCHAR *>(eventData, cbEventData, &offset);
    if (String(U("Event")) != event)
    {
        _failures++;
        LogFailure(U("MyArrayEvent expected param 1 value=\"Event\", saw {STR}"), 
            event);
        return false;
    }

    EventPipeDataDescriptor param2 = metadata.parameters[2];
    if (param2.name != U("Arguments")
        || param2.type != EventPipeTypeCode::ArrayType
        || param2.elementType->type != EventPipeTypeCode::Object)
    {
        _failures++;
        LogFailure(U("KeyValueEvent expected param 2 name=Arguments type=String, saw name={STR} type={INT}"), 
            param2.name, param2.elementType->type);
        return false;
    }

    UINT16 arrayLength = ReadFromBuffer<UINT16>(eventData, cbEventData, &offset);
    if (arrayLength != 1)
    {
        _failures++;
        LogFailure(U("MyArrayEvent expected array length 1, saw {INT}"), arrayLength);
        return false;
    }

    str = ReadFromBuffer<WCHAR *>(eventData, cbEventData, &offset);
    if (String(U("samplekey")) != str)
    {
        _failures++;
        LogFailure(U("MyArrayEvent expected param 2 value=\"samplekey\", saw {STR}"), 
            str);
        return false;
    }

    str = ReadFromBuffer<WCHAR *>(eventData, cbEventData, &offset);
    if (String(U("samplevalue")) != str)
    {
        LogFailure(U("MyArrayEvent expected param 2 value=\"samplevalue\", saw {STR}"), 
            str);
        _failures++;
        return false;
    }

    return true;
}
