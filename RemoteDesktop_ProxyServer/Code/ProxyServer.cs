
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Web;

[assembly: WebActivatorEx.PostApplicationStartMethod(typeof(RemoteDesktop_ProxyServer.Code.ProxyServer), "Start")]
[assembly: WebActivatorEx.ApplicationShutdownMethod(typeof(RemoteDesktop_ProxyServer.Code.ProxyServer), "Stop")]

namespace RemoteDesktop_ProxyServer.Code
{
    public enum ClientState { Pending, PendingPair, Paired };
    public class StateObject
    {
        public Socket Sock = null;
        public const int BufferSize = 1024 * 1024;
        public int BufferCount = 0;
        public byte[] buffer = new byte[BufferSize];
        public DateTime LastTimeHeard = DateTime.Now;
        public DateTime ConnectTime = DateTime.Now;
        public int ID;
        public bool ShouldDisconnect = false;
        public ClientState State = ClientState.Pending;
        public StateObject OtherStateObject = null;
    }

    static public class ProxyServer
    {
        public const int MAXCLIENTS = 32;
        public delegate bool OnClientConnectHandler(StateObject c);
        public delegate void OnClientDisconnectHandler(StateObject c);


        public static event OnClientDisconnectHandler OnClientDisconnectEvent;
        public static event OnClientConnectHandler OnClientConnectEvent;


        private static System.Threading.Thread _Thread = null;
        private static bool _Running = false;
        private static ManualResetEvent allDone = new ManualResetEvent(false);
        private static StateObject[] Connected = new StateObject[MAXCLIENTS];
        private static int ConnectedCount = 0;
        private static object PendingConnectionsLock = new object();
        private static List<StateObject> PendingConnections = new List<StateObject>();

        public static void Start()
        {
            _Running = true;
            _Thread = new System.Threading.Thread(new System.Threading.ThreadStart(_Run));
            _Thread.Start();

        }
        private static void _Run()
        {
            try
            {
                using(var timer = new System.Timers.Timer())
                {
                    timer.Elapsed += _Timer_Elapsed;
                    timer.Interval = 500;//every half a second 
                    timer.Start();
                    using(var listener = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp))
                    {
                        listener.Bind(new IPEndPoint(IPAddress.Any, 443));
                        listener.Listen(64);

                        while(_Running)
                        {
                            allDone.Reset();

                            Debug.WriteLine("Waiting for a connection...");
                            listener.BeginAccept(new AsyncCallback(AcceptCallback), listener);
                            allDone.WaitOne();
                        }
                        Debug.WriteLine("Exiting  listening ..");
                    }
                    timer.Stop();
                }
            } catch(Exception e)
            {
                Debug.WriteLine(e.ToString());
            }

        }

        static void _Timer_Elapsed(object sender, System.Timers.ElapsedEventArgs e)
        {
            for(var i = 0; i < Connected.Length; i++)
            {
                if(Connected[i] == null)
                    continue;//allready disconnected

                if(Connected[i].ShouldDisconnect || !IsSocketConnected(Connected[i].Sock) || (DateTime.Now - Connected[i].LastTimeHeard).TotalSeconds > 60 * 3)
                {
                    Disconnect(Connected[i]);
                }
            }
            var tmp = new List<StateObject>();
            lock(PendingConnectionsLock)
            {
                foreach(var item in PendingConnections)
                {
                    if(item == null)
                        continue;//allready disconnected

                    if(item.ShouldDisconnect || !IsSocketConnected(item.Sock) || (DateTime.Now - item.LastTimeHeard).TotalSeconds > 60 * 3)
                    {
                        tmp.Add(item);
                    }
                }
            }
            foreach(var item in tmp)
                Disconnect(item);
                
      
        }
        static bool IsSocketConnected(Socket s)
        {
            return !((s.Poll(1000, SelectMode.SelectRead) && (s.Available == 0)) || !s.Connected);
        }
        public static void Stop()
        {
            allDone.Set();//ensure the main loop starts back up
            _Running = false;
            if(_Thread != null)
                _Thread.Join(3000);
            _Thread = null;
        }
        public static void AcceptCallback(IAsyncResult ar)
        {

            allDone.Set();

            Socket listener = (Socket)ar.AsyncState;
            Socket handler = null;
            try
            {
                handler = listener.EndAccept(ar);
            } catch(Exception e)
            {
                if(handler != null)
                    handler.Dispose();
                return;
            }
            if(ConnectedCount >= MAXCLIENTS)
            {
                Debug.WriteLine("Too man clients disconnecting new client ");
                handler.Close();
                return;
            }
            handler.NoDelay = true;

            StateObject state = new StateObject();
            state.Sock = handler;
            lock(PendingConnectionsLock)
            {
                PendingConnections.Add(state);
            }
            try
            {
                handler.BeginReceive(state.buffer, 0, StateObject.BufferSize, 0, new AsyncCallback(ReadCallback), state);
            } catch(Exception e)
            {
                if(handler != null)
                    state.LastTimeHeard = DateTime.Now.AddDays(-1);// main loop will deal with disconnects
            }
        }
        //SOO MANY try blocks below... its painful to see how C# handles stuff like this... ugh..
        public static void ReadCallback(IAsyncResult ar)
        {
            StateObject state = (StateObject)ar.AsyncState;
            state.LastTimeHeard = DateTime.Now;
            Socket handler = state.Sock;
            try
            {
                state.BufferCount += handler.EndReceive(ar);
                state.LastTimeHeard = DateTime.Now;
            } catch(Exception e)
            {
                Disconnect(state);
                Debug.WriteLine(e.Message);
                return;
            }
            var otherside = state.OtherStateObject;
            if(state.BufferCount > 0)
            {
                if(otherside != null)
                {//connection is paired.. send away
                    try
                    {
                        otherside.Sock.Send(state.buffer, state.BufferCount, SocketFlags.None);
                        state.BufferCount = 0;
                    } catch(Exception e)
                    {
                        Disconnect(otherside);
                        Debug.WriteLine(e.Message);
                        return;
                    }
                } // else connection is not paired.. examine header to see if a pairing should be made, or buffer data
                else
                {
                    if(state.BufferCount >= 4)
                    {//if there are 4 bytes, then check it out to see if a pairing should be made
                        var id = BitConverter.ToInt32(state.buffer, 0);
                        if(id >= 0 && id < MAXCLIENTS)
                        {
                            Debug.WriteLine("Attempting to pair connections " + id);
                            lock(PendingConnectionsLock)
                            {
                                PendingConnections.Remove(state);
                            }
                            state.State = ClientState.PendingPair;
                            state.ID = id;
                            if(PendingConnections[id] != null)
                            {
                                if(OnClientConnectEvent(state))
                                {
                                    Connected[id] = state;
                                    ConnectedCount += 1;
                                } else
                                {
                                    Disconnect(state);
                                    return;
                                }
                            } else {
                                Disconnect(state);
                                return;
                            }
                        }
                    }
                }
            }
            try
            {
                handler.BeginReceive(state.buffer, state.BufferCount, StateObject.BufferSize - state.BufferCount, 0, new AsyncCallback(ReadCallback), state);
            } catch(Exception e)
            {
                Disconnect(state);
                Debug.WriteLine(e.Message);
            }
        }
        static void Disconnect(StateObject state)
        {
            if(state.State == ClientState.Pending)
            {
                lock(PendingConnectionsLock)
                {
                    PendingConnections.Remove(state);
                }
            } else
            {
                Connected[state.ID] = null;
                ConnectedCount -= 1;
                if(OnClientDisconnectEvent != null)
                    OnClientDisconnectEvent(state);
               
                state.ShouldDisconnect = true;
                state.LastTimeHeard = DateTime.Now.AddDays(-1);// the main loop will catch this and deal with it
            }
            if(state.Sock!=null)  state.Sock.Dispose();
            state.Sock = null;
        }
    }
}