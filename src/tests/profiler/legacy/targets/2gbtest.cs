// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl._2gbtest
{
    using System;
    using System.Collections;
    using System.Collections.Generic;

    struct OneByte
    {
        byte x;
    };

    struct Byte13
    {
        public byte b1;
        public byte b2;
        public byte b3;
        public byte b4;
        public byte b5;
        public byte b6;
        public byte b7;
        public byte b8;
        public byte b9;
        public byte b10;
        public byte b11;
        public byte b12;
        public byte b13;
    };

    class My
    {
        const int MaxStringLength = 0x3FFFFFDF;
        const int MaxDimension = 0X7FEFFFFF;
        const int MaxByteDimension = 0X7FFFFFC7;

        static void TestStringLimits()
        {
            Console.WriteLine("TestStringLimits");

            try
            {
                String s = new String('a', MaxStringLength + 1);

                // Trigger profiler heap walk
                System.GC.Collect();

                GC.KeepAlive(s);

                Console.WriteLine("new String('a', MaxStringLength+1) succeeded unexpectedly");
                failures++;
            }
            catch (OutOfMemoryException)
            {
            }

            try
            {
                String s = new String('a', MaxStringLength);

                // Trigger profiler heap walk
                System.GC.Collect();

                GC.KeepAlive(s);
            }
            catch
            {
                Console.WriteLine("new String('a', MaxStringLength) failed");
                failures++;
            }
        }

        static void Test1DByteLimits()
        {
            Console.WriteLine("Test1DByteLimits");

            try
            {
                byte[] b = new byte[MaxByteDimension + 1];

                // Trigger profiler heap walk
                System.GC.Collect();

                GC.KeepAlive(b);

                Console.WriteLine("new byte[MaxByteDimension+1] succeeded unexpectedly");
                failures++;
            }
            catch (OutOfMemoryException)
            {
            }

            try
            {
                byte[] b = new byte[MaxByteDimension];

                // Trigger profiler heap walk
                System.GC.Collect();

                GC.KeepAlive(b);
            }
            catch
            {
                Console.WriteLine("new byte[MaxByteDimension] failed");
                failures++;
            }
        }

        static void Test2DByteLimits()
        {
            Console.WriteLine("Test2DByteLimits");

            try
            {
                byte[,] b = new byte[MaxByteDimension + 1, 2];

                // Trigger profiler heap walk
                System.GC.Collect();

                GC.KeepAlive(b);

                Console.WriteLine("new byte[MaxByteDimension+1,2] succeeded unexpectedly");
                failures++;
            }
            catch (OutOfMemoryException)
            {
            }

            try
            {
                byte[,] b = new byte[MaxByteDimension, 2];

                // Trigger profiler heap walk
                System.GC.Collect();

                GC.KeepAlive(b);
            }
            catch
            {
                Console.WriteLine("new byte[MaxByteDimension, 2] failed");
                failures++;
            }
        }

        static void Test1DOneByteStructLimits()
        {
            Console.WriteLine("Test1DOneByteStructLimits");

            try
            {
                OneByte[] b = new OneByte[MaxByteDimension + 1];

                // Trigger profiler heap walk
                System.GC.Collect();

                GC.KeepAlive(b);

                Console.WriteLine("new OneByte[MaxByteDimension+1] succeeded unexpectedly");
                failures++;
            }
            catch (OutOfMemoryException)
            {
            }

            try
            {
                OneByte[] b = new OneByte[MaxByteDimension];

                // Trigger profiler heap walk
                System.GC.Collect();

                GC.KeepAlive(b);
            }
            catch
            {
                Console.WriteLine("new OneByte[MaxByteDimension] failed");
                failures++;
            }
        }

        static void Test2DOneByteStructLimits()
        {
            Console.WriteLine("Test2DOneByteStructLimits");

            try
            {
                OneByte[,] b = new OneByte[MaxByteDimension + 1, 2];

                // Trigger profiler heap walk
                System.GC.Collect();

                GC.KeepAlive(b);

                Console.WriteLine("new OneByte[MaxByteDimension+1,2] succeeded unexpectedly");
                failures++;
            }
            catch (OutOfMemoryException)
            {
            }

            try
            {
                OneByte[,] b = new OneByte[MaxByteDimension, 2];

                // Trigger profiler heap walk
                System.GC.Collect();

                GC.KeepAlive(b);
            }
            catch
            {
                Console.WriteLine("new OneByte[MaxByteDimension, 2] failed");
                failures++;
            }
        }

        static void Test1DIntLimits()
        {
            Console.WriteLine("Test1DIntLimits");

            try
            {
                int[] b = new int[MaxDimension + 1];

                // Trigger profiler heap walk
                System.GC.Collect();

                GC.KeepAlive(b);

                Console.WriteLine("new int[MaxDimension+1] succeeded unexpectedly");
                failures++;
            }
            catch (OutOfMemoryException)
            {
            }

            try
            {
                int[] b = new int[MaxDimension];

                // Trigger profiler heap walk
                System.GC.Collect();

                GC.KeepAlive(b);
            }
            catch
            {
                Console.WriteLine("new int[MaxDimension] failed");
                failures++;
            }
        }

        static void Test2DIntLimits()
        {
            Console.WriteLine("Test2DIntLimits");

            try
            {
                int[,] b = new int[MaxDimension + 1, 2];

                // Trigger profiler heap walk
                System.GC.Collect();

                GC.KeepAlive(b);

                Console.WriteLine("new int[MaxDimension+1,2] succeeded unexpectedly");
                failures++;
            }
            catch (OutOfMemoryException)
            {
            }

            try
            {
                int[,] b = new int[MaxDimension, 2];

                // Trigger profiler heap walk
                System.GC.Collect();

                GC.KeepAlive(b);
            }
            catch
            {
                Console.WriteLine("new int[MaxDimension, 2] failed");
                failures++;
            }
        }

        static void GetSet1DIntTest()
        {
            Console.WriteLine("GetSet1DIntTest");

            int[] a = new int[2000000000];
            a[0x66666666] = 0x12345678;
            if ((int)a.GetValue(0x66666666) != 0x12345678)
            {
                Console.WriteLine("(int)a.GetValue(0x66666666) != 0x12345678");
                failures++;
            }
            a.SetValue(0x76543210, 0x67676767);
            if (a[0x67676767] != 0x76543210)
            {
                Console.WriteLine("a[0x67676767] != 0x76543210");
                failures++;
            }
        }

        static void GetSet10DIntTest()
        {
            Console.WriteLine("GetSet10DIntTest");

            int[,,,,,,,,,] a = new int[4, 10, 10, 10, 10, 10, 10, 10, 10, 10];

            a[3, 9, 9, 9, 9, 9, 9, 9, 9, 9] = 0x12345678;
            if ((int)a.GetValue(3, 9, 9, 9, 9, 9, 9, 9, 9, 9) != 0x12345678)
            {
                Console.WriteLine("(int)a.GetValue(3,9,9,9,9,9,9,9,9,9) != 0x12345678");
                failures++;
            }
            a.SetValue(0x67676767, 3, 9, 9, 9, 9, 9, 9, 9, 9, 8);
            if (a[3, 9, 9, 9, 9, 9, 9, 9, 9, 8] != 0x67676767)
            {
                Console.WriteLine("a[3,9,9,9,9,9,9,9,9,8] != 0x67676767");
                failures++;
            }
        }

        /*
            static void TestArrayList()
            {
                Console.WriteLine("TestArrayList");

                long sum1 = 0, sum2 = 0;

                var c = new ArrayList();

                try
                {
                    Object one = (Object)1;
                    for (int i = 0; i < Int32.MaxValue; i++)
                    {
                        if (i % 10000000 < 100000)
                        {
                            c.Add(i);
                            // Create some garbage to stress GC compaction
                            GC.KeepAlive(new Object()); 
                            sum1 += i;
                        }
                        else {
                            c.Add(one);
                            sum1 += 1;
                        }

                    }
                }
                catch(OutOfMemoryException) {
                }

                if (c.Count < 2000000000)
                {
                    Console.WriteLine("failed for less than 2000000000 elements: " + c.Count);
                    failures++;
                }

                foreach (var e in c)
                {
                    sum2 += (int)e;
                }

                if (sum1 != sum2)
                {
                    Console.WriteLine("sum1 != sum2: " + sum1.ToString() + " " + sum2.ToString());
                    failures++;
                }
            }

            static void TestHashtable()
            {
                Console.WriteLine("TestHashtable");

                long sum1 = 0, sum2 = 0;

                var c = new Hashtable();

                try
                {
                    for (int i = 0; i < Int32.MaxValue; i++)
                    {
                        Object o = (Object)i;
                        c.Add(o,o);
                        sum1 += i;
                    }
                }
                catch(ArgumentException) {
                }

                if (c.Count < 2000000000)
                {
                    Console.WriteLine("failed for less than 2000000000 elements: " + c.Count);
                    failures++;
                }

                foreach (var e in c)
                {
                    sum2 += (int)e;
                }

                if (sum1 != sum2)
                {
                    Console.WriteLine("sum1 != sum2: " + sum1.ToString() + " " + sum2.ToString());
                    failures++;
                }
            }

            static void TestList()
            {
                Console.WriteLine("TestList");

                long sum1 = 0, sum2 = 0;

                var c = new List<Byte13>();
                try
                {
                    for (int i = 0; i < Int32.MaxValue; i++)
                    {
                        Byte13 v = new Byte13();
                        v.b3 = (byte)i;
                        v.b5 = (byte)(i >> 8);
                        v.b7 = (byte)(i >> 16);
                        v.b11 = (byte)(i >> 24);

                        c.Add(v);
                        sum1 += i;
                    }
                }
                catch(OutOfMemoryException) {
                }

                if (c.Count < 2000000000)
                {
                    Console.WriteLine("failed for less than 2000000000 elements: " + c.Count);
                    failures++;
                }

                foreach (var e in c)
                {
                    sum2 += (int)e.b3 + ((int)e.b5 << 8) + ((int)e.b7 << 16) + ((int)e.b11 << 24);
                }

                if (sum1 != sum2)
                {
                    Console.WriteLine("sum1 != sum2: " + sum1.ToString() + " " + sum2.ToString());
                    failures++;
                }
            }

            static void TestDictionary()
            {
                Console.WriteLine("TestDictionary");

                long sum1 = 0, sum2 = 0;

                var c = new Dictionary<int, int>();
                try
                {
                    for (int i = 0; i < Int32.MaxValue; i++)
                    {
                        c.Add(i, i);
                        sum1 += i;
                    }
                }
                catch(ArgumentException) {
                }

                if (c.Count < 2000000000)
                {
                    Console.WriteLine("failed for less than 2000000000 elements: " + c.Count);
                    failures++;
                }

                foreach (var e in c)
                {
                    sum2 += e.Value;
                }

                if (sum1 != sum2)
                {
                    Console.WriteLine("sum1 != sum2: " + sum1.ToString() + " " + sum2.ToString());
                    failures++;
                }
            }

            static void TestConcurentDictionary()
            {
                Console.WriteLine("TestConcurentDictionary");

                long sum1 = 0, sum2 = 0;

                var c = new ConcurrentDictionary<int, int>();

                try
                {
                    for (int i = 0; i < Int32.MaxValue; i++)
                    {
                        c.TryAdd(i, i);
                        sum1 += i;
                    }
                }
                catch(Exception e) {
        Console.WriteLine(e);
                }

                if (c.Count < 2000000000)
                {
                    Console.WriteLine("failed for less than 2000000000 elements: " + c.Count);
                    failures++;
                }

                foreach (var e in c)
                {
                    sum2 += e.Value;
                }

                if (sum1 != sum2)
                {
                    Console.WriteLine("sum1 != sum2: " + sum1.ToString() + " " + sum2.ToString());
                    failures++;
                }
            }
        */
        static int failures;

        public static int _2gbtest()
        {

            TestStringLimits();

            Test1DByteLimits();
            Test2DByteLimits();

            Test1DOneByteStructLimits();
            Test2DOneByteStructLimits();

            Test1DIntLimits();
            Test2DIntLimits();

            GetSet1DIntTest();
            GetSet10DIntTest();

            // TestArrayList();
            // TestHashtable();
            // TestList();
            // TestDictionary();
            // TestConcurentDictionary();
            // TestHashSet();

            if (failures != 0)
            {
                Console.WriteLine(failures.ToString() + " tests FAILED");
                return -1;
            }

            Console.WriteLine("PASSED");
            return 100;
        }
    }

}

