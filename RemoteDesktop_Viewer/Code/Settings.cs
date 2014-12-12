using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RemoteDesktop_Viewer.Code
{
    public static class Settings
    {
        public static readonly string ProxyServer = "localhost:1466";
        public static readonly string AuthenticationPath = "/Home/Authenticate";
        public static readonly string SignalRHubName = "ProxyHub";
        public static readonly string URIScheme = "http://";
#if WIN64
         public const string DLL_Name = "RemoteDesktopViewer_Library_64.dll";
#else
        public const string DLL_Name = "RemoteDesktopViewer_Library_32.dll";
#endif



    }
}
