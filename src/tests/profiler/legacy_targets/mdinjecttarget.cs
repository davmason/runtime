// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.mdinjecttarget
{
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //  This assembly MDInjectTarget is for testing MetaData injection tests on REJIT. 
    //  Test Profiler       : InjectMetaData.dll
    //  Other Dependencies  : MDInjectSource.exe 
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    using System;
    using System.Text;
    using System.Runtime.InteropServices;
    using System.Reflection;


    public class MDInjectTarget
    {
        public static int mdinjecttarget(string[] args)
        {
            AssemblyName sourceDllName = new AssemblyName("MDInjectSource");
            Assembly assem = Assembly.Load(sourceDllName);
            BindingFlags flags = BindingFlags.CreateInstance;
            Console.WriteLine("Force a reference on System.Reflection with {0}", flags);
            if (args[0].Equals("NewUserString", StringComparison.CurrentCultureIgnoreCase))
            {
                TestUserDefinedString t = new TestUserDefinedString();
                return t.RunTest();
            }
            else if (args[0].Equals("NewTypes", StringComparison.CurrentCultureIgnoreCase) ||
                args[0].Equals("NewGenericTypes", StringComparison.CurrentCultureIgnoreCase))
            {
                TestAssemblyInjection t = new TestAssemblyInjection();
                return t.RunTest();
            }
            else if (args[0].Equals("NewModuleRef", StringComparison.CurrentCultureIgnoreCase))
            {
                TestModuleRefInjection t = new TestModuleRefInjection();
                return t.RunTest();
            }
            else if (args[0].Equals("NewAssemblyRef", StringComparison.CurrentCultureIgnoreCase))
            {
                TestAssemblyInjection t = new TestAssemblyInjection();
                return t.RunTest();
            }
            else if (args[0].Equals("NewMethodSpec", StringComparison.CurrentCultureIgnoreCase))
            {
                TestMethodSpec t = new TestMethodSpec();
                return t.RunTest();
            }
            else if (args[0].Equals("NewTypeSpec", StringComparison.CurrentCultureIgnoreCase))
            {
                TestTypeSpec t = new TestTypeSpec();
                return t.RunTest();
            }
            else if (args[0].Equals("NewExternalMethodCall", StringComparison.CurrentCultureIgnoreCase))
            {
                TestCrossAssemblyCall t = new TestCrossAssemblyCall();
                return t.RunTest();
            }

            return -1;
        }

        public static void ControlFunction()
        {
            Console.WriteLine("This Triggers REJIT");
        }

        public static int DummyMethod()
        {
            Console.WriteLine("DummyMethod");
            return -1;
        }

        public static int DummyMethod(int t)
        {
            Console.WriteLine("Dummy Method {0}", t);
            return -1;
        }
    }

    public class TestUserDefinedString
    {
        private int PrintHelloWorld()
        {
            string greetings = "Hello World";
            Console.WriteLine(greetings);

            // Constructing a string which is not in the metadata string table.
            StringBuilder sb = new StringBuilder();
            for (char i = 'a'; i <= 'j'; i++)
            {
                sb.Append(i);
            }

            // After REJIT greetings == "abcdefghij"
            if (greetings.Equals(sb.ToString()))
            {
                return 100; // SUCCESS 
            }

            return -1;
        }

        public int RunTest()
        {
            PrintHelloWorld();
            MDInjectTarget.ControlFunction();
            return PrintHelloWorld(); // Should print a new user defined string.
        }
    }


    public class TestAssemblyInjection
    {
        // This will get instrumented to call MDMaster.Verify()
        private int CallMethod()
        {
            return MDInjectTarget.DummyMethod();
        }

        public int RunTest()
        {
            CallMethod();
            MDInjectTarget.ControlFunction();
            return CallMethod(); // Should print new stuff
        }
    }

    public class TestAssemblyInjectionReflection
    {
        // This will get instrumented to call MDMaster.Verify()
        private int CallMethod()
        {
            return MDInjectTarget.DummyMethod();
        }

        public int RunTest()
        {
            CallMethod();
            MDInjectTarget.ControlFunction();
            return CallMethod(); // Should print new stuff
        }
    }

    public class InjectType
    {
        public static int InjectMethod()
        {
            Console.WriteLine("Inside InjectMethod");
            return 100;
        }
    }

    public class TestCrossAssemblyCall
    {
        private int CallMethod()
        {
            return MDInjectTarget.DummyMethod();
        }

        void SampleMethod()
        {
            Console.WriteLine("SampleMethod called");
        }

        public int RunTest()
        {
            CallMethod();
            MDInjectTarget.ControlFunction();
            return CallMethod();
        }
    }

    public class TestModuleRefInjection
    {
        private int CallMethod()
        {
            return MDInjectTarget.DummyMethod();
        }

        public int RunTest()
        {
            CallMethod();
            MDInjectTarget.ControlFunction();
            return CallMethod();
        }
    }

    public class TestAssemblyRefInjection
    {
        private int CallMethod()
        {
            return MDInjectTarget.DummyMethod();
        }

        public int RunTest()
        {
            CallMethod();
            MDInjectTarget.ControlFunction();
            return CallMethod();
        }
    }


    public class TestMethodSpec
    {
        private int CallMethod()
        {
            return MDInjectTarget.DummyMethod(1);
        }

        public int RunTest()
        {
            return CallMethod();
        }
    }

    public class TestTypeSpec
    {
        private int CallMethod()
        {
            return MDInjectTarget.DummyMethod(5);
        }

        public int RunTest()
        {
            return CallMethod();
        }
    }


    public static class SomeStaticClass
    {
        public static int GenericMethod<T>(T t)
        {
            Console.WriteLine(t.ToString());
            return 100;
        }
    }

    public class MyGenericClass<T>
    {
        public static int Method(T t)
        {
            Console.WriteLine(t);
            return 100;
        }
    }

}

