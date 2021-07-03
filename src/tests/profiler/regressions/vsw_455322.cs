// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System;
using System.Linq;
using System.Collections.Generic;

namespace Profiler.Tests
{
    class vsw_455322
    {
        static readonly Guid VSW_455322Guid = new Guid("TODO: add guid");
        public static int Main(string[] args)
        {
            if (args.Length > 0 && args[0].Equals("RunTest", StringComparison.OrdinalIgnoreCase))
            {
                string[] newArgs = args.Length > 1 ? Enumerable.TakeLast(args, args.Length - 1).ToArray() : new string[] { };
                return LegacyTestTargets.fibonacci(newArgs);
            }

            return ProfilerTestRunner.Run(profileePath: System.Reflection.Assembly.GetExecutingAssembly().Location,
                                          testName: "VSW_455322",
                                          profilerClsid: VSW_455322Guid);
        }
    }
}


