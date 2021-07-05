// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System;
using System.Linq;
using System.Collections.Generic;

namespace Profiler.Tests
{
    class shadowstackfastwithdsselt32
    {
        static readonly Guid LegacyProfilerGuid = new Guid("465F1659-E372-4A7F-825E-153B227BA671");
        public static int Main(string[] args)
        {
            if (args.Length > 0 && args[0].Equals("RunTest", StringComparison.OrdinalIgnoreCase))
            {
                string[] newArgs = args.Length > 1 ? Enumerable.TakeLast(args, args.Length - 1).ToArray() : new string[] { };
                return LegacyTestTargets.tailcall(newArgs);
            }

            return ProfilerTestRunner.Run(profileePath: System.Reflection.Assembly.GetExecutingAssembly().Location,
                                          testName: "ShadowStackFastWithDSSELT3",
                                          profilerClsid: LegacyProfilerGuid,
                                          satelliteModule: "jit");
        }
    }
}


