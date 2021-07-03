// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.finalizer
{
    using System;

    class Finalizer
    {
        private char[] data;
        private static Finalizer resururector;
        private static int MAX = 10000;
        private int index;

        public Finalizer(int i)
        {
            index = i;
            data = new char[i];
        }

        ~Finalizer()
        {
            // For every 10 objects, resurect 1 object
            // this is cover the resurecting scenario of finalizer objects.
            if (index % 10 == 0)
            {
                resururector = this;
            }

        }

        public static void finalizer()
        {
            for (int i = 0; i < MAX; ++i)
            {
                Finalizer f = new Finalizer(i * 16);
                GC.Collect();
                GC.WaitForPendingFinalizers();

            }

        }

    }

}

