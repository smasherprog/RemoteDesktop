using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace RemoteDesktop_Viewer.Code
{

    public static class Settings
    {
#if WIN64
         public const string DLL_Name = "RemoteDesktopViewer_Library_64.dll";
#else
        public const string DLL_Name = "RemoteDesktopViewer_Library_32.dll";
#endif


        public static string Gateway { get { return "localhost"; } }
        public static string AuthenticationUrl { get { return "http://localhost:3406/RAT_Support/Authenticate"; } }
        public static string SignalRHubName { get { return "ProxyHub"; } }
        public static string SignalRHubUrl { get { return "http://localhost:3406/"; } }


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
