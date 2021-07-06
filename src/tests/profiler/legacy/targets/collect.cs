// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.collect
{
    using System;
    // using System.Runtime.ConstrainedExecution;
    using System.Runtime.InteropServices;
    using System.Runtime.CompilerServices;

    class fin
    {
        static int finCount;

        ~fin()
        {
            finCount++;
        }
    }

    class critfin // : CriticalFinalizerObject
    {
        static int critfinCount;

        ~critfin()
        {
            critfinCount++;
        }
    }


    class collect
    {
        [MethodImplAttribute(MethodImplOptions.NoInlining)]
        static void PassByRef(ref byte b)
        {
            GC.Collect();
            b = 3;
        }

        [MethodImplAttribute(MethodImplOptions.NoInlining)]
        static void AllocateGarbage()
        {
            for (int i = 0; i < 50; i++)
            {
                byte[] b = new byte[1000];
            }
        }

        static int s1;
        static object s2, s3, s4, s5, s6;

        public static void runCollect()
        {
            GC.Collect();

            new fin();
            new critfin();

            GC.Collect();

            GC.WaitForPendingFinalizers();


            GC.Collect();


            new fin();

            string s = new string(' ', 10);
            WeakReference w = new WeakReference(s);
            AllocateGarbage();

            int sum = 0;
            byte[] bb = new byte[10 * 1000];

            unsafe
            {
                fixed (char* cp = s)
                {
                    PassByRef(ref bb[13]);
                    for (char* x = cp; *x != 0; x++)
                        sum += *x;
                }
            }

            GC.WaitForPendingFinalizers();

            byte[] bbb = new byte[100 * 1000];
            byte[] b = new byte[100 * 1000];
            byte[] bbbb = new byte[100 * 1000];

            GC.Collect();

            s3 = w;
            w = null;

            GC.Collect();

            GC.WaitForPendingFinalizers();

            GC.Collect();

            s1 = sum;
            s2 = s;
            s3 = w;
            s4 = bb;
            s5 = bbb;
            s6 = bbbb;

            GC.Collect();

            GC.WaitForPendingFinalizers();

            GC.Collect();

            s2 = null;
            s3 = null;
            s4 = null;
            s5 = null;
            s6 = null;

            GCHandle[] wr = new GCHandle[100];
            for (var i = 0; i < 100; i++)
            {
                wr[i] = GCHandle.Alloc(s1);
            }
            for (var i = 0; i < 100; i++)
            {
                wr[i].Free();
            }

            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();

            GC.Collect();
            GC.WaitForPendingFinalizers();
            GC.Collect();
        }
    }
}

