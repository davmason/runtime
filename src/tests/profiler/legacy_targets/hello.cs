// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.hello
{
    using System;
    using System.Diagnostics;


    class Foo
    {
        static Foo()
        {
        }
    }

    abstract class HelloBase
    {
        public
        abstract String GetWho();

        protected String who_;

    }

    class HelloWorld : HelloBase
    {
        protected HelloWorld(String arg)
        {
            who_ = arg;
        }

        public override string GetWho()
        {
            return who_;
        }

        public static int hello(String[] args)
        {
            HelloBase h;
            String message = "";


            // Force a static class initializor to be run.
            new Foo();

            // Console.WriteLine("Number of command line parameters = {0}", args.Length );

            if (args.Length == 1)
                h = new HelloWorld(args[0]);

            else
            {
                for (int i = 0; i < args.Length; i++)
                    message = message + args[i] + " ";


                h = new HelloWorld(message);
            }

            message = "Hello " + h.GetWho();

            Console.WriteLine("Just a Test");
            Console.WriteLine("Another string");

            // Console.WriteLine(message);
            return 0;
        }
    }
}

