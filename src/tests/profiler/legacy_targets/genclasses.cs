// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.genclasses
{
    using System.Diagnostics;

    struct SV { public int x; }

    class A<T> { public T item; }

    class B<T>
    {
        T item;
        public void F(T p)
        {
            if (Debugger.IsAttached)
                Debugger.Break();
            // B.F
            item = p;
        }
    }

    class C<T1, T2>
    {
        T1 item1;
        T2 item2;
        public bool item3;

        public void F1(T1 p1)
        {
            if (Debugger.IsAttached)
                Debugger.Break();

            // C.F1
            item1 = p1;
            item3 = true;
        }

        public void F2(T2 p2)
        {
            if (Debugger.IsAttached)
                Debugger.Break();

            // C.F2
            item2 = p2;
            item3 = true;
        }

        public void F3(T1 p1, T2 p2)
        {
            if (Debugger.IsAttached)
                Debugger.Break();

            // C.F3
            item1 = p1;
            item2 = p2;
            item3 = false;
        }
    }

    interface I<T>
    {
        void IF1(T p);
    }

    class D<T1, T2, T3> : C<T1, T2>, I<T3>
    {
        public T1 DF1(T1 p1, T2 p2, T3 p3)
        {
            if (Debugger.IsAttached)
                Debugger.Break();

            // D.DF1
            return p1;
        }

        public void IF1(T3 p)
        {
            if (Debugger.IsAttached)
                Debugger.Break();

            // D.IF1
            p = p;
        }
    }

    class E<T> : C<string, T> { }

    class F<T1, T2> : E<T2> { }

    class Z
    {
        static int staticitem;
        int item;
        public void F(int p)
        {
            staticitem = p;
            item = p + p;
        }
    }


    class Test
    {
        public static void genclasses()
        {
            int i1 = 1;
            int i2 = 2;
            double d1 = 3.3;
            double d2 = 4.4;
            string s1 = "foo";
            string s2 = "bar";
            bool b = false;
            char c = 'c';
            SV sv = new SV();
            int[,] ia = new int[5, 5];
            bool[] ba1 = new bool[2];
            bool[,] ba2 = new bool[2, 2];
            bool[,,,,] ba5 = new bool[2, 2, 2, 2, 2];

            A<int> ai = new A<int>();
            A<double> ad = new A<double>();
            A<string> ast = new A<string>();
            A<bool> ab = new A<bool>();
            ai.item = 0;

            B<int> bi1 = new B<int>();
            B<int> bi2 = new B<int>();
            B<int> bi3 = new B<int>();
            B<int> bi4 = new B<int>();
            B<int> bi5 = new B<int>();
            B<double> bd = new B<double>();
            B<string> bs = new B<string>();
            B<bool> bb = new B<bool>();
            B<char> bc = new B<char>();

            C<int, double> cid = new C<int, double>();
            C<double, int> cdi = new C<double, int>();
            C<string, string> css1 = new C<string, string>();
            C<string, string> css2 = new C<string, string>();
            C<string, string> css3 = new C<string, string>();
            C<string, string> css4 = new C<string, string>();
            C<string, string> css5 = new C<string, string>();

            C<A<bool>, B<char>> big1 = new C<A<bool>, B<char>>();
            A<C<A<bool>, B<char>>> big2 = new A<C<A<bool>, B<char>>>();
            B<A<C<A<bool>, B<char>>>> big3 = new B<A<C<A<bool>, B<char>>>>();

            E<int> ei = new E<int>();
            E<double> ed = new E<double>();

            D<int, double, string> dids = new D<int, double, string>();
            D<int, double, bool> didb = new D<int, double, bool>();

            F<string, bool> fsb = new F<string, bool>();

            A<SV> asv = new A<SV>();
            A<double[]> ada = new A<double[]>();

            Z boring = new Z();
            Z z1 = new Z();
            Z z2 = new Z();

            A<int>[] aia1 = new A<int>[10];
            A<int>[,] aia2 = new A<int>[10, 10];
            A<int>[,,] aia3 = new A<int>[10, 10, 10];

            if (Debugger.IsAttached)
                Debugger.Break();

            bi1.F(i1);
            bi2.F(i2);
            bd.F(d1);
            bs.F(s1);
            bb.F(b);
            bc.F(c);
            cid.F3(i1, d2);
            cdi.F3(d1, i2);
            css1.F3(s1, s2);
            big1.F3(ab, bc);
            sv.x = 100;
            dids.DF1(i1, d1, s1);
            dids.F1(i1);
            dids.F3(i1, d1);
            dids.IF1(s1);
            z1.F(i1);
            z2.F(i2);


        }
    }


}

