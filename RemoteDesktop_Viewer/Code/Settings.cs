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

    }
}
