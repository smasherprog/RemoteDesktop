using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RemoteDesktop_CSLibrary
{
    public static class App_Settings
    {
#if DEBUG
        public static readonly int Proxy_Listen_Port = 443;
#else 
        public static readonly int Proxy_Listen_Port = 5939;
#endif

    }
}
