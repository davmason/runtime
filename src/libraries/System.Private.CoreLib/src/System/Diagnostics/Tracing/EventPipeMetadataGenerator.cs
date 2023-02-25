// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
using System.Collections.Generic;
using System.Diagnostics.CodeAnalysis;
using System.Reflection;
using EventMetadata = System.Diagnostics.Tracing.EventSource.EventMetadata;

namespace System.Diagnostics.Tracing
{
#if FEATURE_PERFTRACING
    internal sealed class EventPipeMetadataGenerator
    {
        private enum MetadataTag
        {
            Opcode = 1,
            ParameterPayload = 2
        }

        public static EventPipeMetadataGenerator Instance = new EventPipeMetadataGenerator();

        private EventPipeMetadataGenerator() { }

        [UnconditionalSuppressMessage("ReflectionAnalysis", "IL2026:RequiresUnreferencedCode",
            Justification = "This overload is only called with primitive types or arrays of primitive types.")]
        public byte[]? GenerateEventMetadata(EventMetadata eventMetadata)
        {
            ParameterInfo[] parameters = eventMetadata.Parameters;
            EventParameterInfo[] eventParams = new EventParameterInfo[parameters.Length];
            for (int i = 0; i < parameters.Length; i++)
            {
                TraceLoggingTypeInfo paramTypeInfo = TraceLoggingTypeInfo.GetInstance(parameters[i].ParameterType, new List<Type>());
                Debug.Assert(paramTypeInfo is ScalarTypeInfo || paramTypeInfo is ScalarArrayTypeInfo);
                eventParams[i].SetInfo(parameters[i].Name!, parameters[i].ParameterType, paramTypeInfo);
            }

            return GenerateMetadata(
                eventMetadata.Descriptor.EventId,
                eventMetadata.Name,
                eventMetadata.Descriptor.Keywords,
                eventMetadata.Descriptor.Level,
                eventMetadata.Descriptor.Version,
                (EventOpcode)eventMetadata.Descriptor.Opcode,
                eventParams);
        }

        public byte[]? GenerateEventMetadata(
            int eventId,
            string eventName,
            EventKeywords keywords,
            EventLevel level,
            uint version,
            EventOpcode opcode,
            TraceLoggingEventTypes eventTypes)
        {
            TraceLoggingTypeInfo[] typeInfos = eventTypes.typeInfos;
            string[]? paramNames = eventTypes.paramNames;
            EventParameterInfo[] eventParams = new EventParameterInfo[typeInfos.Length];
            for (int i = 0; i < typeInfos.Length; i++)
            {
                string paramName = string.Empty;
                if (paramNames != null)
                {
                    paramName = paramNames[i];
                }
                eventParams[i].SetInfo(paramName, typeInfos[i].DataType, typeInfos[i]);
            }

            return GenerateMetadata(eventId, eventName, (long)keywords, (uint)level, version, opcode, eventParams);
        }

        internal unsafe byte[]? GenerateMetadata(
            int eventId,
            string eventName,
            long keywords,
            uint level,
            uint version,
            EventOpcode opcode,
            EventParameterInfo[] parameters)
        {
            byte[]? metadata = null;
            bool hasV2ParameterTypes = false;
            try
            {
                // eventID          : 4 bytes
                // eventName        : (eventName.Length + 1) * 2 bytes
                // keywords         : 8 bytes
                // eventVersion     : 4 bytes
                // level            : 4 bytes
                // parameterCount   : 4 bytes
                uint v1MetadataLength = 24 + ((uint)eventName.Length + 1) * 2;
                uint v2MetadataLength = 0;
                uint defaultV1MetadataLength = v1MetadataLength;

                // Check for an empty payload.
                // Write<T> calls with no arguments by convention have a parameter of
                // type NullTypeInfo which is serialized as nothing.
                if ((parameters.Length == 1) && (parameters[0].ParameterType == typeof(EmptyStruct)))
                {
                    parameters = Array.Empty<EventParameterInfo>();
                }

                // Increase the metadataLength for parameters.
                foreach (EventParameterInfo parameter in parameters)
                {
                    uint pMetadataLength;
                    if (!parameter.GetMetadataLength(out pMetadataLength))
                    {
                        // The call above may return false which means it is an unsupported type for V1.
                        // If that is the case we use the v2 blob for metadata instead
                        hasV2ParameterTypes = true;
                        break;
                    }

                    v1MetadataLength += (uint)pMetadataLength;
                }

                if (hasV2ParameterTypes)
                {
                    v1MetadataLength = defaultV1MetadataLength;

                    // V2 length is the parameter count (4 bytes) plus the size of the params
                    v2MetadataLength = 4;
                    foreach (EventParameterInfo parameter in parameters)
                    {
                        uint pMetadataLength;
                        if (!parameter.GetMetadataLengthV2(out pMetadataLength))
                        {
                            // We ran in to an unsupported type, return empty event metadata
                            parameters = Array.Empty<EventParameterInfo>();
                            v1MetadataLength = defaultV1MetadataLength;
                            v2MetadataLength = 0;
                            hasV2ParameterTypes = false;
                            break;
                        }

                        v2MetadataLength += (uint)pMetadataLength;
                    }
                }

                // Optional opcode length needs 1 byte for the opcode + 5 bytes for the tag (4 bytes size, 1 byte kind)
                uint opcodeMetadataLength = opcode == EventOpcode.Info ? 0u : 6u;
                // Optional V2 metadata needs the size of the params + 5 bytes for the tag (4 bytes size, 1 byte kind)
                uint v2MetadataPayloadLength = v2MetadataLength == 0 ? 0 : v2MetadataLength + 5;
                uint totalV2MetadataLength = v2MetadataPayloadLength + opcodeMetadataLength;
                uint totalMetadataLength = v1MetadataLength + totalV2MetadataLength;
                metadata = new byte[totalMetadataLength];

                // Write metadata: metadataHeaderLength, eventID, eventName, keywords, eventVersion, level,
                //                 parameterCount, param1..., optional extended metadata
                fixed (byte* pMetadata = metadata)
                {
                    uint offset = 0;

                    BufferHelpers.WriteToBuffer(pMetadata, totalMetadataLength, ref offset, (uint)eventId);
                    fixed (char* pEventName = eventName)
                    {
                        BufferHelpers.WriteToBuffer(pMetadata, totalMetadataLength, ref offset, (byte*)pEventName, ((uint)eventName.Length + 1) * 2);
                    }
                    BufferHelpers.WriteToBuffer(pMetadata, totalMetadataLength, ref offset, keywords);
                    BufferHelpers.WriteToBuffer(pMetadata, totalMetadataLength, ref offset, version);
                    BufferHelpers.WriteToBuffer(pMetadata, totalMetadataLength, ref offset, level);

                    if (hasV2ParameterTypes)
                    {
                        // If we have unsupported types, the V1 metadata must be empty. Write 0 count of params.
                        BufferHelpers.WriteToBuffer(pMetadata, totalMetadataLength, ref offset, 0);
                    }
                    else
                    {
                        // Without unsupported V1 types we can write all the params now.
                        BufferHelpers.WriteToBuffer(pMetadata, totalMetadataLength, ref offset, (uint)parameters.Length);
                        foreach (var parameter in parameters)
                        {
                            if (!parameter.GenerateMetadataV1(pMetadata, ref offset, totalMetadataLength))
                            {
                                // If we fail to generate metadata for any parameter, we should return the "default" metadata without any parameters
                                return GenerateMetadata(eventId, eventName, keywords, level, version, opcode, Array.Empty<EventParameterInfo>());
                            }
                        }
                    }

                    Debug.Assert(offset == v1MetadataLength);

                    if (opcode != EventOpcode.Info)
                    {
                        // Size of opcode
                        BufferHelpers.WriteToBuffer(pMetadata, totalMetadataLength, ref offset, 1);
                        BufferHelpers.WriteToBuffer(pMetadata, totalMetadataLength, ref offset, (byte)MetadataTag.Opcode);
                        BufferHelpers.WriteToBuffer(pMetadata, totalMetadataLength, ref offset, (byte)opcode);
                    }

                    if (hasV2ParameterTypes)
                    {
                        // Write the V2 supported metadata now
                        // Starting with the size of the V2 payload
                        BufferHelpers.WriteToBuffer(pMetadata, totalMetadataLength, ref offset, v2MetadataLength);
                        // Now the tag to identify it as a V2 parameter payload
                        BufferHelpers.WriteToBuffer(pMetadata, totalMetadataLength, ref offset, (byte)MetadataTag.ParameterPayload);
                        // Then the count of parameters
                        BufferHelpers.WriteToBuffer(pMetadata, totalMetadataLength, ref offset, (uint)parameters.Length);
                        // Finally the parameters themselves
                        foreach (var parameter in parameters)
                        {
                            if (!parameter.GenerateMetadataV2(pMetadata, ref offset, totalMetadataLength))
                            {
                                // If we fail to generate metadata for any parameter, we should return the "default" metadata without any parameters
                                return GenerateMetadata(eventId, eventName, keywords, level, version, opcode, Array.Empty<EventParameterInfo>());
                            }
                        }
                    }

                    Debug.Assert(totalMetadataLength == offset);
                }
            }
            catch
            {
                // If a failure occurs during metadata generation, make sure that we don't return
                // malformed metadata.  Instead, return a null metadata blob.
                // Consumers can either build in knowledge of the event or skip it entirely.
                metadata = null;
            }

            return metadata;
        }
    }

    internal struct EventParameterInfo
    {
        internal string ParameterName;
        internal Type ParameterType;
        internal TraceLoggingTypeInfo TypeInfo;

        internal void SetInfo(string name, Type type, TraceLoggingTypeInfo typeInfo)
        {
            ParameterName = name;
            ParameterType = type;
            TypeInfo = typeInfo;
        }

        internal unsafe bool GenerateMetadataV1(byte* pMetadataBlob, ref uint offset, uint blobSize)
        {
            return GenerateMetadataHelperV1(pMetadataBlob, ref offset, blobSize, out _);
        }

        internal unsafe bool GetMetadataLength(out uint size)
        {
            uint offset = 0;
            return GenerateMetadataHelperV1(null, ref offset, 0, out size);
        }

        private unsafe bool GenerateMetadataHelperV1(byte* pMetadataBlob, ref uint offset, uint blobSize, out uint sizeWritten)
        {
            sizeWritten = 0;

            if (TypeInfo is InvokeTypeInfo invokeTypeInfo)
            {
                // Each nested struct is serialized as:
                //     TypeCode.Object              : 4 bytes
                //     Number of properties         : 4 bytes
                //     Property description 0...N
                //     Nested struct property name  : NULL-terminated string.
                if (pMetadataBlob != null)
                {
                    BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, (uint)TypeCode.Object);
                }
                sizeWritten += 4;

                // Get the set of properties to be serialized.
                PropertyAnalysis[]? properties = invokeTypeInfo.properties;
                if (properties != null)
                {
                    if (pMetadataBlob != null)
                    {
                        // Write the count of serializable properties.
                        BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, (uint)properties.Length);
                    }
                    sizeWritten += 4;

                    foreach (PropertyAnalysis prop in properties)
                    {
                        if (!GenerateMetadataForPropertyV1(prop, pMetadataBlob, ref offset, blobSize, ref sizeWritten))
                        {
                            return false;
                        }
                    }
                }
                else
                {
                    if (pMetadataBlob != null)
                    {
                        // This struct has zero serializable properties so we just write the property count.
                        BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, (uint)0);
                    }
                    sizeWritten += 4;
                }

                if (pMetadataBlob != null)
                {
                    // Top-level structs don't have a property name, but for simplicity we write a NULL-char to represent the name.
                    BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, '\0');
                }
                sizeWritten += 2;
            }
            else if (TypeInfo is ScalarArrayTypeInfo || TypeInfo is EnumerableTypeInfo)
            {
                // Array/Enumerable are unsupported in v1
                return false;
            }
            else
            {
                Debug.Assert(TypeInfo is ScalarTypeInfo);

                TypeCode typeCode = GetTypeCodeExtended(ParameterType);
                if (pMetadataBlob != null)
                {
                    // Write parameter type.
                    BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, (uint)typeCode);
                }
                sizeWritten += 4;

                uint nameLength = ((uint)ParameterName.Length + 1) * 2;

                if (pMetadataBlob != null)
                {
                    // Write parameter name.
                    fixed (char* pParameterName = ParameterName)
                    {
                        BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, (byte*)pParameterName, nameLength);
                    }
                }
                sizeWritten += nameLength;
            }
            return true;
        }

        private static unsafe bool GenerateMetadataForPropertyV1(PropertyAnalysis property, byte* pMetadataBlob, ref uint offset, uint blobSize, ref uint sizeWritten)
        {
            Debug.Assert(property != null);
            Debug.Assert(pMetadataBlob != null);

            // Check if this property is a nested struct.
            if (property.typeInfo is InvokeTypeInfo invokeTypeInfo)
            {
                if (pMetadataBlob != null)
                {
                    // Each nested struct is serialized as:
                    //     TypeCode.Object              : 4 bytes
                    //     Number of properties         : 4 bytes
                    //     Property description 0...N
                    //     Nested struct property name  : NULL-terminated string.
                    BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, (uint)TypeCode.Object);
                }
                sizeWritten += 4;

                // Get the set of properties to be serialized.
                PropertyAnalysis[]? properties = invokeTypeInfo.properties;
                if (properties != null)
                {
                    if (pMetadataBlob != null)
                    {
                        // Write the count of serializable properties.
                        BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, (uint)properties.Length);
                    }
                    sizeWritten += 4;

                    foreach (PropertyAnalysis prop in properties)
                    {
                        if (!GenerateMetadataForPropertyV1(prop, pMetadataBlob, ref offset, blobSize, ref sizeWritten))
                        {
                            return false;
                        }
                    }
                }
                else
                {
                    if (pMetadataBlob != null)
                    {
                        // This struct has zero serializable properties so we just write the property count.
                        BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, (uint)0);
                    }
                    sizeWritten += 4;
                }

                uint nameLength = ((uint)property.name.Length + 1) * 2;
                if (pMetadataBlob != null)
                {
                    // Write the property name.
                    fixed (char* pPropertyName = property.name)
                    {
                        BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, (byte*)pPropertyName, nameLength);
                    }
                }
                sizeWritten += nameLength;
            }
            else if (property.typeInfo is ScalarArrayTypeInfo || property.typeInfo is EnumerableTypeInfo)
            {
                // Array/Enumerable are unsupported in v1
                return false;
            }
            else
            {
                Debug.Assert(property.typeInfo is ScalarTypeInfo);

                // Each primitive type is serialized as:
                //     TypeCode : 4 bytes
                //     PropertyName : NULL-terminated string
                TypeCode typeCode = GetTypeCodeExtended(property.typeInfo.DataType);
                if (pMetadataBlob != null)
                {
                    // Write the type code.
                    BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, (uint)typeCode);
                }
                sizeWritten += 4;

                uint nameLength = ((uint)property.name.Length + 1) * 2;
                if (pMetadataBlob != null)
                {
                    // Write the property name.
                    fixed (char* pPropertyName = property.name)
                    {
                        BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, (byte*)pPropertyName, nameLength);
                    }
                }
                sizeWritten += nameLength;
            }
            return true;
        }

        internal unsafe bool GenerateMetadataV2(byte* pMetadataBlob, ref uint offset, uint blobSize)
        {
            return GenerateMetadataHelperV2(pMetadataBlob, ref offset, blobSize, out _);
        }

        internal unsafe bool GetMetadataLengthV2(out uint size)
        {
            uint offset = 0;
            return GenerateMetadataHelperV2(null, ref offset, 0, out size);
        }

        private unsafe bool GenerateMetadataHelperV2(byte* pMetadataBlob, ref uint offset, uint blobSize, out uint sizeWritten)
        {
            sizeWritten = 0;
            return GenerateMetadataForNamedTypeV2(ParameterName, TypeInfo, pMetadataBlob, ref offset, blobSize, ref sizeWritten);
        }

        private static unsafe bool GenerateMetadataForNamedTypeV2(string name, TraceLoggingTypeInfo typeInfo, byte* pMetadataBlob, ref uint offset, uint blobSize, ref uint sizeWritten)
        {
            uint thisFieldLength = 4;
            uint fieldLengthOffset = offset;
            offset += 4;

            uint nameLength = ((uint)name.Length + 1) * 2;
            if (pMetadataBlob != null)
            {
                // Write the property name.
                fixed (char* pPropertyName = name)
                {
                    BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, (byte*)pPropertyName, nameLength);
                }
            }
            thisFieldLength += nameLength;

            bool success = GenerateMetadataForTypeV2(typeInfo, pMetadataBlob, ref offset, blobSize, ref thisFieldLength);
            if (success)
            {
                if (pMetadataBlob != null)
                {
                    BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref fieldLengthOffset, thisFieldLength);
                }
                sizeWritten += thisFieldLength;
            }

            return success;
        }

        private static unsafe bool GenerateMetadataForTypeV2(TraceLoggingTypeInfo typeInfo, byte* pMetadataBlob, ref uint offset, uint blobSize, ref uint sizeWritten)
        {
            Debug.Assert(typeInfo != null);
            Debug.Assert(pMetadataBlob != null);

            // Check if this type is a nested struct.
            if (typeInfo is InvokeTypeInfo invokeTypeInfo)
            {
                if (pMetadataBlob != null)
                {
                    // Each nested struct is serialized as:
                    //     TypeCode.Object              : 4 bytes
                    //     Number of properties         : 4 bytes
                    //     Property description 0...N
                    BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, (uint)TypeCode.Object);
                }
                sizeWritten += 4;

                // Get the set of properties to be serialized.
                PropertyAnalysis[]? properties = invokeTypeInfo.properties;
                if (properties != null)
                {
                    if (pMetadataBlob != null)
                    {
                        // Write the count of serializable properties.
                        BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, (uint)properties.Length);
                    }
                    sizeWritten += 4;

                    foreach (PropertyAnalysis prop in properties)
                    {
                        if (!GenerateMetadataForNamedTypeV2(prop.name, prop.typeInfo, pMetadataBlob, ref offset, blobSize, ref sizeWritten))
                        {
                            return false;
                        }
                    }
                }
                else
                {
                    if (pMetadataBlob != null)
                    {
                        // This struct has zero serializable properties so we just write the property count.
                        BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, (uint)0);
                    }
                    sizeWritten += 4;
                }
            }
            else if (typeInfo is EnumerableTypeInfo enumerableTypeInfo)
            {
                if (pMetadataBlob != null)
                {
                    // Each enumerable is serialized as:
                    //     TypeCode.Array               : 4 bytes
                    //     ElementType                  : N bytes
                    BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, EventPipeTypeCodeArray);
                }
                sizeWritten += 4;

                GenerateMetadataForTypeV2(enumerableTypeInfo.ElementInfo, pMetadataBlob, ref offset, blobSize, ref sizeWritten);
            }
            else if (typeInfo is ScalarArrayTypeInfo arrayTypeInfo)
            {
                if (pMetadataBlob != null)
                {
                    // Each scalar array is serialized as:
                    //     TypeCode.Array               : 4 bytes
                    //     Scalar type code             : 4 bytes
                    BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, EventPipeTypeCodeArray);
                    TypeCode typeCode = GetTypeCodeExtended(arrayTypeInfo.DataType.GetElementType()!);
                    BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, (uint)typeCode);
                }
                sizeWritten += 8;
            }
            else
            {
                Debug.Assert(typeInfo is ScalarTypeInfo);

                if (pMetadataBlob != null)
                {
                    // Each primitive type is serialized as:
                    //     TypeCode : 4 bytes
                    TypeCode typeCode = GetTypeCodeExtended(typeInfo.DataType);
                    BufferHelpers.WriteToBuffer(pMetadataBlob, blobSize, ref offset, (uint)typeCode);
                }
                sizeWritten += 4;
            }

            return true;
        }

        // Array is not part of TypeCode, we decided to use 19 to represent it. (18 is the last type code value, string)
        private const int EventPipeTypeCodeArray = 19;

        private static TypeCode GetTypeCodeExtended(Type parameterType)
        {
            // Guid is not part of TypeCode, we decided to use 17 to represent it, as it's the "free slot"
            // see https://github.com/dotnet/runtime/issues/9629#issuecomment-361749750 for more
            const TypeCode GuidTypeCode = (TypeCode)17;

            if (parameterType == typeof(Guid)) // Guid is not a part of TypeCode enum
                return GuidTypeCode;

            // IntPtr and UIntPtr are converted to their non-pointer types.
            if (parameterType == typeof(IntPtr))
                return IntPtr.Size == 4 ? TypeCode.Int32 : TypeCode.Int64;

            if (parameterType == typeof(UIntPtr))
                return UIntPtr.Size == 4 ? TypeCode.UInt32 : TypeCode.UInt64;

            return Type.GetTypeCode(parameterType);
        }
    }

    internal static class BufferHelpers
    {
        // Copy src to buffer and modify the offset.
        // Note: We know the buffer size ahead of time to make sure no buffer overflow.
        internal static unsafe void WriteToBuffer(byte* buffer, uint bufferLength, ref uint offset, byte* src, uint srcLength)
        {
            Debug.Assert(bufferLength >= (offset + srcLength));
            for (int i = 0; i < srcLength; i++)
            {
                *(byte*)(buffer + offset + i) = *(byte*)(src + i);
            }
            offset += srcLength;
        }

        internal static unsafe void WriteToBuffer<T>(byte* buffer, uint bufferLength, ref uint offset, T value) where T : unmanaged
        {
            Debug.Assert(bufferLength >= (offset + sizeof(T)));
            *(T*)(buffer + offset) = value;
            offset += (uint)sizeof(T);
        }
    }
#endif // FEATURE_PERFTRACING
}
