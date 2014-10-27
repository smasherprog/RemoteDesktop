using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace RemoteDesktopServer
{
    public static class Server_PInvoke
    {
        [DllImport("RemoteDesktop_Library.dll")]
        public static extern IntPtr CreateServer();
        [DllImport("RemoteDesktop_Library.dll")]
        public static extern void DestroyServer(IntPtr server);
        [DllImport("RemoteDesktop_Library.dll")]
        public static extern void Listen(IntPtr server, ushort port);    
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        public delegate void OnConnectCallback();
        [DllImport("RemoteDesktop_Library.dll")]
        public static extern void SetOnConnectCallback(IntPtr server, [MarshalAs(UnmanagedType.FunctionPtr)]OnConnectCallback callback);
  
    }
}
