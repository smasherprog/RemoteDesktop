using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace RemoteDesktop_Viewer.Code
{
    public static class CPP_Settings
    {
#if WIN64
         public const string DLL_Name = "RemoteDesktopViewer_Library_64.dll";
#else
        public const string DLL_Name = "RemoteDesktopViewer_Library_32.dll";
#endif

        [DllImport(DLL_Name, CallingConvention = CallingConvention.StdCall)]
        public static extern IntPtr ProxyServer();
        [DllImport(DLL_Name, CallingConvention = CallingConvention.StdCall)]
        public static extern IntPtr AuthenticationPath();
        [DllImport(DLL_Name, CallingConvention = CallingConvention.StdCall)]
        public static extern IntPtr SignalRHubName();
        [DllImport(DLL_Name, CallingConvention = CallingConvention.StdCall)]
        public static extern IntPtr URIScheme();
    }
    public static class Settings
    {
#if WIN64
         public const string DLL_Name = "RemoteDesktopViewer_Library_64.dll";
#else
        public const string DLL_Name = "RemoteDesktopViewer_Library_32.dll";
#endif
        public static string ProxyServer { get { return Marshal.PtrToStringUni(CPP_Settings.ProxyServer()); } }
        public static string AuthenticationPath { get { return Marshal.PtrToStringUni(CPP_Settings.ProxyServer()); } }
        public static string SignalRHubName { get { return Marshal.PtrToStringUni(CPP_Settings.SignalRHubName()); } }
        public static string URIScheme { get { return  Marshal.PtrToStringUni(CPP_Settings.URIScheme()); } }

    }
}
