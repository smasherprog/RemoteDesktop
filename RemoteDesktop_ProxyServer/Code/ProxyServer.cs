
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
    public enum Sides { Left, Right };
    public class StateObject
    {
        public StateObject(Sides s, int id)
        {
            Side = s;
            ID = id;
        }
        public Socket Sock = null;
        public const int BufferSize = 1024 * 1024;
        public int BufferCount = 0;
        public byte[] buffer = new byte[BufferSize];
        public DateTime LastTimeHeard = DateTime.Now;
        public DateTime ConnectTime = DateTime.Now;
        public int ID;
        public Sides Side;
        public bool ShouldDisconnect = false;

    }
    public class Pairing
    {
        public StateObject Left = null;
        public StateObject Right = null;
    }
    static public class ProxyServer
    {
        public delegate void OnClientConnectHandler(StateObject c);
        public delegate void OnClientDisconnectHandler(StateObject c);
        public delegate void OnClientsPairedHandler(Pairing p);

        public static event OnClientDisconnectHandler OnClientDisconnectEvent;
        public static event OnClientConnectHandler OnClientConnectEvent;
        public static event OnClientsPairedHandler OnClientsPairedEvent;

        private static System.Threading.Thread _Thread = null;
        private static bool _Running = false;
        private static ManualResetEvent allDone = new ManualResetEvent(false);
        private static System.Collections.Concurrent.ConcurrentQueue<int> SocketIds = new System.Collections.Concurrent.ConcurrentQueue<int>();
        private static Pairing[] SocketPairs = new Pairing[32];


        public static void Start()
        {

            _Running = true;
            for(var i = 0; i < SocketPairs.Length; i++)
            {
                SocketPairs[i] = new Pairing();
                SocketIds.Enqueue(i);
            }

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
            var i = -1;
            foreach(var item in SocketPairs)
            {   
                i++;
                if(item.Right == null && item.Left == null)
                    continue;//allready disconnected
                if(item.Left != null && (item.Left.ShouldDisconnect || !IsSocketConnected(item.Left.Sock) || (DateTime.Now - item.Left.LastTimeHeard).TotalSeconds > 60 * 3))
                {
                    OnClientDisconnectEvent(item.Left);
                    item.Left.Sock.Dispose();
                    item.Left.Sock = null;
                    item.Left = null;
                }
                if(item.Right != null && (item.Right.ShouldDisconnect || !IsSocketConnected(item.Right.Sock) || (DateTime.Now - item.Right.LastTimeHeard).TotalSeconds > 60 * 3))
                {
                    OnClientDisconnectEvent(item.Right);
                    item.Right.Sock.Dispose();
                    item.Right.Sock = null;
                    item.Right = null;
                }
                if(item.Right == null && item.Left == null)
                    SocketIds.Enqueue(i);
             
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
            if(!SocketIds.Any())
                return;
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
            int id = 0;
            if(!SocketIds.TryDequeue(out id))
            {
                handler.Close();
                return;
            }
            handler.NoDelay = true;

            StateObject state = new StateObject(Sides.Left, id);
            state.Sock = handler;
            SocketPairs[id].Left = state;//always left side first
            Debug.WriteLine("Adding Client to id " + id);
            if(OnClientConnectEvent != null)
                OnClientConnectEvent(state);

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
            } catch(Exception e)
            {
                Disconnect(state);
                Debug.WriteLine(e.Message);
                return;
            }
            StateObject otherside = null;
            if(state.Side == Sides.Left)
                otherside = SocketPairs[state.ID].Right;
            else
                otherside = SocketPairs[state.ID].Left;
            if(SocketPairs[state.ID].Left == null)
            {
                Disconnect(state);
                return;
            }

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
                        if(id >= 0 && id < SocketPairs.Length)
                        {
                            Debug.WriteLine("Attempting to pair connections " + id);
                            if(SocketPairs[id].Right == null)
                            {//good pairing!!
                                Debug.WriteLine("Successful pairing! " + id);

                                otherside = SocketPairs[id].Left;
                                if(otherside != null)
                                {
                                    var oldid = state.ID;
                                    SocketPairs[oldid].Left = null;//this has to be a left node
                                    state.Side = Sides.Right;
                                    state.ID = id;
                                    SocketPairs[id].Right = state;//PAIR UP
                                    SocketIds.Enqueue(oldid);//add this id back to the pool
                                    //send out any pending data
                                    otherside.Sock.Send(state.buffer, state.BufferCount, SocketFlags.None);
                                    state.BufferCount = 0;

                                    if(OnClientsPairedEvent != null)
                                        OnClientsPairedEvent(SocketPairs[id]);
                                    Debug.WriteLine("Finished Successful pairing! " + id);
                                } else
                                {
                                    Disconnect(state);
                                    return;
                                }
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
            StateObject otherside = null;
            if(state.Side == Sides.Left)
                otherside = SocketPairs[state.ID].Right;
            else
                otherside = SocketPairs[state.ID].Left;
            if(otherside != null)
            {
                otherside.ShouldDisconnect = true;
                otherside.LastTimeHeard = DateTime.Now.AddDays(-1);// the main loop will catch this and deal with it
            }
            state.ShouldDisconnect = true;
            state.LastTimeHeard = DateTime.Now.AddDays(-1);// the main loop will catch this and deal with it
        }
    }
}