// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.tieredcompile
{
    using System;
    using System.Diagnostics;
    using System.Runtime.CompilerServices;
    using System.Threading;
    using System.Threading.Tasks;

    public class Program
    {
        public static void tieredcompile(string[] args)
        {
            TierUp();
        }


        [MethodImpl(MethodImplOptions.NoOptimization)]
        public static void TierUp()
        {
            // Run the rejittable method a bunch to almost trigger rejit
            for (int i = 0; i < 200 - 1; i++)
            {
                MethodThatWillBeVersioned(i);
                // Sleep here to guarantee that the tiered method will execute. Without a sleep here
                // the loop may finish before the new method is finished jitting and patched in.
                Thread.Sleep(1);
            }
        }

        [MethodImpl(MethodImplOptions.NoInlining)]
        static int MethodThatWillBeVersioned(int arg)
        {
            int x = arg;
            int ret = 0;
            while (x > 0)
            {
                if (x % 2 == 1)
                {
                    x--;
                }
                else
                {
                    x /= 2;
                }
                ret++;
            }
            return ret;
        }
    }

}

