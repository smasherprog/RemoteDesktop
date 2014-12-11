using Microsoft.AspNet.SignalR;
using RemoteDesktop_CSLibrary;
using RemoteDesktop_ProxyServer.Code;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;

[assembly: WebActivatorEx.PostApplicationStartMethod(typeof(RemoteDesktop_ProxyServer.Signalr.ProxyWatcher), "Start")]

namespace RemoteDesktop_ProxyServer.Signalr
{
    //THIS CLASS IS NOT THREAD SAFE YET.. JUST A PROTOTYPE.. MMM KAY!!!
    public class ProxyWatcher
    {
        private readonly static Lazy<ProxyWatcher> _instance = new Lazy<ProxyWatcher>(() => new ProxyWatcher(GlobalHost.ConnectionManager.GetHubContext<ProxyHub>()));
        public IHubContext Context;
        public static List<Client_Pair> Clients = new List<Client_Pair>();
        public static object _ClientsLock = new object();
        private ProxyWatcher(IHubContext context)
        {
            Context = context;
        }
        public static void Start()
        {//set callbacks
            ProxyServer.OnClientConnectEvent += ProxyServer_OnClientConnectEvent;
            ProxyServer.OnClientsPairedEvent += ProxyServer_OnClientsPairedEvent;
            ProxyServer.OnClientDisconnectEvent += ProxyServer_OnClientDisconnectEvent;
        }
        public static ProxyWatcher Instance
        {
            get
            {
                return _instance.Value;
            }
        }
        static void ProxyServer_OnClientDisconnectEvent(StateObject c)
        {
            if(c == null)
                return;
            lock(_ClientsLock)
            {
                var left = Clients.FirstOrDefault(a => a.Pair.Any(b => b.ID == c.ID));
                left.Pair.RemoveAll(a => a.IP == c.Sock.RemoteEndPoint.ToString());
                Clients.RemoveAll(a => a.Pair == null || !a.Pair.Any());
            }

            _instance.Value.Context.Clients.All.AvailableClients(Clients);
        }

        static void ProxyServer_OnClientsPairedEvent(Pairing p)
        {
            if(p.Left == null || p.Right == null)
                return;// CRazy!!
            lock(_ClientsLock)
            {
                foreach(var item in Clients)
                {
                    item.Pair.RemoveAll(a => a.IP == p.Left.Sock.RemoteEndPoint.ToString());
                    item.Pair.RemoveAll(a => a.IP == p.Right.Sock.RemoteEndPoint.ToString());
                }
                Clients.RemoveAll(a => a.Pair == null || !a.Pair.Any());

                Clients.Add(new Client_Pair
                {
                    Pair = new List<Client>() { 
                    new Client { ID = p.Left.ID, IP = p.Left.Sock.RemoteEndPoint.ToString(), ConnectTime = p.Left.ConnectTime, Status = "Paired" },
                    new Client { ID = p.Right.ID, IP = p.Right.Sock.RemoteEndPoint.ToString(), ConnectTime = p.Right.ConnectTime, Status = "Paired" }
                }
                });
            }
            _instance.Value.Context.Clients.All.AvailableClients(Clients);
        }

        static void ProxyServer_OnClientConnectEvent(StateObject c)
        {
            lock(_ClientsLock)
            {
                Clients.Add(new Client_Pair { Pair = new List<Client>() { new Client { ID = c.ID, ConnectTime = c.ConnectTime, IP = c.Sock.RemoteEndPoint.ToString(), Status = "Waiting for Pairing . . " } } });
            }
            _instance.Value.Context.Clients.All.AvailableClients(Clients);
        }

    }
}