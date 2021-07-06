// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.statics_argretval
{
    using System;

    public struct SimpleStruct
    {
        public int i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z;
    }

    public struct SimpleStructWithStatics
    {
        public int i, j;
        public static int k, l;
    }

    public class SimpleClass
    {
        public int i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z;
    }

    public class SimpleClassWithStatics
    {
        public int i, j;
        public static int k, l;
    }

    public class ClassWithComplexStatics
    {
        public ClassWithComplexStatics()
        {
            local_i = 100;
            ClassWithComplexStatics.i = 200;
            ClassWithComplexStatics.i_thread_static = 300;
            ClassWithComplexStatics.i_context_static = 400;
        }

        int local_i;

        static int i;

        [ThreadStatic]
        static int i_thread_static;

        //[ContextStatic]
        static int i_context_static;

        //static int[] a = { 2, 3, 4, 5, 6, 3, 4, 5, 3, 3, 4 };

        public ClassWithComplexStatics ClassWithComplexStaticsTest(ClassWithComplexStatics ccs)
        {
            int my_i = 1;

            ClassWithComplexStatics local_ccs = new ClassWithComplexStatics();

            local_ccs.local_i = my_i;
            ClassWithComplexStatics.i = local_ccs.local_i * 2;
            ClassWithComplexStatics.i_thread_static = ClassWithComplexStatics.i * 2;
            ClassWithComplexStatics.i_context_static = ClassWithComplexStatics.i_thread_static * 2;

            return local_ccs;
        }

    }

    class GenericMethod
    {
        public static void GenericFoo<T>(T t)
        {
            T local_t = t;
        }

        public static T GenericBar<T>(T t, int i)
        {
            T local_t = t;
            return local_t;
        }
    }

    class GenericClass<T>
    {
        private T class_t;

        public void Foo(T t)
        {
            class_t = t;
        }

        public T Bar(T t, int i)
        {
            class_t = t;
            return class_t;
        }

        public X GenericFooBar<X>(X x, T t)
        {
            X local_x = x;
            T local_t = t;
            return local_x;
        }
    }


    class GenericOrder<T, U, V>
    {
        private T class_t;
        private U class_u;
        private V class_v;

        public V Foo(U u, V v, T t)
        {
            class_t = t;
            class_u = u;
            class_v = v;
            return class_v;
        }

        public A Bar<A, B>(B b, A a, V v, T t, U u)
        {
            A local_a = a;
            B local_b = b;
            class_t = t;
            class_u = u;
            class_v = v;
            return local_a;
        }
    }


    public class Test
    {
        public int IntValueTest(int i, int ii, int iii)
        {
            int local_int = 0;

            local_int = i * ii * iii;

            return 12345;
        }

        public static SimpleStruct SimpleStructTest(SimpleStruct ss)
        {
            SimpleStruct local_ss = new SimpleStruct();
            local_ss.i = ss.i * 2;
            local_ss.j = ss.j * 2;

            return local_ss;
        }

        public static SimpleStructWithStatics SimpleStructStaticsTest(SimpleStructWithStatics sss)
        {
            SimpleStructWithStatics local_sss = new SimpleStructWithStatics();
            local_sss.i = sss.i * 2;
            local_sss.j = sss.j * 2;

            SimpleStructWithStatics.k = SimpleStructWithStatics.k * 2;
            SimpleStructWithStatics.l = SimpleStructWithStatics.l * 2;

            return local_sss;
        }

        public static SimpleClass SimpleClassTest(SimpleClass sc)
        {
            SimpleClass local_sc = new SimpleClass();
            local_sc.i = sc.i * 2;
            local_sc.j = sc.j * 2;

            return local_sc;
        }

        public static SimpleClassWithStatics SimpleClassStaticsTest(SimpleClassWithStatics scs)
        {
            SimpleClassWithStatics local_scs = new SimpleClassWithStatics();
            local_scs.i = scs.i * 2;
            local_scs.j = scs.j * 2;

            SimpleClassWithStatics.k = SimpleClassWithStatics.k * 2;
            SimpleClassWithStatics.l = SimpleClassWithStatics.l * 2;

            return local_scs;
        }

        public static void statics_argretval(string[] args)
        {
            if (args.Length > 0)
            {
                if (args[0] == "-attach")
                {
                    Console.Write("Sleeping for attach... ");
                    System.Threading.Thread.Sleep(1000 * 15);
                    Console.WriteLine("done.");
                }
            }

            // Simple unit test case.  Int's in and Int's out.
            Test testobj = new Test();
            int i = testobj.IntValueTest(1, 2, 3);

            // Struct arg and ret val test case
            SimpleStruct local_ss = new SimpleStruct();
            local_ss.i = 11;
            local_ss.j = 22;
            SimpleStruct ss = SimpleStructTest(local_ss);

            // Class arg and ret val test case
            SimpleClass local_sc = new SimpleClass();
            local_sc.i = 11;
            local_sc.j = 22;
            SimpleClass sc = SimpleClassTest(local_sc);

            // Struct with statics arg and ret val test case
            SimpleStructWithStatics local_sss = new SimpleStructWithStatics();
            local_sss.i = 11;
            local_sss.j = 22;
            SimpleStructWithStatics.k = 33;
            SimpleStructWithStatics.l = 44;
            SimpleStructWithStatics sss = SimpleStructStaticsTest(local_sss);

            // Class with statics arg and ret val test case
            SimpleClassWithStatics local_scs = new SimpleClassWithStatics();
            local_scs.i = 11;
            local_scs.j = 22;
            SimpleClassWithStatics.k = 33;
            SimpleClassWithStatics.l = 44;
            SimpleClassWithStatics scs = SimpleClassStaticsTest(local_scs);


            // Let's feed some more interesting static cases into the Profiler
            ClassWithComplexStatics local_ccs = new ClassWithComplexStatics();
            ClassWithComplexStatics ccs = local_ccs.ClassWithComplexStaticsTest(local_ccs);

            // Time to play with Generics
            GenericClass<int> gci = new GenericClass<int>();
            GenericClass<GenericClass<int>> gcgci = new GenericClass<GenericClass<int>>();

            gci.Foo(100);
            gci.Bar(200, 300);
            gci.GenericFooBar<double>(1.0, 400);

            gcgci.Foo(gci);
            gcgci.Bar(gci, 500);
            gcgci.GenericFooBar<string>("Hello", gci);

            GenericMethod.GenericFoo<int>(600);
            GenericMethod.GenericBar<int>(700, 800);
            GenericMethod.GenericFoo<GenericClass<int>>(gci);
            GenericMethod.GenericBar<GenericClass<int>>(gci, 900);

            GenericOrder<int, GenericClass<int>, double> goigcid = new GenericOrder<int, GenericClass<int>, double>();
            goigcid.Foo(gci, 2.0, 1000);
            goigcid.Bar<string, int>(1100, "Hello", 2.0, 1200, gci);

            System.GC.Collect();

        }
    }


}

