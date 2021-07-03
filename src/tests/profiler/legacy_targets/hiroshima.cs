// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System;
using System.Threading;
using System.Globalization;


namespace LegacyTestTargetsImpl.hiroshima
{
    class AahExceptions
    {
        static int numOfExceptions = 5;
        static int depth = 6;


        public static void hiroshima(string[] args)
        {
            int threads = 250;


            if (args.Length > 0)
            {
                // command-line options:
                //   -t<number> -- number of threads (default is 250)
                //   -e<number> -- number of exceptions per thread (default is 5)
                //   -d<number> -- number of depth per exception group (default is 6)
                for (int i = 0; i < args.Length; i++)
                {
                    if (args[i].Length >= 2 && args[i][0] == '-')
                    {
                        switch (args[i][1])
                        {
                            case 't':
                                threads = Int32.Parse(args[i].Substring(2, args[i].Length - 2), CultureInfo.InvariantCulture);
                                break;

                            case 'e':
                                numOfExceptions = Int32.Parse(args[i].Substring(2, args[i].Length - 2), CultureInfo.InvariantCulture);
                                break;

                            case 'd':
                                depth = Int32.Parse(args[i].Substring(2, args[i].Length - 2), CultureInfo.InvariantCulture);
                                break;

                            default:
                                throw new Exception("Invalid Command-Line Arguments");
                        }
                    }
                    else
                        throw new Exception("Invalid Command-Line Arguments");
                }
            }


            // first throw the exceptions from threads
            FloodExceptions(threads);

        }

        //
        //- Nested Exceptions --------------------------------------------------
        // 
        static void NestedExceptions()
        {
            if (depth > 0)
            {
                Console.WriteLine("Burst of nested exceptions");
                depth = depth - 5;
                try
                {
                    try
                    {
                        throw new MyException("Nested Exception - depth 0");
                    }
                    catch (MyException)
                    {
                        try
                        {
                            throw new MyException1("Nested Exception - depth 1");
                        }
                        catch (MyException1)
                        {
                            try
                            {
                                throw new MyException1("Nested Exception - depth 2");
                            }
                            catch (MyException1)
                            {
                                try
                                {
                                    throw new MyException1("Nested Exception - depth 3");
                                }
                                catch (MyException1)
                                {
                                    try
                                    {
                                        throw new MyException("Nested Exception - depth 4");
                                    }
                                    catch (MyException)
                                    {
                                        try
                                        {
                                            throw new MyException1("Nested Exception - depth 5");
                                        }
                                        catch (MyException1)
                                        {
                                            try
                                            {
                                                throw new MyException1("Nested Exception - depth 6");
                                            }
                                            finally
                                            {
                                                throw new MyException("");
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                catch (MyException1)
                {
                    Console.WriteLine("Should Never Happen!!!");
                }
                catch (MyException)
                {
                    NestedExceptions();
                }
            }
        }


        static void FloodExceptions(int i_nThreads)
        {
            System.Console.WriteLine("Exception Flooding Test for {0} treads", i_nThreads);
            Thread[] t = new Thread[i_nThreads];
            for (int n = 0; n < i_nThreads; n++)
            {
                t[n] = new Thread(new ThreadStart(Flood));
                t[n].Start();
            }

            //Wait for the threads to end first
            for (int n = 0; n < i_nThreads; n++)
            {
                t[n].Join();
            }

        }

        static void Flood()
        {
            for (int n = 0; n < numOfExceptions; n++)
            {
                try
                {
                    ThisThrows();

                }
                catch (MyException ex)
                {
                    String s = ex.ToString();
                    NestedExceptions();
                }
            }
        }

        static void ThisThrows()
        {
            throw new MyException("Flood ...");
        }
    }


    class MyException : Exception
    {
        protected String m_strMsg = "What?";
        public MyException(String i_strMsg)
        {
            m_strMsg = i_strMsg;
        }
        public string GetMessage()
        {
            return "MyException::" + m_strMsg;
        }
    }

    class MyException1 : MyException
    {
        public MyException1(String i_strMsg) : base(i_strMsg)
        {
        }
        new public string GetMessage()
        {
            return "MyException1::" + m_strMsg;
        }
    }
}


