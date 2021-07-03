// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.fibonacci
{
    using System;
    using System.Threading;

    class fib
    {
        public void CalculateFib()
        {
            ulong i = 0;
            ulong j = 1;
            ulong k = 0;

            // Console.WriteLine("Fibonacci calculation started.");

            for (ulong l = 0UL; l < 1000000000UL; l++) // 1000000000
            {
                // Useless math
                k = k - i * j;

                // The fib part
                k = i + j;
                i = j;
                j = k;
            }

            Console.WriteLine("Fibonacci calculation finished = {0}", k);
        }

        public void CalculateFibExtraMath()
        {
            ulong i = 0;
            ulong j = 1;
            ulong k = 0;

            // Console.WriteLine("Fibonacci calculation started.");

            for (ulong l = 0UL; l < 1000000000UL; l++) // 1000000000
            {
                // More Useless math
                k = k - i * j;
                k = k + i / j;

                // The fib part
                k = i + j;
                i = j;
                j = k;
            }

            Console.WriteLine("Fibonacci calculation finished = {0}", k);
        }

    }

    class test
    {
        public static void fibonacci(string[] args)
        {

            System.Console.WriteLine("Starting fibonacci.");

            Thread[] fibThreads;
            fibThreads = new Thread[5];

            fib[] fibArray;
            fibArray = new fib[5];

            fibArray[0] = new fib();
            fibArray[1] = new fib();
            fibArray[2] = new fib();
            fibArray[3] = new fib();
            fibArray[4] = new fib();

            if (args.Length == 0)
            {
                fibThreads[0] = new Thread(new ThreadStart(fibArray[0].CalculateFib));
                fibThreads[1] = new Thread(new ThreadStart(fibArray[1].CalculateFib));
                fibThreads[2] = new Thread(new ThreadStart(fibArray[2].CalculateFib));
                fibThreads[3] = new Thread(new ThreadStart(fibArray[3].CalculateFib));
                fibThreads[4] = new Thread(new ThreadStart(fibArray[4].CalculateFib));
            }
            else
            {
                fibThreads[0] = new Thread(new ThreadStart(fibArray[0].CalculateFibExtraMath));
                fibThreads[1] = new Thread(new ThreadStart(fibArray[1].CalculateFibExtraMath));
                fibThreads[2] = new Thread(new ThreadStart(fibArray[2].CalculateFibExtraMath));
                fibThreads[3] = new Thread(new ThreadStart(fibArray[3].CalculateFibExtraMath));
                fibThreads[4] = new Thread(new ThreadStart(fibArray[4].CalculateFibExtraMath));
            }

            fibThreads[0].Start();
            fibThreads[1].Start();
            fibThreads[2].Start();
            fibThreads[3].Start();
            fibThreads[4].Start();

            fibThreads[0].Join();
            fibThreads[1].Join();
            fibThreads[2].Join();
            fibThreads[3].Join();
            fibThreads[4].Join();

            System.Console.WriteLine("Fibonacci complete.");
        }

    }

}

