// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System;
using System.IO;

namespace LegacyTestTargetsImpl.car
{
    class Car
    {
        ~Car()
        {
            Console.WriteLine("Car Finalizer has been called");
        }
    }

    class Program
    {
        public static void car(string[] args)
        {
            for (var n = 64; n-- > 0;)
                new Car();
            Console.WriteLine("I'm working...");
            for (var n = 4; n-- > 0;)
            {
                GC.Collect();
                GC.WaitForPendingFinalizers();
            }
        }
    }
}


