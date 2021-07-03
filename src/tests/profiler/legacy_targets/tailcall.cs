// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

using System;

namespace LegacyTestTargetsImpl.tailcall
{
    internal class MainEntry
    {
        public static void tailcall(string[] args)
        {
            int local = 1;
            int local2 = 2;
            int local3 = 3;
            FunctionClass fclass = new FunctionClass(local, local2, local3);
            fclass.fun0(local, local2);
        }
    }

    public class FunctionClass
    {
        public int a;

        public int b;

        public int c;

        public int[] localArray;

        public FunctionClass(int a, int b, int c)
        {
            this.a = a;
            this.b = b;
            this.c = c;
            this.localArray = new int[this.a + this.b + this.c];
        }

        public FunctionClass()
        {
            this.a = 0;
            this.b = 0;
            this.c = 0;
        }

        public int fun0(int index, int value)
        {
            int dummy = 3;
            dummy = index + value + dummy;
            dummy = dummy + this.a + this.b + this.c;
            this.fun1(index, value);
            return 0;
        }

        public int fun1(int index, int value)
        {
            int dummy = 3;
            dummy = index + value + dummy;
            dummy = dummy + this.a + this.b + this.c;
            this.fun2(index, value);
            return 1;
        }

        public int fun2(int index, int value)
        {
            int dummy = 3;
            dummy = index + value + dummy;
            dummy = dummy + this.a + this.b + this.c;
            this.fun3(index, value);
            return 2;
        }

        public int fun3(int index, int value)
        {
            int dummy = 3;
            dummy = index + value + dummy;
            dummy = dummy + this.a + this.b + this.c;
            this.fun4(index, value);
            return 3;
        }

        public int fun4(int index, int value)
        {
            int dummy = 3;
            dummy = index + value + dummy;
            dummy = dummy + this.a + this.b + this.c;
            this.fun5(index, value);
            return 4;
        }

        public int fun5(int index, int value)
        {
            int dummy = 3;
            dummy = index + value + dummy;
            dummy = dummy + this.a + this.b + this.c;
            return this.fun6(index, value);
        }

        public int fun6(int index, int value)
        {
            int dummy = 3;
            dummy = index + value + dummy;
            dummy = dummy + this.a + this.b + this.c;
            return this.fun7(index, value);
        }

        public int fun7(int index, int value)
        {
            int dummy = 3;
            dummy = index + value + dummy;
            dummy = dummy + this.a + this.b + this.c;
            return this.fun8(index, value);
        }

        public int fun8(int index, int value)
        {
            int dummy = 3;
            dummy = index + value + dummy;
            dummy = dummy + this.a + this.b + this.c;
            return this.fun9(index, value);
        }

        public int fun9(int index, int value)
        {
            this.localArray[index] = value;
            return 9;
        }
    }
}


