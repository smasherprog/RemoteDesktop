using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RemoteDesktop_CSLibrary
{
    public class Client
    {
        public int ID { get; set; }
        public string IP { get; set; }
        public DateTime ConnectTime { get; set; }
        public string Status { get; set; }
    }
    public class Client_Pair
    {

        public List<Client> Pair = new List<Client>();
    }
}
