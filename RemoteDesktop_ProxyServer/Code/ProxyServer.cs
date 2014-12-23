
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
        public int Src_ID;
        public int Dst_ID;
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
        private static StateObject[] Connected = new StateObject[MAXCLIENTS];// this is an array so that I can perform operations on the container without having to lock it
        private static int ConnectedCount = 0;
        private static object PendingConnectionsLock = new object();
        private static object DisconectLock = new object();
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

            foreach(var item in Connected.Where(a => a != null && a.State == ClientState.PendingPair && a.Dst_ID > -1))
            {//try to match up any connections if they are pending, a viewer might be waiting on a server, or visa versa
                Pair(item);
            }
        }
        static void Pair(StateObject state)
        {  //attempt to pair up!
            if(state.Dst_ID < 0 || state.Dst_ID > MAXCLIENTS)
                return;// invalid index
            var otherstate = Connected[state.Dst_ID];//get the other if it exists
            if(otherstate != null)
            {//PAIR UP!!
                //send out any data before the pairing is complete to prevent any races
                otherstate.Sock.Send(state.buffer, state.BufferCount, SocketFlags.None);
                state.BufferCount = 0;
                lock(DisconectLock)
                {//protect this state
                    state.OtherStateObject = otherstate;
                    otherstate.OtherStateObject = state;
                    otherstate.State = state.State = ClientState.Paired;
                }
                Debug.WriteLine("PAIRED!");
            }

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
            Debug.WriteLine("Accept Connection");
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
                    if(state.BufferCount >= 8 && state.State == ClientState.Pending)
                    {//if there are 8 bytes, then check it out to see if a pairing should be made
                        var dst_id = BitConverter.ToInt32(state.buffer, 0);
                        var src_id = BitConverter.ToInt32(state.buffer, 4);
                        if(dst_id > -1 && Connected.Any(a => a != null && a.Dst_ID == dst_id))
                        {
                            Debug.WriteLine("Disconnecting connection because the dst id is already in use");
                            Disconnect(state);
                            return;//get out this client is disconnected
                        }
                        Debug.WriteLine("Attempting to pair connections " + src_id);
                        lock(PendingConnectionsLock)
                        {
                            PendingConnections.Remove(state);
                        }
                       
                        state.Src_ID = src_id;
                        state.Dst_ID = dst_id;

                        if(OnClientConnectEvent(state))
                        {//upper level will set src_id to a valid value
                            Connected[state.Src_ID] = state; 
                            state.State = ClientState.PendingPair;
                            if(state.Dst_ID != -1)
                                Pair(state);

                            ConnectedCount += 1;
                        } else
                        {
                            Debug.WriteLine("Disconnecting connection because the OnClientConnectEvent returned false");
                            Disconnect(state);
                            return;
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
            state.ShouldDisconnect = true;
            state.LastTimeHeard = DateTime.Now.AddDays(-1);
            if(state.State == ClientState.Pending)
            {
                lock(PendingConnectionsLock)
                {
                    PendingConnections.Remove(state);
                }
            }
            if(state.State != ClientState.Pending)
            {
                Connected[state.Src_ID] = null;
                ConnectedCount -= 1;
                if(OnClientDisconnectEvent != null)
                    OnClientDisconnectEvent(state);

                StateObject other = null;
                lock(DisconectLock)
                {
                    //exclusive access to clear both sides
                    other = state.OtherStateObject;//make a copy
                    state.OtherStateObject = null;

                    if(other != null)
                        other.OtherStateObject = null;//dont let it come back
                }
                //disconnect the other side
                if(other != null)
                {
                    Connected[other.Src_ID] = null;
                    ConnectedCount -= 1;
                    if(OnClientDisconnectEvent != null)
                        OnClientDisconnectEvent(other);
                    if(other.Sock != null)
                        other.Sock.Dispose();
                    other.ShouldDisconnect = true;
                    other.LastTimeHeard = DateTime.Now.AddDays(-1);
                    other.Sock = null;
                }


            }
            if(state.Sock != null)
                state.Sock.Dispose();
            state.OtherStateObject = null;// make sure to clear this as well
            state.Sock = null;
        }
    }
}