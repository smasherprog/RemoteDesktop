using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using Microsoft.AspNet.SignalR;
using System.Threading.Tasks;
using RemoteDesktop_ProxyServer.Model;

namespace RemoteDesktop_ProxyServer.Signalr
{
    [AuthorizeClaimsAttribute]
    public class ProxyHub : Hub
    {
        //entered  dummy data for now
        static List<Client> _Clients = new List<Client>() { 
            new Client { IP = "10.23.1.23", Name = "Test Computer", Status = "Connected to Cont\\Joe", ConnectTime = DateTime.Now },
            new Client { IP = "202.111.23.12", Name = "Other Computer", Status = "Waiting For Pairing", ConnectTime = DateTime.Now.AddMinutes(-10) } 
        };
        public override Task OnConnected()
        {
          return Clients.Caller.AvailableClients(_Clients);
        }

    }
}