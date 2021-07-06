// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.statics_domainneutral
{
    using System;
    using System.Threading;
    using System.Diagnostics;

    class MyStaticClass
    {
        [ThreadStaticAttribute]
        private static int myThreadStaticTestVar = 0xABC;

        //[ContextStaticAttribute]
        //private static int myContextStaticTestVar = 0xBCD;

        //private static int myAppdomainStaticTestVar = 0xDEF;

        //This is a little tricky example to create  a RVA static since there is no directly way to do so in C#
        //moved the RVA MyStaticClass case to another program RVAMyStaticClass.il
        //private static int[] myRVAStaticTestVar={0xCDE,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,9,10,
        //										 1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,9,10,
        //										 1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,9,10,1,2,3,4,5,6,7,8,9,10};
        //static MyStaticClass()
        //{
        //    //myThreadStaticTestVar = 0xABC;
        //    //myContextStaticTestVar = 0xBCD;
        //    //myAppdomainStaticTestVar = 0xDEF;
        //}      


        //[LoaderOptimization(LoaderOptimization.MultiDomain)]
        public static void statics_domainneutral()
        {
            Console.WriteLine("myThreadStaticTestVar MAIN");
            Thread.CurrentThread.Name = "myThreadStaticTestThread";
            //	AppDomain otherDomain = AppDomain.CreateDomain("myThreadStaticTestAppDomain");
            for (int i = 0; i < 2; ++i)
            {
                TimeSpan waitTime = new TimeSpan(0, 0, 1);
                MyStaticClass s = new MyStaticClass();
                //System.Console.WriteLine("Main AD: {0}", AppDomain.CurrentDomain.FriendlyName );
                //	    otherDomain.DoCallBack(new CrossAppDomainDelegate(MyCallBack));
                Thread[] threadPool = new Thread[10];
                for (int j = 0; j < 10; ++j)
                {
                    threadPool[j] = new Thread(MyCallBack);
                    threadPool[j].Start();
                }
                for (int j = 0; j < 10; ++j)
                {
                    threadPool[j].Join();
                }
                GC.Collect();
            }
            Console.WriteLine("myThreadStaticTestVar 0x{0:X}", myThreadStaticTestVar);
            //Console.WriteLine("myContextStaticTestVar 0x{0:X}", myContextStaticTestVar);
            //Console.WriteLine("myAppdomainStaticTestVar 0x{0:X}", myAppdomainStaticTestVar);
        }

        static public void MyCallBack()
        {
            GC.Collect();
            //GC.WaitForFullGCComplete();
            TimeSpan waitTime = new TimeSpan(0, 0, 1);
            Thread.Sleep(waitTime);
            MyStaticClass s = new MyStaticClass();
            MyStaticClass ps = s;
            //System.Console.WriteLine("Callback AD: {0}", AppDomain.CurrentDomain.FriendlyName );
            GC.Collect();
            //    GC.WaitForFullGCComplete();
            //Console.WriteLine("myThreadStaticTestVar 0x{0:X}", myThreadStaticTestVar);
            int i = myThreadStaticTestVar;
            ps = null;
            GC.Collect();
            //GC.WaitForFullGCComplete();
            Thread.Sleep(waitTime);
        }

    }

}

