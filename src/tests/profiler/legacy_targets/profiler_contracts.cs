// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.profiler_contracts
{
    using System;
    using System.Threading;


    class My
    {

        static My()
        {
            Thread.CurrentThread.Name = "Hello world";

            //AppDomain app = AppDomain.CreateDomain("Hello world");
            //AppDomain.Unload(app);

            try
            {
                try
                {
                    throw new Exception();
                }
                finally
                {
                    Console.WriteLine("In finally");
                }
            }
            catch (Exception e)
            {
                Console.WriteLine("In catch");
            }

            GC.Collect();
        }

        public static void profiler_contracts()
        {
            new My().ToString();
        }

    }

}

