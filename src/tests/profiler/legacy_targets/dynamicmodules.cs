// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System;
using System.IO;
using System.Reflection;
using System.Reflection.Emit;


namespace LegacyTestTargetsImpl.dynamicmodules
{
    class DynamicModule
    {
        public static void dynamicmodules(string[] args)
        {
            ExecRefEmit();
        }
        public static void ExecRefEmit()
        {
            AssemblyBuilder asmBldr = AssemblyBuilder.DefineDynamicAssembly(
            new AssemblyName("MyRefEmitAssembly"),
            AssemblyBuilderAccess.Run | AssemblyBuilderAccess.Run);
            ModuleBuilder modBldr = asmBldr.DefineDynamicModule("MyRefEmitModule0");
            //ModuleBuilder modBldr1 = asmBldr.DefineDynamicModule("MyRefEmitModule1", false);
            //ModuleBuilder modBldr2 = asmBldr.DefineDynamicModule("MyRefEmitModule2", true);
            //ModuleBuilder modBldr3 = asmBldr.DefineDynamicModule("MyRefEmitModule3", "A.dll");
            //ModuleBuilder modBldr4 = asmBldr.DefineDynamicModule("MyRefEmitModule4", "B.dll", true);
            //ModuleBuilder modBldr5 = asmBldr.DefineDynamicModule("MyRefEmitModule5", "C.dll", false);
        }
    }
}

