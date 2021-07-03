// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.gen_factorial
{
    using System;
    using System.Threading;

    interface vcBase
    {
        void Init(int a, int b, int c);
        int f1();
        int A
        {
            get;
            set;
        }
        int B
        {
            get;
            set;
        }
        int C
        {
            get;
            set;
        }
    }

    struct vc : vcBase
    {
        private int p_a, p_b, p_c;
        public int A
        {
            get
            {
                return p_a;
            }
            set
            {
                p_a = value;
            }
        }
        public int B
        {
            get
            {
                return p_b;
            }
            set
            {
                p_b = value;
            }

        }
        public int C
        {
            get
            {
                return p_c;
            }
            set
            {
                p_c = value;
            }

        }
        public void Init(int a, int b, int c)
        {
            this.p_a = a;
            this.p_b = b;
            this.p_c = c;
        }

        public int f1()
        {
            int f = p_a + p_b;
            f += p_c;

            return f;
        }

        public override Boolean Equals(Object obj)
        {
            vc o = (vc)obj;
            return ((o.A == p_a) && (o.B == p_b) && (o.C == p_c));
        }

        public override int GetHashCode()
        {
            return p_a + p_b + p_c;
        }

    }


    class Factorial<T> where T : vcBase
    {
        public static int entryPoint(int f)
        {
            int g = 1;

            for (int i = 1; i <= f; i++)
            {
                g *= i;

                if (sleepOrNot != 0)
                    Thread.Sleep(1000);

                // Console.WriteLine(i);
            }

            return g;
        }

        static int tmp1 = 10;
        static Object staticObject;
        static int sleepOrNot = 0;

        public static int OldMain(String[] args, T newT)
        {
            T vc1 = newT;
            vc1.Init(1, 2, 5);

            T vcret = m11(3, 5, 7, newT);

            int t1 = vc1.f1();

            Int32 vc2 = (Int32)81;
            int t1a = Convert.ToInt32(vc2);

            sleepOrNot = args.Length;

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

            T[,] vca = new T[5, 6];

            for (int i = 0; i < 5; i++)
                for (int j = 0; j < 6; j++)
                {
                    vca[i, j] = newT;
                    vca[i, j].Init(i, j, i * j);
                }

            T vc3 = vca[3, 4];

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

            Factorial<vc> fac = new Factorial<vc>();

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

            T vcret2 = m11(2, 4, 6, newT);

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
            int i = (((T)o1).A + ((T)o1).B + ((T)o1).C);
        }

        public void m9(vcBase o1)
        {
            int i = (o1.A + o1.B + o1.C);
        }

        public void m10(Object o, int[,] array)
        {
            int x = array[0, 0];

            x += ((int[,])o)[0, 0];
        }

        public static T m11(int a, int b, int c, T newT)
        {
            T ret = newT;
            ret.Init(a, b, c);

            return ret;
        }

        public T m12(int a, int b, int c, T newT)
        {
            T ret = newT;
            ret.Init(a, b, c);

            return ret;
        }


        private int a, b, c;

        private Object o1;

        public int d, e, f;

        public T vc2;


    } //end class Factorial

    class Test
    {
        public static int gen_factorial(String[] args)
        {
            return Factorial<vc>.OldMain(args, new vc());
        }
    }

} //end of namespace

