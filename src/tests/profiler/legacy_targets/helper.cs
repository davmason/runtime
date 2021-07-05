// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System;

namespace Profiler.Tests
{
    public static class LegacyTestTargets
    {
        public static int divzero(string[] args)
        {
            LegacyTestTargetsImpl.divzero.C.divzero(args);
            return 100;
        }

        public static int tieredcompile(string[] args)
        {
            LegacyTestTargetsImpl.tieredcompile.Program.tieredcompile(args);
            return 100;
        }

        public static int floatingpointregisters(string[] args)
        {
            LegacyTestTargetsImpl.floatingpointregisters.My.floatingpointregisters();
            return 100;
        }

        public static int mytest(string[] args)
        {
            LegacyTestTargetsImpl.mytest.My.mytest();
            return 100;
        }

        public static int finalizer(string[] args)
        {
            LegacyTestTargetsImpl.finalizer.Finalizer.finalizer();
            return 100;
        }

        public static int car(string[] args)
        {
            LegacyTestTargetsImpl.car.Program.car(args);
            return 100;
        }

        public static int factorial(string[] args)
        {
            return LegacyTestTargetsImpl.factorial.Factorial.factorial(args);
        }

        public static int array_params(string[] args)
        {
            LegacyTestTargetsImpl.array_params.Test.array_params();
            return 100;
        }

        public static int gen_sharedcode(string[] args)
        {
            LegacyTestTargetsImpl.gen_sharedcode.Test.gen_sharedcode();
            return 100;
        }

        public static int genclasses(string[] args)
        {
            LegacyTestTargetsImpl.genclasses.Test.genclasses();
            return 100;
        }

        public static int gen_factorial(string[] args)
        {
            return LegacyTestTargetsImpl.gen_factorial.Test.gen_factorial(args);
        }

        public static int fibonacci(string[] args)
        {
            LegacyTestTargetsImpl.fibonacci.test.fibonacci(args);
            return 100;
        }

        public static int hello(string[] args)
        {
            return LegacyTestTargetsImpl.hello.HelloWorld.hello(args);
        }

        public static int profiler_contracts(string[] args)
        {
            LegacyTestTargetsImpl.profiler_contracts.My.profiler_contracts();
            return 100;
        }

        public static int collectible(string[] args)
        {
            LegacyTestTargetsImpl.collectible.Test.collectible();
            return 100;
        }

        public static int jmc1(string[] args)
        {
            LegacyTestTargetsImpl.jmc1.Foo.jmc1();
            return 100;
        }

        public static int tailcall(string[] args)
        {
            LegacyTestTargetsImpl.tailcall.MainEntry.tailcall(args);
            return 100;
        }

        public static int statics_domainneutral(string[] args)
        {
            LegacyTestTargetsImpl.statics_domainneutral.MyStaticClass.statics_domainneutral();
            return 100;
        }

        public static int dynamicmodules(string[] args)
        {
            LegacyTestTargetsImpl.dynamicmodules.DynamicModule.dynamicmodules(args);
            return 100;
        }

        public static int dynil(string[] args)
        {
            LegacyTestTargetsImpl.dynil.DynIL.dynil(args);
            return 100;
        }

        public static int statics_argretval(string[] args)
        {
            LegacyTestTargetsImpl.statics_argretval.Test.statics_argretval(args);
            return 100;
        }

        public static int frozenstrings(string[] args)
        {
            LegacyTestTargetsImpl.frozenstrings.FrozenStrings.frozenstrings();
            return 100;
        }

        public static int gen_sharedstatics(string[] args)
        {
            LegacyTestTargetsImpl.gen_sharedstatics.SharedTest.gen_sharedstatics();
            return 100;
        }

        public static int mdinjecttarget(string[] args)
        {
            return LegacyTestTargetsImpl.mdinjecttarget.MDInjectTarget.mdinjecttarget(args);
        }

        public static int gc(string[] args)
        {
            LegacyTestTargetsImpl.gc.GC.gc(args);
            return 100;
        }

        public static int _2gbtest(string[] args)
        {
            return LegacyTestTargetsImpl._2gbtest.My._2gbtest();
        }

        public static int largeobjectalloc(string[] args)
        {
            return LegacyTestTargetsImpl.largeobjectalloc.Mainy.largeobjectalloc(args);
        }

        public static int collect(string[] args)
        {
            LegacyTestTargetsImpl.collect.collect.runCollect();
            return 100;
        }

        public static int exception(string[] args)
        {
            LegacyTestTargetsImpl.exception.MyException.exception(args);
            return 100;
        }

        public static int hiroshima(string[] args)
        {
            LegacyTestTargetsImpl.hiroshima.AahExceptions.hiroshima(args);
            return 100;
        }

        public static int thread_name(string[] args)
        {
            LegacyTestTargetsImpl.thread_name.ThreadExample.thread_name();
            return 100;
        }

        public static int attachprofilerunit(string[] args)
        {
            LegacyTestTargetsImpl.attachprofilerunit.AttachUnitTests.attachprofilerunit(args);
            return 100;
        }

    }
}

