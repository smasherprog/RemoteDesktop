using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace RemoteDesktop_CSLibrary
{
    public class Handle_Wrapper:IDisposable
    {

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool FreeLibrary(IntPtr hModule);

        public IntPtr Handle { get; set; }
        public Handle_Wrapper(IntPtr handle)
        {
            Handle = handle;
        }
        public void Dispose()
        {
            if (Handle != IntPtr.Zero) FreeLibrary(Handle);
            Handle = IntPtr.Zero;
        }
    }
}
