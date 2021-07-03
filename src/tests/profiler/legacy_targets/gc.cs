// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.

namespace LegacyTestTargetsImpl.gc
{
    using System;

    public class GC
    {

        public static void gc(String[] Args)
        {
            int iRep = 5;
            int iObj = 50;

            System.GC.Collect();
            System.GC.WaitForPendingFinalizers();

            GC Mv_Base = new GC();
            Mv_Base.runTest(iRep, iObj);

            System.GC.Collect();
            System.GC.WaitForPendingFinalizers();

            // Sleep 1/2 sec, so that our dbg_u028.exe doesn't fail. [m_pCurrentProcess->IsRunning(&m_bIsRunning)]
            System.Threading.Thread.Sleep(500);
        }


        public GC()
        {
        }


        public bool runTest(int Rep, int Obj)
        {
            int TotalMem = 0;


            for (int A = 0; A < Rep; A++)
            {
                CvA_RandomNode = new RandomNode[Obj];

                for (int J = 0; J < Obj; J++)
                {
                    CvA_RandomNode[J] = new RandomNode(J, J);
                    if (J == 0)
                    {
                        RandomNode.setBigSize(0);
                        RandomNode.setUseFinal(false);
                    }
                }

                TotalMem += RandomNode.getBigSize();

                CvA_RandomNode = null;
            }


            return true;
        }

        RandomNode[] CvA_RandomNode;

    };
}


