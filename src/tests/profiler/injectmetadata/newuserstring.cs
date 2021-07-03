// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System;
using System.Linq;
using System.Collections.Generic;

namespace Profiler.Tests
{
    class newuserstring
    {
        static readonly Guid NewUserStringGuid = new Guid("TODO: add guid");
        public static int Main(string[] args)
        {
            if (args.Length > 0 && args[0].Equals("RunTest", StringComparison.OrdinalIgnoreCase))
            {
                string[] newArgs = args.Length > 1 ? Enumerable.TakeLast(args, args.Length - 1).ToArray() : new string[] { };
                return LegacyTestTargets.mdinjecttarget(newArgs);
            }

            return ProfilerTestRunner.Run(profileePath: System.Reflection.Assembly.GetExecutingAssembly().Location,
                                          testName: "NewUserString",
                                          profilerClsid: NewUserStringGuid);
        }
    }
}


