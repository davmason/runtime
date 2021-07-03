// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System;
using System.Linq;
using System.Collections.Generic;

namespace Profiler.Tests
{
    class vsw_256310
    {
        static readonly Guid VSW_256310Guid = new Guid("TODO: add guid");
        public static int Main(string[] args)
        {
            if (args.Length > 0 && args[0].Equals("RunTest", StringComparison.OrdinalIgnoreCase))
            {
                string[] newArgs = args.Length > 1 ? Enumerable.TakeLast(args, args.Length - 1).ToArray() : new string[] { };
                return LegacyTestTargets.genclasses(newArgs);
            }

            return ProfilerTestRunner.Run(profileePath: System.Reflection.Assembly.GetExecutingAssembly().Location,
                                          testName: "VSW_256310",
                                          profilerClsid: VSW_256310Guid);
        }
    }
}


