using Dia2Lib;
using Microsoft.Diagnostics.NETCore.Client;
using Microsoft.Diagnostics.Tracing;
using Microsoft.Diagnostics.Tracing.Session;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Diagnostics.Tracing;
using System.Reflection;
using System.Threading.Tasks;
using System.Linq;

namespace EventListenerTest
{
    [EventSource(Name = "AllOverloadsEventSource")]
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

        [Event(18)]
        public void TestEvent18(long l, byte[] array)
        {
            WriteEvent(18, l, array);
        }
    }

    internal class Program
    {
        private static int Main(string[] args)
        {
            var client = new DiagnosticsClient(Environment.ProcessId);

            var provider = new EventPipeProvider("AllOverloadsEventSource", EventLevel.Verbose);
            using (var session = client.StartEventPipeSession(provider, false))
            using (var source = new EventPipeEventSource(session.EventStream))
            {
                Dictionary<string, List<Tuple<string, object>>> methodCalls = new Dictionary<string, List<Tuple<string, object>>>();

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

                            methodCalls.Add(testMethod.Name, methodCallArgs);

                            testMethod.Invoke(myEventSource, methodCallArgs.Select(x => x.Item2).ToArray());
                        }
                    }
                }

                int eventCount = 0;
                source.AllEvents += traceEvent =>
                    {
                        if (traceEvent.ProviderName != "AllOverloadsEventSource")
                        {
                            return;
                        }

                        ++eventCount;
                        if (!methodCalls.ContainsKey(traceEvent.EventName))
                        {
                            throw new ArgumentException($"Event {traceEvent.EventName} is an unknown event");
                        }

                        List<Tuple<string, object>> methodArgs = methodCalls[traceEvent.EventName];
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

                Task processTask = Task.Run(source.Process);
                session.Stop();

                processTask.Wait();

                if (eventCount > 0)
                {
                    // We would have thrown an exception for any errors
                    return 100;
                }

                Console.WriteLine("Test failed because no events were seen.");
                return -1;
            }
        }

        private static void VerifyObjectsEqual(string eventName, string argName, object payloadValue, object sentValue)
        {
            if (payloadValue.GetType() != sentValue.GetType())
            {
                throw new Exception($"Event {eventName} mismatched arg types for arg {argName}. Saw {payloadValue.GetType()} and {sentValue.GetType()}");
            }

            if (payloadValue.GetType() == typeof(object[]))
            {
                object[] lhs = (object[]) payloadValue;
                object[] rhs = (object[]) sentValue;

                if (lhs.Length != rhs.Length)
                {
                    throw new Exception($"Event {eventName} arg {argName} saw mismatched array length");
                }

                for (int i = 0; i < lhs.Length; ++i)
                {
                    if (!lhs[i].Equals(rhs[i]))
                    {
                        throw new Exception($"Event {eventName} arg {argName} saw mismatched array args at index {i} {lhs[i]} {rhs[i]}");
                    }
                }
            }
            else if (payloadValue.GetType() == typeof(byte[]))
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
                return 123456;
            }
            else if (info.ParameterType == typeof(byte[]))
            {
                return new byte[] { 1, 2, 3, 4 };
            }
            else if (info.ParameterType == typeof(object[]))
            {
                return new object[] { 1, "a string", new byte[] { 1, 2, 3 } };
            }

            throw new ArgumentException("Saw unexpected type");
        }
    }
}