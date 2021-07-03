// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.collectible
{
    using System;
    using System.Collections.Generic;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Runtime.InteropServices;
    using System.Diagnostics;

    public interface IHelloWorld
    {
        void Hello();
    }

    class Test
    {
        public static IHelloWorld GetCollectibleTypeInstance(int i)
        {
            AssemblyBuilder ab = AssemblyBuilder.DefineDynamicAssembly(
                new AssemblyName("CollectibleAssembly" + i),
                AssemblyBuilderAccess.RunAndCollect);

            ModuleBuilder mb = ab.DefineDynamicModule("CollectibleModule" + i);

            TypeBuilder tb = mb.DefineType("CollectibleHelloType" + i, TypeAttributes.Public);
            tb.AddInterfaceImplementation(typeof(IHelloWorld));
            tb.DefineDefaultConstructor(MethodAttributes.Public);

            MethodBuilder methb = tb.DefineMethod("Hello", MethodAttributes.Public | MethodAttributes.Virtual);
            methb.GetILGenerator().Emit(OpCodes.Ldstr, "Hello World " + i);
            methb.GetILGenerator().EmitCall(OpCodes.Call, typeof(Console).GetMethod("WriteLine", new Type[] { typeof(string) }), null);
            methb.GetILGenerator().Emit(OpCodes.Ret);

            Type HelloType = tb.CreateTypeInfo().UnderlyingSystemType;
            return (IHelloWorld)Activator.CreateInstance(HelloType);
        }

        public static void collectible()
        {
            for (int i = 0; i < 100; i++)
            {
                IHelloWorld obj = GetCollectibleTypeInstance(i);
                obj.Hello();
            }
        }
    }
}

