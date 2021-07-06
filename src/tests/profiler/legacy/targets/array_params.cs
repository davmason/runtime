// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.array_params
{
    using System;

    public struct Point
    {
        public int x;
        public int y;
    }

    struct Tester
    {
        int j;

        public void SetJ()
        {
            j = 69;
        }

        public int GetJ()
        {
            return j;
        }

    }


    public class Test
    {
        int i;

#pragma warning disable 0649  // Field 'Test.t' is never assigned to, and will always have its default value.
        static Tester t;
#pragma warning restore 0649

        public static void ArrayParameterTestFunc(int[,] array1,
                                                        Test[] array2,
                                                        Point[] array3,
                                                        Test[,] array4)
        {
            Console.WriteLine("");
        }

        public static void array_params()
        {
            t.SetJ();

            // Allocate some arrays.
            int[,] my_ints = new int[3, 4];

            int k = 0;
            for (int i = 0; i < 3; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    my_ints[i, j] = k++;
                }
            }

            Test[] my_array = new Test[10];
            my_array[0] = new Test();
            my_array[0].i = my_array[0].i = 13;

            Point[] my_point = new Point[3];

            Test[,] my_array2 = new Test[5, 6];

            ArrayParameterTestFunc(my_ints, my_array, my_point, my_array2);
        }
    }

}

