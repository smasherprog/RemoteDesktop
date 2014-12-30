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

        [StructLayout(LayoutKind.Sequential)]
        public struct Traffic_Stats
        {
            public long CompressedSendBytes;
            public long CompressedRecvBytes;//overall lifetime totals 
            public long UncompressedSendBytes;

            public long UncompressedRecvBytes;//overall lifetime totals 

            public long CompressedSendBPS;
            public long CompressedRecvBPS;
            public long UncompressedSendBPS;
            public long UncompressedRecvBPS;
        }
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct Settings_Header
        {
          
            public int Image_Quality;
            public bool GrayScale;
            public bool ShareClip;
        }
    }
}
