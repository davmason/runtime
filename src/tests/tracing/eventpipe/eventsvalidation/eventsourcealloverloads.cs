// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using Microsoft.Diagnostics.NETCore.Client;
using Microsoft.Diagnostics.Tracing;
using System;
using System.Collections.Generic;
using System.Diagnostics.Tracing;
using System.Linq;
using System.Reflection;
using Tracing.Tests.Common;

namespace Tracing.Tests.AllOverloadsTest
{
    [EventSource(Name = "Test.AllOverloadsEventSource")]
    class AllOverloadsEventSource : EventSource
    {
        [Event(1)]
        public void TestEvent1(string s1, string s2, string s3)
        {
            WriteEvent(1, s1, s2, s3);
        }

        [Event(2)]
        public void TestEvent2(string s, int i1, int i2)
        {
            WriteEvent(2, s, i1, i2);
        }

        [Event(3)]
        public void TestEvent3(long l1, long l2, long l3)
        {
            WriteEvent(3, l1, l2, l3);
        }

        [Event(4)]
        public void TestEvent4(int i1, int i2, int i3)
        {
            WriteEvent(4, i1, i2, i3);
        }

        [Event(5)]
        public void TestEvent5(string s1, string s2)
        {
            WriteEvent(5, s1, s2);
        }

        [Event(6)]
        public void TestEvent6(string s, long l)
        {
            WriteEvent(6, s, l);
        }

        [Event(7)]
        public void TestEvent7(string s, int i)
        {
            WriteEvent(7, s, i);
        }

        [Event(8)]
        public void TestEvent8(long l, string s)
        {
            WriteEvent(8, l, s);
        }

        [Event(9)]
        public void TestEvent9(long l1, long l2)
        {
            WriteEvent(9, l1, l2);
        }

        [Event(10)]
        public void TestEvent10(int i1, int i2)
        {
            WriteEvent(10, i1, i2);
        }

        [Event(11)]
        public void TestEvent11(int i, string s)
        {
            WriteEvent(11, i, s);
        }

        [Event(12)]
        public void TestEvent12(string s)
        {
            WriteEvent(12, s);
        }

        [Event(13)]
        public void TestEvent13(long l, byte[] array)
        {
            WriteEvent(13, l, array);
        }

        [Event(14)]
        public void TestEvent14(long l)
        {
            WriteEvent(14, l);
        }

        [Event(15)]
        public void TestEvent15(int i)
        {
            WriteEvent(15, i);
        }

        [Event(16)]
        public void TestEvent16(byte[] array)
        {
            WriteEvent(16, array);
        }

        [Event(17)]
        public void TestEvent17()
        {
            WriteEvent(17);
        }
    }

    internal class Program
    {
        private static Dictionary<string, List<Tuple<string, object>>> _methodCalls = new Dictionary<string, List<Tuple<string, object>>>();

        private static Dictionary<string, ExpectedEventCount> _expectedEventCounts = new Dictionary<string, ExpectedEventCount>()
        {
            { "Test.AllOverloadsEventSource", 17 }
        };

        private static Func<EventPipeEventSource, Func<int>> _ValidateEvents = (source) =>
        {
            source.Dynamic.All += traceEvent =>
            {
                if (traceEvent.ProviderName != "Test.AllOverloadsEventSource")
                {
                    return;
                }

                if (!_methodCalls.ContainsKey(traceEvent.EventName))
                {
                    throw new ArgumentException($"Event {traceEvent.EventName} is an unknown event");
                }

                List<Tuple<string, object>> methodArgs = _methodCalls[traceEvent.EventName];
                if (methodArgs.Count != traceEvent.PayloadNames.Length) 
                {
                    throw new Exception($"Event {traceEvent.EventName} has mismatched argument count");
                }

                for (int i = 0; i < methodArgs.Count; ++i)
                {
                    string name = traceEvent.PayloadNames[i];
                    Tuple<string, object> tuple= methodArgs[i];
                    if (name != tuple.Item1)
                    {
                        throw new Exception($"Event {traceEvent.EventName} arg {name} has mismatched names {name} and {tuple.Item1}");
                    }

                    object payloadValue = traceEvent.PayloadByName(name);
                    object sentValue = tuple.Item2;
                    VerifyObjectsEqual(traceEvent.EventName, name, payloadValue, sentValue);
                }
            };


            return () => 100;
        };

        private static Action _eventGeneratingAction = () =>
        {
            using (AllOverloadsEventSource myEventSource = new AllOverloadsEventSource())
            {
                foreach (MethodInfo testMethod in myEventSource.GetType().GetMethods())
                {
                    if (testMethod.Name.StartsWith("TestEvent"))
                    {
                        List<Tuple<string, object>> methodCallArgs = new List<Tuple<string, object>>();
                        foreach (ParameterInfo info in testMethod.GetParameters())
                        {
                            object arg = GetObjectForParameterInfo(info);
                            methodCallArgs.Add(Tuple.Create(info.Name ?? string.Empty, arg));
                        }

                        _methodCalls.Add(testMethod.Name, methodCallArgs);

                        testMethod.Invoke(myEventSource, methodCallArgs.Select(x => x.Item2).ToArray());
                    }
                }
            }
        };

        private static int Main(string[] args)
        {
            var providers = new List<EventPipeProvider>()
            {
                new EventPipeProvider("Test.AllOverloadsEventSource", EventLevel.Verbose)
            };

            return IpcTraceTest.RunAndValidateEventCounts(_expectedEventCounts, _eventGeneratingAction, providers, 1024, _ValidateEvents);
        }

        private static void VerifyObjectsEqual(string eventName, string argName, object payloadValue, object sentValue)
        {
            if (payloadValue.GetType() != sentValue.GetType())
            {
                throw new Exception($"Event {eventName} mismatched arg types for arg {argName}. Saw {payloadValue.GetType()} and {sentValue.GetType()}");
            }

            if (payloadValue.GetType() == typeof(byte[]))
            {
                byte[] lhs = (byte[])payloadValue;
                byte[] rhs = (byte[])sentValue;

                if (lhs.Length != rhs.Length)
                {
                    throw new Exception($"Event {eventName} arg {argName} saw mismatched array length");
                }

                for (int i = 0; i < lhs.Length; ++i)
                {
                    if (lhs[i] != (rhs[i]))
                    {
                        for (int j = 0; j < lhs.Length; ++j)
                        {
                            Console.Write($"lhs[{j}]={lhs[j]} rhs[{j}]={rhs[j]}");
                        }

                        Console.WriteLine();
                        throw new Exception($"Event {eventName} arg {argName} saw mismatched byte args at index {i} {lhs[i]} {rhs[i]}");
                    }
                }
            }
            else
            {
                if (!payloadValue.Equals(sentValue))
                {
                    throw new Exception($"Event {eventName} arg {argName} saw mismatched values {payloadValue} and {sentValue}");
                }
            }
        }

        private static object GetObjectForParameterInfo(ParameterInfo info)
        {
            if (info.ParameterType == typeof(string))
            {
                return "A sample string";
            }
            else if (info.ParameterType == typeof(int))
            {
                return 1234;
            }
            else if (info.ParameterType == typeof(long))
            {
                return 123456L;
            }
            else if (info.ParameterType == typeof(byte[]))
            {
                return new byte[] { 1, 2, 3, 4 };
            }

            throw new ArgumentException("Saw unexpected type");
        }
    }
}
