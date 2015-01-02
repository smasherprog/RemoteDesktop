using Microsoft.AspNet.SignalR;
using RemoteDesktop_CSLibrary;
using RemoteDesktop_GatewayServer.Code;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Web;

//start and stop to ensure proper destruction and creation
[assembly: WebActivatorEx.PostApplicationStartMethod(typeof(RemoteDesktop_GatewayServer.Signalr.ProxyWatcher), "Start")]
[assembly: WebActivatorEx.ApplicationShutdownMethod(typeof(RemoteDesktop_GatewayServer.Signalr.ProxyWatcher), "Stop")]

namespace RemoteDesktop_GatewayServer.Signalr
{
    public class ProxyWatcher
    {
        private readonly static Lazy<ProxyWatcher> _instance = new Lazy<ProxyWatcher>(() => new ProxyWatcher(GlobalHost.ConnectionManager.GetHubContext<ProxyHub>()));
        public IHubContext Context;
        public static GatewayServer _GatewayServer;
        private ProxyWatcher(IHubContext context)
        {
            Context = context;

            _GatewayServer.ClientManager.OnClientConnectEvent += ClientManager_OnClientConnectEvent;
            _GatewayServer.ClientManager.OnClientDisconnectEvent += ClientManager_OnClientDisconnectEvent;
            _GatewayServer.ClientManager.OnPairedEvent += ClientManager_OnPairedEvent;
        }

        void ClientManager_OnPairedEvent(List<Client> c)//always count == 2
        {
            _instance.Value.Context.Clients.All.AvailableClients(_GatewayServer.ClientManager.Clients);
        }

        void ClientManager_OnClientDisconnectEvent(List<Client> c)//always count >=1
        {
            _instance.Value.Context.Clients.All.AvailableClients(_GatewayServer.ClientManager.Clients);
        }

        void ClientManager_OnClientConnectEvent(List<Client> c)//always count >=1
        {
            _instance.Value.Context.Clients.All.AvailableClients(_GatewayServer.ClientManager.Clients);
        }
        public static void Start()
        {
            if(_GatewayServer == null)
            {//webactivatorex sometimes calls start twice.. I think it might call 
                Debug.WriteLine("Calling Start on Gateway");
                _GatewayServer = new GatewayServer();
            }
        
        }
        public static void Stop()
        {
            _GatewayServer.Dispose();
            _GatewayServer = null;
        }
        public static ProxyWatcher Instance
        {
            get
            {
                return _instance.Value;
            }
        }


        public static Client ReserveID(string ip, string computername, string username, string mac, string sessionId)
        {
            var ret = _GatewayServer.ClientManager.ReserveID(ip, computername, username, mac, sessionId);
            if(ret != null)
                _instance.Value.Context.Clients.All.AvailableClients(_GatewayServer.ClientManager.Clients);
            return ret;
        }
    }
}