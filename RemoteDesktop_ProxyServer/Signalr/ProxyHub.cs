using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using Microsoft.AspNet.SignalR;
using System.Threading.Tasks;
namespace RemoteDesktop_ProxyServer.Signalr
{
    [AuthorizeClaimsAttribute]
    public class ProxyHub : Hub
    {
        //THIS CLASS IS NOT THREAD SAFE YET.. JUST A PROTOTYPE.. MMM KAY!!!
        ProxyWatcher _ProxyWatcher;
        public ProxyHub() : this(ProxyWatcher.Instance) { }
        public ProxyHub(ProxyWatcher i)
        {
            _ProxyWatcher = i;
        }
        public override Task OnConnected()
        {
            return Clients.Caller.AvailableClients(RemoteDesktop_ProxyServer.Signalr.ProxyWatcher.Clients);
        }

    }
}