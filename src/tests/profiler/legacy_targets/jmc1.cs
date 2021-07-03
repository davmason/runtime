// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.jmc1
{
    // Test for JMC
    // Methods beginning w/ A are user code.
    using System;

    class Foo
    {

        public static void jmc1()
        {
            for (int i = 0; i < 10; i++)
            {
                Console.WriteLine("In Main, {0}", i);
                A1();
                Console.WriteLine("end of Main");
            }
        }

        static void A1()
        {
            Console.WriteLine("In A1 <");
            B1();
            Console.WriteLine("In A1 >");
        }

        static void B1()
        {
            Console.WriteLine("In B1 <");
            A2();
            Console.WriteLine("In B1 |");
            B2();
            Console.WriteLine("In B1 >");
        }

        static void A2()
        {
            Console.WriteLine("In A2!");
            // Recursion test
            int l = R(3);
            Console.WriteLine("End of A2, l = {0}", l);
        }

        static int R(int x)
        {
            Console.WriteLine("in R({0})", x);
            if (x == 0)
                return 1;

            // Make a recursive call here!
            int y = R(x - 1);

            return y + 1;
        }

        static void B2()
        {
            Console.WriteLine("In B2 <");
            A3();
            Console.WriteLine("In B2 >");
        }

        static void A3()
        {
            Console.WriteLine("In A3!");
        }


    }
}

