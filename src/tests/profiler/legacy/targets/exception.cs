// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.exception
{
    using System;


    //
    // MyException
    //
    class MyException
    {
        public MyException(int entries)
        {
            _entries = entries;
            _Initialize();

        } // ctor


        public static void exception(String[] args)
        {
            int x = 0;
            MyException myexception = new MyException(16);


            try
            {
                try
                {
                    myexception._Generate_Exception0();
                }
                finally
                {
                    x++;
                }
            }
            catch (Exception exception0)
            {
                try
                {
                    throw new Exception();
                }
                catch (Exception exception1)
                {
                    x += myexception._Generate_Exception1();
                }
            }
            finally
            {
                x++;  // x should be 6 after this point                  
            }

            Console.WriteLine(x);

        } // main


        public void _Generate_Exception0()
        {
            _iArray[_entries] = 100;

        } // _Generate_Exception0


        public int _Generate_Exception1()
        {
            int i = 0;


            // generate and handle exception
            try
            {
                int j = i / i;
            }
            catch (Exception e0)
            {
                try
                {
                    throw new Exception();
                }
                catch (Exception e1)
                {
                    i = 3;
                }
            }
            finally
            {
                i++;
            }


            return i;

        } // _Generate_Exception1


        private void _Initialize()
        {
            _iArray = new int[_entries];

        } // _Initialize


        // data members
        private int _entries;
        private int[] _iArray;

    } // MyException

} // ExceptionSpace

