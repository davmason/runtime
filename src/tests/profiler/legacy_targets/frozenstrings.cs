// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.frozenstrings
{
    using System.Runtime.CompilerServices;

class FrozenStrings
    {

        static string m_s1 = "Hello World";

        [MethodImpl(MethodImplOptions.NoInlining)]
        public static void frozenstrings()
        {
            string m_s2 = "Please freeze this string";

            System.Console.WriteLine("String 1: " + m_s1);
            System.Console.WriteLine("String 2: " + m_s2);
        }

    }


}

