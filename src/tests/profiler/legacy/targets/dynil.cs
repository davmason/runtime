// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.dynil
{
    using System;
    using System.Threading;
    using System.Reflection;
    using System.Reflection.Emit;


    public class DynIL
    {
        private static ModuleBuilder _GetModuleBuilder()
        {
            AssemblyName assemblyName;


            assemblyName = new AssemblyName();
            if (assemblyName != null)
            {
                AssemblyBuilder assemblyBuilder;


                assemblyName.Name = "dynil.mod";
                assemblyBuilder = AssemblyBuilder.DefineDynamicAssembly(assemblyName,
                                                                   AssemblyBuilderAccess.Run);
                if (assemblyBuilder != null)
                    return assemblyBuilder.DefineDynamicModule("dynil1.mod");
            }


            return null;

        } // _GetModuleBuilder


        private static void _Phase2(Type t, String clazz, String method)
        {
            if (t != null)
            {
                Object obj;


                obj = Activator.CreateInstance(t);
                if (obj != null)
                {
                    MethodInfo methodInfo;


                    methodInfo = t.GetMethod(method);
                    if (methodInfo != null)
                    {
                        Object ret;
                        Object[] args = new Object[1];


                        args[0] = 3;
                        ret = methodInfo.Invoke(obj, args);
                        Console.WriteLine(ret);

                        args[0] = 6;
                        ret = methodInfo.Invoke(obj, args);
                        Console.WriteLine(ret);

                        args[0] = 81;
                        ret = methodInfo.Invoke(obj, args);
                        Console.WriteLine(ret);
                    }
                }
            }

        } // _Phase2


        public static void dynil(String[] args)
        {
            ModuleBuilder moduleBuilder;


            moduleBuilder = _GetModuleBuilder();
            if (moduleBuilder != null)
            {
                TypeBuilder typeBuilder;


                typeBuilder = moduleBuilder.DefineType("Clazz");
                if (typeBuilder != null)
                {
                    MethodBuilder methodBuilder;
                    Type[] parameters = { Type.GetType("System.Int32") };


                    methodBuilder = typeBuilder.DefineMethod("F",
                                                              MethodAttributes.Public,
                                                              Type.GetType("System.Int32"),
                                                              parameters);
                    if (methodBuilder != null)
                    {
                        ILGenerator ilGenerator;


                        ilGenerator = methodBuilder.GetILGenerator();
                        if (ilGenerator != null)
                        {
                            ilGenerator.Emit(OpCodes.Ldarg, (short)1);
                            ilGenerator.Emit(OpCodes.Ldarg, (short)1);
                            ilGenerator.Emit(OpCodes.Mul);
                            ilGenerator.Emit(OpCodes.Ret);

                            Type t = typeBuilder.CreateTypeInfo().UnderlyingSystemType;
                            _Phase2(t, "Clazz", "F");
                        }
                    }
                }
            }

        } // main

    } // DynIL
}

