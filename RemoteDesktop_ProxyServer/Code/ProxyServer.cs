using RemoteDesktop_ProxyServer.Model;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;

namespace RemoteDesktop_ProxyServer.Code
{
    static public class ProxyServer
    {
        private static List<Client> _Clients = new List<Client>();

        public static List<Client> Clients { get { return _Clients; } }
        private static System.Threading.Thread _Thread = null;
        private static bool _Running = false;
        public static void Start(int port)
        {
            _Clients = new List<Client>() { 
                new Client { IP = "10.23.1.23", Name = "Test Computer", Status = "Connected to Cont\\Joe", ConnectTime = DateTime.Now },
                new Client { IP = "202.111.23.12", Name = "Other Computer", Status = "Waiting For Pairing", ConnectTime = DateTime.Now.AddMinutes(-10) } 
            };
            _Running = true;
            _Thread = new System.Threading.Thread(new System.Threading.ThreadStart(_Run));

        }
        private static void _Run(){
            while(_Running)
            {
                //do stuff here
            }
        }
        public static void Stop()
        {
            _Running = false;
            if(_Thread != null)  _Thread.Join(3000);
            _Thread = null;
        }

    }
}