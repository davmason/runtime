// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.attachprofilerunit
{
    using System;
    using System.IO;
    using System.Threading;

    class AttachUnitTests
    {

        static void DelayForAttach()
        {
            Console.WriteLine("\n\t\tattachProfilerUnit.exe sleeping 15 seconds...");
            System.Threading.Thread.Sleep(1000 * 15);
            Console.WriteLine("\n\t\tattachProfilerUnit.exe awake.");
            /*
            while(true)
            {

            }
            */
        }

        static void ExceptionTest()
        {
            Console.WriteLine("ExceptionTest()");

            try
            {
                throw new MyException();
            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
            }
        }

        public static void attachprofilerunit(string[] args)
        {
            foreach (string s in args)
            {
                switch (s.ToLower())
                {
                    case "exception":
                        ExceptionTest();
                        break;

                    case "attachdelay":
                        DelayForAttach();
                        break;

                    default:
                        Console.WriteLine("Unknown unit test {0}", s);
                        break;
                }
            }
            Console.WriteLine("Exiting from attachProfilerUnit.exe");
        }
    }

    class MyException : Exception
    {
        public MyException()
        {
        }
    }


}

