// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.factorial
{
    using System;
    using System.Threading;

    struct vc
    {
        public int a, b, c;

        public vc(int a, int b, int c)
        {
            this.a = a;
            this.b = b;
            this.c = c;
        }

        public int f1()
        {
            int f = a + b;
            f += c;

            return f;
        }

        public override Boolean Equals(Object obj)
        {
            vc o = (vc)obj;
            return ((o.a == a) && (o.b == b) && (o.c == c));
        }

        public override int GetHashCode()
        {
            return a + b + c;
        }

    }


    class Factorial
    {
        public static int entryPoint(int f)
        {
            int g = 1;

            for (int i = 1; i <= f; i++)
            {
                g *= i;




                // Console.WriteLine(i);
            }

            return g;
        }

        static int tmp1 = 10;
        static Object staticObject;


        public static int factorial(String[] args)
        {
            vc vc1 = new vc(1, 2, 5);
            vc vcret = m11(3, 5, 7);

            int t1 = vc1.f1();

            Int32 vc2 = (Int32)81;
            int t1a = Convert.ToInt32(vc2);



            int t2 = 127;
            int t3 = tmp1;

            t3 = t3 + t2;


            int _foo = 5;

            _foo = _foo + 34;

            staticObject = new Object();

            double dA = 4.9E-20;

            if (dA == 0.0)
                dA = 1;

            int[][] mdia = new int[5][];
            for (int i = 0; i < 5; i++)
                mdia[i] = new int[6];

            mdia[0][0] = 1;
            mdia[1][1] = 2;
            mdia[2][2] = 3;
            mdia[3][3] = 4;
            mdia[4][4] = 5;

            int[,] mdia2 = new int[5, 6];

            for (int i = 0; i < 5; i++)
                for (int j = 0; j < 6; j++)
                    mdia2[i, j] = i * j;

            vc[,] vca = new vc[5, 6];

            for (int i = 0; i < 5; i++)
                for (int j = 0; j < 6; j++)
                    vca[i, j] = new vc(i, j, i * j);

            vc vc3 = vca[3, 4];

            char[] chArr2 = new char[9];

            for (int i = 0; i < 9; i++)
                chArr2[i] = (char)(i + 'a');

            int len = args.Length;

            int f = 42;
            int mm = 4;
            int mm1 = 5;

            int[] array1 = new int[10];

            for (int i = 0; i < 10; i++)
                array1[i] = i * 2;

            String[] aString = new String[10];

            int factorial = entryPoint(f);

            Factorial fac = new Factorial();

            int ii = fac.m1(f, 1, 2, 3);

            factorial = 75;
            int jj = fac.m1(f * 2, 1, 2, 3);

            jj = fac.m2();

            fac.o1 = new Object();

            for (int k = 1; k < 7; k++)
            {
                int d = fac.m4(k);

                mm++;
            }

            fac.m5();

            fac.m6(1);

            int z = fac.m7();

            fac.m8(vc1);
            fac.m9(vc1);
            fac.m10(mdia2, mdia2);

            vc vcret2 = m11(2, 4, 6);

            // Console.WriteLine("*** EXECUTION IS OVER ***");
            return 0;
        }

        public int m1(int a, int b, int c, int d)
        {
            this.a = a;
            this.d = a;

            return this.a;
        }

        public int m2()
        {
            int unused1;

            System.String s1 = "Hi there!";


            return a;
        }

        public int m3() { return b; }

        public int m4(int s)
        {
            int a;
            int b = 1;
            int c;
            int d = 1;

            return b + d + s;
        }

        public void m5()
        {
            int a = 4;
            int b = 5;
        }

        public void m6(int a)
        {
            if (a < 10)
                m6(a + 1);
        }

        public int m7()
        {
            return tmp1;
        }

        public void m8(Object o1)
        {
            int i = (((vc)o1).a + ((vc)o1).b + ((vc)o1).c);
        }

        public void m9(vc o1)
        {
            int i = (o1.a + o1.b + o1.c);
        }

        public void m10(Object o, int[,] array)
        {
            int x = array[0, 0];

            x += ((int[,])o)[0, 0];
        }

        public static vc m11(int a, int b, int c)
        {
            vc ret = new vc(a, b, c);

            return ret;
        }

        public vc m12(int a, int b, int c)
        {
            vc ret = new vc(a, b, c);

            return ret;
        }


        private int a, b, c;

        private Object o1;

        public int d, e, f;

        public vc vc2;


    } //end class Factorial


} //end of namespace

