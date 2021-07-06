// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.floatingpointregisters
{
    using System;
    using System.Threading;
    using System.Diagnostics;

    class My
    {

        static void GCWorker()
        {
            for (int i = 0; i < 100; i++) { GC.Collect(); Thread.Sleep(0); }
        }

        static double s_x;

        static double foo(double x)
        {
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;
            s_x += x;


            return x;
        }

        public static void floatingpointregisters()
        {
            Console.WriteLine("FloatingPointRegisters Starting");

            new Thread(GCWorker).Start();

            bool pass = true;

            for (int i = 0; i < 10000000; i++)
            {
                if (foo(8.0) != 8.0)
                {
                    pass = false;
                    Console.Write(".");
                }
            }

            Console.WriteLine("FloatingPointRegisters Finished.  Setting EnvVer Value.");

            if (!pass)
            {
                throw new Exception("FloatingPointRegisters didn't pass");
            }
        }

    }

}

