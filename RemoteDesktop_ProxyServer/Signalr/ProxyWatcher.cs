using Microsoft.AspNet.SignalR;
using RemoteDesktop_CSLibrary;
using RemoteDesktop_ProxyServer.Code;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Security.Cryptography;
using System.Web;

[assembly: WebActivatorEx.PostApplicationStartMethod(typeof(RemoteDesktop_ProxyServer.Signalr.ProxyWatcher), "Start")]
[assembly: WebActivatorEx.ApplicationShutdownMethod(typeof(RemoteDesktop_ProxyServer.Signalr.ProxyWatcher), "Stop")]

namespace RemoteDesktop_ProxyServer.Signalr
{

    //THIS CLASS IS NOT THREAD SAFE YET.. JUST A PROTOTYPE.. MMM KAY!!!
    public class ProxyWatcher
    {
        private readonly static Lazy<ProxyWatcher> _instance = new Lazy<ProxyWatcher>(() => new ProxyWatcher(GlobalHost.ConnectionManager.GetHubContext<ProxyHub>()));
        public IHubContext Context;
        public static Client[] Clients = new Client[ProxyServer.MAXCLIENTS];
        private static object _ClientsLock = new object();

        private static System.Collections.Concurrent.ConcurrentQueue<int> Ids = new System.Collections.Concurrent.ConcurrentQueue<int>();
        private static System.Timers.Timer _TimeoutTimer;

        private ProxyWatcher(IHubContext context)
        {
            Context = context;
        }
        public static void Start()
        {//set callbacks
            for (var i = 0; i < ProxyServer.MAXCLIENTS; i++)
            {
                Ids.Enqueue(i);
                Clients[i] = null;
            }


            _TimeoutTimer = new System.Timers.Timer();
            _TimeoutTimer.Interval = 5000;//every 5 seconds
            _TimeoutTimer.Elapsed += _TimeoutTimer_Elapsed;
            _TimeoutTimer.Start();
            ProxyServer.OnClientConnectEvent += ProxyServer_OnClientConnectEvent;
            ProxyServer.OnClientDisconnectEvent += ProxyServer_OnClientDisconnectEvent;
        }
        public static void Stop()
        {//set callbacks

            if (_TimeoutTimer != null)
            {
                _TimeoutTimer.Stop();
                _TimeoutTimer.Dispose();
                _TimeoutTimer = null;
            }
        }
        static void _TimeoutTimer_Elapsed(object sender, System.Timers.ElapsedEventArgs e)
        {

            for (var i = 0; i < Clients.Length; i++)
            {
                if (Clients[i] != null)
                {
                    var dt = (DateTime.Now - Clients[i].ConnectTime).TotalSeconds;//disconnect viewers after 10 seconds of no data, and servers after 5 minutes to allow id reuse
                    if ((dt > 10 && Clients[i].Host == Client.Host_Type.Viewer) || (dt > 60 * 5 && Clients[i].Host == Client.Host_Type.Server))
                    {
                        Clients[i] = null;
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
        private static string ByteArrayToString(byte[] ba)
        {
            var hex = new System.Text.StringBuilder(ba.Length * 2);
            foreach (byte b in ba)
                hex.AppendFormat("{0:x2}", b);
            return hex.ToString();
        }
        public static Client ReserveID(string ip, string computername, string username, string mac, int sessionId)
        {
            var id = -1;
            //check for a previous connection
            var prev = Clients.FirstOrDefault(a => a != null && a.Mac_Address == mac && a.SessionID == sessionId && a.Host == Client.Host_Type.Server);
            if (prev != null)
            {
                if(prev.Status != Client.Connection_Status.Pending)
                    return null;
                prev.UserName = username;
                prev.ComputerName = computername;
                prev.ConnectTime = DateTime.Now;
                prev.Status = Client.Connection_Status.Pending;
                return prev;
            }
            if (Ids.TryDequeue(out id))
            {
                var c = new Client();
                c.ComputerName = computername;
                c.UserName = username;
                c.Mac_Address = mac;
                c.SessionID = sessionId;
                c.Host = Client.Host_Type.Server;
                c.ConnectTime = DateTime.Now;
                c.Src_ID = id;
                c.Firewall_IP = ip;
                c.Status = Client.Connection_Status.Pending;
                using (var aes = new AesManaged())
                {
                    aes.KeySize = 256;
                    aes.GenerateKey();
                    c.AES_Session_Key = ByteArrayToString(aes.Key);
                }
                Clients[id] = c;
                return c;
            }
            return null;
        }
        static void ProxyServer_OnClientDisconnectEvent(StateObject c)
        {
            if (c == null)
                return;

            if (c.Src_ID != -1)
            {
                var client = Clients[c.Src_ID];
                if (client != null)
                {
                    if (client.Host == Client.Host_Type.Viewer)
                    {
                        Clients[c.Src_ID] = null;
                        Ids.Enqueue(c.Src_ID);
                    }
                    else
                    {
                        client.ConnectTime = DateTime.Now;
                        client.Status = Client.Connection_Status.Pending;
                    }
                }

            }


            _instance.Value.Context.Clients.All.AvailableClients(Clients);
        }


        static bool ProxyServer_OnClientConnectEvent(StateObject c)
        {
            var ret = false;
            if (c.Src_ID > -1)
            {//must be a server connecting
                if (c.Dst_ID != -1 || c.Src_ID > ProxyServer.MAXCLIENTS) ret = false;
                var pos = Clients[c.Src_ID];
                if (pos == null)
                {
                    pos.Dst_ID = c.Src_ID = -1;
                    ret = false;
                }
                else if (pos.Status == Client.Connection_Status.Pending && pos.Firewall_IP == c.Sock.RemoteEndPoint.ToString().Split(':')[0])
                {
                    pos.Status = Client.Connection_Status.Connected;
                    pos.Host = Client.Host_Type.Server;
                    ret = true;
                }
                else
                {
                    pos.Dst_ID = c.Src_ID = -1;
                    ret = false;
                }
               pos.Dst_ID= c.Dst_ID = -1;//always -1 just in case
            }
            else
            {//must be a viewer connecting... check for valid mapping
                if (c.Src_ID != -1 || c.Dst_ID < 0 || c.Dst_ID > ProxyServer.MAXCLIENTS) ret = false;
                else
                {
                    if(Clients.Any(a => a != null && a.Dst_ID == c.Dst_ID))
                        return false;// do not allow more than 1 pairing
                    var posclinet = new Client();
                    int id = -1;
                    if (Ids.TryDequeue(out id))
                    {//get a new if for the client
                        c.Src_ID = posclinet.Src_ID = id;
                        posclinet.Status = Client.Connection_Status.Pending;
                        posclinet.ConnectTime = DateTime.Now;
                        posclinet.Host = Client.Host_Type.Viewer;
                        posclinet.Dst_ID = c.Dst_ID;
                        posclinet.Firewall_IP = c.Sock.RemoteEndPoint.ToString().Split(':')[0];
                        posclinet.UserName = "Dummy";
                        posclinet.ComputerName = "Dummy";
                        Clients[id] = posclinet;//add to list
                        ret = true;
                    }
                    else ret = false;
                }
            }
            if (!ret) return ret;
            _instance.Value.Context.Clients.All.AvailableClients(Clients);
            return ret;
        }

    }
}