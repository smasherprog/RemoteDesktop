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
        public enum Connection_Status { Reserved, Pending, Connected, Paired };
        public int Src_ID { get; set; }
        public int Dst_ID { get; set; }
        public string Firewall_IP { get; set; }
        public string Internal_IP { get; set; }
        public string Mac_Address { get; set; }
        public string ComputerName { get; set; }
        public string UserName { get; set; }
        public string SessionID { get; set; }
        public Host_Type Host { get; set; }
        public DateTime ConnectTime { get; set; }
        public Connection_Status Status { get; set; }
        public string AES_Session_Key { get; set; } //TO BE MOVED LATER
    }

}
