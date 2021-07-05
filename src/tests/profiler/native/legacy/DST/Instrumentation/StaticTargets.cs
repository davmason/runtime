using System;
using System.Collections.Generic;
using System.Text;
using System.Security;
using System.Runtime.InteropServices;

namespace Instrumentation.Managed
{      
    public class StaticTargets
    {
        [DllImport("TestProfiler.dll")]
        public static extern void InstrumentationMethodEnter(IntPtr moduleID, IntPtr functionID);

        public static void MethodEnter(IntPtr methodID, IntPtr functionID)
        {
            InstrumentationMethodEnter(methodID, functionID);
        }
    }
}
