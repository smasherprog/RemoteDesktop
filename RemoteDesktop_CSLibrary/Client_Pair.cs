using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RemoteDesktop_CSLibrary
{

    public class Client
    {
        public enum Host_Type {Viewer, Server};
        public int ID { get; set; }
        public string Firewall_IP { get; set; }
        public string Internal_IP { get; set; }
        public string Mac_Address { get; set; }
        public string ComputerName { get; set; }
        public string UserName { get; set; }
        public Host_Type Host { get; set; }
        public DateTime ConnectTime { get; set; }
        public string Status { get; set; }
    }

}
