using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;

namespace RemoteDesktop_ProxyServer.Model
{
    public class Client
    {
        public string Name { get; set; }
        public string IP { get; set; }
        public DateTime ConnectTime { get; set; }
        public string Status { get; set; }
    }
}