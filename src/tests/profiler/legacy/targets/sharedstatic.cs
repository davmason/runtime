// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.gen_sharedstatics
{
    class StaticGeneric<T>
    {
        public static int i;
        public static T t;
    }


    class SharedTest
    {
        public static void gen_sharedstatics()
        {
            StaticGeneric<int>.i = 200;
            StaticGeneric<int>.t = 400;

            StaticGeneric<int> sgi1 = new StaticGeneric<int>();
            StaticGeneric<int> sgi2 = new StaticGeneric<int>();

            StaticGeneric<int>.i = 500;
            StaticGeneric<int>.i = 700;

        }
    }
}

