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
        public override Task OnConnected()
        {
          return Clients.Caller.AvailableClients(RemoteDesktop_ProxyServer.Code.ProxyServer.Clients);
        }

    }
}