using Microsoft.AspNet.SignalR;
using RemoteDesktop_CSLibrary;
using RemoteDesktop_ProxyServer.Code;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;

[assembly: WebActivatorEx.PostApplicationStartMethod(typeof(RemoteDesktop_ProxyServer.Signalr.ProxyWatcher), "Start")]
[assembly: WebActivatorEx.ApplicationShutdownMethod(typeof(RemoteDesktop_ProxyServer.Signalr.ProxyWatcher), "Stop")]

namespace RemoteDesktop_ProxyServer.Signalr
{
    public struct ConnectionMap
    {
        public int Viewer = -1;
        public int Server = -1;
    }
    //THIS CLASS IS NOT THREAD SAFE YET.. JUST A PROTOTYPE.. MMM KAY!!!
    public class ProxyWatcher
    {
        private readonly static Lazy<ProxyWatcher> _instance = new Lazy<ProxyWatcher>(() => new ProxyWatcher(GlobalHost.ConnectionManager.GetHubContext<ProxyHub>()));
        public IHubContext Context;
        public static List<Client> Clients = new List<Client>();
        public static object _ClientsLock = new object();

        private static Client[] PendingClients = new Client[ProxyServer.MAXCLIENTS];
        private static System.Collections.Concurrent.ConcurrentQueue<int> Ids = new System.Collections.Concurrent.ConcurrentQueue<int>();
        private static System.Timers.Timer _TimeoutTimer;

        private static ConnectionMap[] ConnectionMapping = new ConnectionMap[ProxyServer.MAXCLIENTS];

        private ProxyWatcher(IHubContext context)
        {
            Context = context;
        }
        public static void Start()
        {//set callbacks
            for(var i = 0; i < PendingClients.Length; i++)
                Ids.Enqueue(i);
            for(var i = 0; i < ConnectionMapping.Length; i++)
                ConnectionMapping[i] = new ConnectionMap();
            _TimeoutTimer = new System.Timers.Timer();
            _TimeoutTimer.Interval = 5000;//every 5 seconds
            _TimeoutTimer.Elapsed += _TimeoutTimer_Elapsed;
            _TimeoutTimer.Start();
            ProxyServer.OnClientConnectEvent += ProxyServer_OnClientConnectEvent;
            ProxyServer.OnClientDisconnectEvent += ProxyServer_OnClientDisconnectEvent;
        }
            public static void Stop()
        {//set callbacks
        
            if(_TimeoutTimer!=null){
                _TimeoutTimer.Stop();
                _TimeoutTimer.Dispose();
                _TimeoutTimer=null;
            }
        }
        static void _TimeoutTimer_Elapsed(object sender, System.Timers.ElapsedEventArgs e)
        {
            for(var i=0; i< PendingClients.Length; i++){
                if(PendingClients[i] != null)
                {
                    if((DateTime.Now - PendingClients[i].ConnectTime).TotalSeconds > 10)
                    {
                        PendingClients[i] = null;
                        Ids.Enqueue(i);
                    }
                }
            }
        }
        public static ProxyWatcher Instance
        {
            get
            {
                return _instance.Value;
            }
        }
        public static int ReserveID(string computername, string username, string mac)
        {
            var id = -1;
            if(Ids.TryDequeue(out id))
            {
                var c = new Client();
                c.ComputerName = computername;
                c.UserName = username;
                c.Mac_Address = mac;
                c.Host = Client.Host_Type.Server;
                c.ConnectTime = DateTime.Now;//double duty.. if no connect attempt in 10 seconds.. timeout
                PendingClients[id] = c;
                return id;
            }
            return -1;
        }
        static void ProxyServer_OnClientDisconnectEvent(StateObject c)
        {
            if(c == null)
                return;
            lock(_ClientsLock)
            {
                Clients.RemoveAll(a=>a.Mac_Address)
                var left = Clients.FirstOrDefault(a => a.Pair.Any(b => b.ID == c.ID));
                left.Pair.RemoveAll(a => a.IP == c.Sock.RemoteEndPoint.ToString());
                Clients.RemoveAll(a => a.Pair == null || !a.Pair.Any());
            }

            _instance.Value.Context.Clients.All.AvailableClients(Clients);
        }


        static bool ProxyServer_OnClientConnectEvent(StateObject c)
        {
            var pos= PendingClients[c.ID];
            PendingClients[c.ID] = null;
            if(pos!=null){
                pos.ConnectTime = DateTime.Now;
                pos.
            }
            if(ConnectionMapping[c.ID].)
            {

            }
            lock(_ClientsLock)
            {
                Clients.Add(new Client_Pair { Pair = new List<Client>() { new Client { ID = c.ID, ConnectTime = c.ConnectTime, IP = c.Sock.RemoteEndPoint.ToString(), Status = "Waiting for Pairing . . " } } });
            }
            _instance.Value.Context.Clients.All.AvailableClients(Clients);
        }

    }
}