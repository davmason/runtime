// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.gen_sharedcode
{
    class A<T>
    {
        T item;
        public void TestFunction(T p)
        {
            // A.F
            item = p;
        }
    }

    class B<T>
    {
        public void TestFunction()
        {
            // B.F
            System.Console.WriteLine("B.F");
        }
    }

    class C
    {
        public void TestFunction<T>(T p)
        {
            T lp;

            lp = p;

            System.Console.WriteLine(p.ToString());
        }
    }

    class Test
    {

        public static void gen_sharedcode()
        {
            System.Object so = new System.Object();
            System.String ss = "HELLO";
            System.Int32 si = 32;
            //System.Windows.Forms.Form swff = new System.Windows.Forms.Form();

            C c = new C();

            A<System.Object> aso = new A<System.Object>();
            A<System.String> ass = new A<System.String>();
            A<System.Int32> asi = new A<System.Int32>();
            //A<System.Windows.Forms.Form> aswff = new A<System.Windows.Forms.Form>();

            B<System.Object> bso = new B<System.Object>();
            B<System.String> bss = new B<System.String>();
            B<System.Int32> bsi = new B<System.Int32>();
            //B<System.Windows.Forms.Form> bswff = new B<System.Windows.Forms.Form>();

            aso.TestFunction(so);
            ass.TestFunction(ss);
            asi.TestFunction(si);
            //aswff.TestFunction(swff);

            bso.TestFunction();
            bss.TestFunction();
            bsi.TestFunction();
            //bswff.TestFunction();

            c.TestFunction<System.Object>(so);
            c.TestFunction<System.String>(ss);
            c.TestFunction<System.Int32>(si);
            //c.TestFunction<System.Windows.Forms.Form>(swff);
        }

    }
}

