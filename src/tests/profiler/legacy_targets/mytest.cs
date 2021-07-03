// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.mytest
{
    using System;

    struct OneTwoThreeFour
    {
        int one;
        int two;
        int three;
        int four;

        public OneTwoThreeFour(int dummy)
        {
            one = 1;
            two = 2;
            three = 3;
            four = 4;
        }
    }

    class My
    {

        static void foo(OneTwoThreeFour s1, OneTwoThreeFour s2, OneTwoThreeFour s3, OneTwoThreeFour s4, OneTwoThreeFour s5)
        {
        }

        public static void mytest()
        {
            foo(new OneTwoThreeFour(0), new OneTwoThreeFour(0), new OneTwoThreeFour(0), new OneTwoThreeFour(0), new OneTwoThreeFour(0));
        }

    }
}

