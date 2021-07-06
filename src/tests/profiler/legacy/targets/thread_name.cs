// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.thread_name
{
    using System;
    using System.Threading;

    public class ThreadExample
    {

        public static void B()
        {
            System.Threading.Thread.CurrentThread.Name = "ThirdThread";
        }

        public static void A()
        {
            B();
        }

        public static void thread_name()
        {
            System.Threading.Thread.CurrentThread.Name = "MainThread";

            Thread t1 = new Thread(new ThreadStart(A));
            Thread t2 = new Thread(new ThreadStart(A));
            Thread t3 = new Thread(new ThreadStart(A));

            t1.Name = "SecondThread";
            t2.Start();
            t2.Join();
            t3.Name = null;
        }
    }
}

