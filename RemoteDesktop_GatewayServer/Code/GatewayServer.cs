
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Web;

namespace RemoteDesktop_GatewayServer.Code
{

    public class GatewayServer : IDisposable
    {
        private System.Threading.Thread _Thread = null;
        private bool _Running = false;
        private ManualResetEvent allDone = new ManualResetEvent(false);
        private Client_Wrapper[] Connected = new Client_Wrapper[Constants.MAXCLIENTS];// this is an array so that I can perform operations on the container without having to lock it
        private int ConnectedCount = 0;
        private object PendingConnectionsLock = new object();
        private List<ServerSocket> PendingConnections = new List<ServerSocket>();
        private object DisconectLock = new object();
        public ClientManager ClientManager = new ClientManager();

        public GatewayServer()
        {
            _Running = true;
            _Thread = new System.Threading.Thread(new System.Threading.ThreadStart(_Run));
            _Thread.Start();
        }
        public void Dispose()
        {
            _Running = false;
            allDone.Set();//ensure the main loop starts back up
            _Running = false;
            if(_Thread != null)
                _Thread.Join(3000);
            ClientManager.Dispose();
            ClientManager = null;
            _Thread = null;

        }

        private void _Run()
        {
            try
            {
                using(var timer = new System.Timers.Timer())
                {
                    timer.Elapsed += _Timer_Elapsed;
                    timer.Interval = 500;//every half a second 
                    timer.Start();
                    // LOG += "Starting RUN\n";
                    using(var listener = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp))
                    {
                        listener.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
                        var port = System.Configuration.ConfigurationManager.AppSettings["RAT_Gateway_Listen_Port"];
                        if(string.IsNullOrWhiteSpace(port))
                            port = "5939";

                        var intport = Convert.ToInt32(port);
                        //  LOG += "Port "+ port+"\n";
                        listener.Bind(new IPEndPoint(IPAddress.Any, intport));
                        listener.Listen(64);

                        while(_Running)
                        {
                            allDone.Reset();
                            //    LOG += "Waiting for a connection...\n";
                            Debug.WriteLine("Waiting for a connection...");
                            listener.BeginAccept(new AsyncCallback(AcceptCallback), listener);
                            allDone.WaitOne();
                        }
                        Debug.WriteLine("Exiting  listening ..");
                    }
                    //CLEANUP !!!!
                    timer.Stop();
                    lock(PendingConnectionsLock)
                    {
                        foreach(var item in PendingConnections)
                        {
                            item.SocketObject.Dispose();
                        }
                    }
                }
            } catch(Exception e)
            {
                Debug.WriteLine(e.ToString());
            }

        }
        void ProcessPossibleConnectDisconnects()
        {
            foreach(var item in Connected.Where(a => a != null))
            {
                if(item.SocketObject.ShouldDisconnect)
                {
                    Debug.WriteLine("Disconnecting a connected client because ShouldDisconnect == true");
                    Disconnect(item);
                } else if(!IsSocketConnected(item.SocketObject.SocketObject))
                {
                    Debug.WriteLine("Disconnecting a connected client because  !IsSocketConnected(item.SocketObject.SocketObject) == true");
                    Disconnect(item);
                } else if((DateTime.Now - item.SocketObject.LastTimeHeard).TotalSeconds > Constants.VIEWER_DISCONNECT_TIMEOUT)
                {
                    Debug.WriteLine("Disconnecting a connected client because timeout has been hit");
                    Disconnect(item);
                }
            }
        }
        //Assumes a lock on the pendingconnections container....
        void ProcessPossiblePendingConnectionDisconnects()
        {
            var tmp = new List<ServerSocket>();
            foreach(var item in PendingConnections)
            {
                if(!IsSocketConnected(item.SocketObject))
                {
                    Debug.WriteLine("Disconnecting a Pending client because  !IsSocketConnected(item.SocketObject.SocketObject) == true");
                    tmp.Add(item);
                } else if((DateTime.Now - item.LastTimeHeard).TotalSeconds > Constants.VIEWER_DISCONNECT_TIMEOUT)
                {
                    Debug.WriteLine("Disconnecting a Pending client because timeout has been hit");
                    tmp.Add(item);
                }
            }
            //remove from pending buffer and dispose.. 
            foreach(var item in tmp)
            {
                PendingConnections.RemoveAll(a => a == item);
                item.SocketObject.Dispose();
            }
        }
        void _Timer_Elapsed(object sender, System.Timers.ElapsedEventArgs e)
        {
            try
            {
                //check on disconnects
                ProcessPossibleConnectDisconnects();
                var tmp = new List<ServerSocket>();
                var connecting_clients = new List<List<Client_Wrapper>>();
                lock(PendingConnectionsLock)
                {
                    ProcessPossiblePendingConnectionDisconnects();
                    //check on any pairing of connections
                    foreach(var item in PendingConnections)
                    {
                        //these are non blocking sockets so this will return immediately
                        try
                        {
                            item.BufferCount += item.SocketObject.Receive(item.buffer, item.BufferCount, Constants.GATEWAY_HEADER_SIZE - item.BufferCount, SocketFlags.None);
                        } catch(System.Net.Sockets.SocketException ex)
                        {
                            if(ex.ErrorCode != Constants.WSAEWOULDBLOCK)
                            {//make sure to ig
                                Debug.WriteLine("Disconnecting Pending Client error in receive");
                                Debug.WriteLine(ex.Message);
                                tmp.Add(item);
                                continue;
                            }
                        }
                        var possibleclients = PairUp(item);
                        if(possibleclients != null)
                        {//this means a possible pairing was made, or a single connection was made
                            connecting_clients.Add(possibleclients);
                        }
                    }
                    //remove any disconnected clients
                    foreach(var item in tmp)
                    {
                        PendingConnections.RemoveAll(a => a == item);
                        item.SocketObject.Close();
                        item.SocketObject.Dispose();
                    }
                    tmp.Clear();
                    //add any connected clients
                    foreach(var item in connecting_clients)
                    {//remove from pending connects and move into the connected
                        foreach(var subitem in item)
                        {
                            PendingConnections.RemoveAll(a => a == subitem.SocketObject);
                            if(Connected[subitem.ClientObject.Src_ID] != null)
                            {
                                if(Connected[subitem.ClientObject.Src_ID] != subitem)
                                {
                                    Debug.WriteLine("Bad Connect");
                                }
                            }
                            subitem.SocketObject.ShouldDisconnect = false;//just in case
                            Connected[subitem.ClientObject.Src_ID] = subitem;
                        }
                    }
                }
                //send out any pending data
                foreach(var item in connecting_clients)
                { //send any lingering data and begin the async receive if paired

                    if(item.Count == 2)
                    {//a pair was made... wire up the sockets and get started,otherwise, it will be in the Connected list and sit there until a timeout or a pairing

                        var left = item.FirstOrDefault();
                        var right = item.LastOrDefault();
                        Debug.Assert(left != null && right != null);

                        left.SocketObject.OtherSocketObject = right.SocketObject;
                        //neeed to block when its to anasync call.. wierd, isnt it?
                        left.SocketObject.SocketObject.Blocking = true;
                        right.SocketObject.SocketObject.Blocking = true;
                        right.SocketObject.OtherSocketObject = left.SocketObject;

                        try
                        {
                            Debug.WriteLine("Beginning Pair Send and async");
                            Debug.WriteLine(left.SocketObject.BufferCount + " bytes of data to send on left");
                            Debug.WriteLine(right.SocketObject.BufferCount + " bytes of data to send on right");
                            Send(left);
                            Send(right);

                            BeginReceive(left);
                            BeginReceive(right);

                        } catch(Exception ex)
                        {
                            Debug.WriteLine("Disconnecting because of an error in initial pairing send");
                            Disconnect(left);//this will disconnect both clients in case of a thrown error
                            Debug.WriteLine(ex.Message);
                        }

                    } else
                    {
                        Debug.WriteLine("Client " + item.FirstOrDefault().ClientObject.Src_ID + " added to connected list, but was not paired");
                    }

                }
            } catch(Exception ex)
            {
                Debug.WriteLine(ex.Message);
            }

        }
        static int Get_DisconnectTime(Client_Wrapper o)
        {
            if(o == null)
                return Constants.VIEWER_DISCONNECT_TIMEOUT;
            if(o.ClientObject == null)
                return Constants.VIEWER_DISCONNECT_TIMEOUT;
            if(o.ClientObject.Host == RemoteDesktop_CSLibrary.Client.Host_Type.Server)
                return Constants.SERVER_DISCONNECT_TIMEOUT;
            else
                return Constants.VIEWER_DISCONNECT_TIMEOUT;
        }
        List<Client_Wrapper> PairUp(ServerSocket state)
        {
            if(state.BufferCount >= Constants.GATEWAY_HEADER_SIZE)
            {//if there is enough data, try to pair up
                //the ids are validated in the manager class
                return ClientManager.Add(state, BitConverter.ToInt32(state.buffer, 0), BitConverter.ToInt32(state.buffer, 4));
            }
            return null;
        }
        static byte[] testbuffer = new byte[1];
        static bool IsSocketConnected(Socket s)
        {
            if(s == null)
                return false;
            try
            {
                bool part1 = s.Poll(1000, SelectMode.SelectRead);
                bool part2 = (s.Available == 0);
                if(part1 && part2)
                    return false;
                else
                    return true;
            } catch(System.Net.Sockets.SocketException e)
            {
                if(e.ErrorCode != Constants.WSAEWOULDBLOCK)
                {
                    Debug.WriteLine(e.Message);
                    return false;
                }
                return true;
            }
        }

        void AcceptCallback(IAsyncResult ar)
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
            if(ConnectedCount >= Constants.MAXCLIENTS)
            {
                Debug.WriteLine("Too man clients disconnecting new client ");
                handler.Close();
                return;
            }
            handler.NoDelay = true;
            handler.Blocking = false;
            var state = new ServerSocket();
            state.SocketObject = handler;

            Debug.WriteLine("Accept Pending Connection");
            lock(PendingConnectionsLock)
            {
                PendingConnections.Add(state);
            }
        }
        //SOO MANY try blocks below... its painful to see how C# handles stuff like this... ugh..

        void ReadCallback(IAsyncResult ar)
        {
            var state = (Client_Wrapper)ar.AsyncState;
            Socket handler = state.SocketObject.SocketObject;
            state.SocketObject.LastTimeHeard = DateTime.Now;
            if(state.ClientObject.Status != RemoteDesktop_CSLibrary.Client.Connection_Status.Paired)
                return;// not in a valid state for receiving data....
            try
            {
                state.SocketObject.BufferCount += handler.EndReceive(ar);
                Send(state);
                BeginReceive(state);
            } catch(Exception e)
            {
                Debug.WriteLine("Disconnecting because of an error in ReadCallback");
                Disconnect(state);
                Debug.WriteLine(e.Message);
            }
        }
        void BeginReceive(Client_Wrapper state)
        {
            state.SocketObject.SocketObject.BeginReceive(state.SocketObject.buffer, state.SocketObject.BufferCount, state.SocketObject.buffer.Length - state.SocketObject.BufferCount, 0, new AsyncCallback(ReadCallback), state);
        }

        static void Send(Client_Wrapper c)//this is synchronous .. SUE ME!
        {
            if(c.SocketObject.BufferCount > 0 && c.SocketObject.OtherSocketObject != null)
            {
                var otherside = c.SocketObject.OtherSocketObject;
                otherside.LastTimeHeard = DateTime.Now;
                int offset = 0;
                while(offset < c.SocketObject.BufferCount)
                {
                    offset += otherside.SocketObject.Send(c.SocketObject.buffer, offset, c.SocketObject.BufferCount - offset, SocketFlags.None);
                }
                c.SocketObject.BufferCount = 0;
            }
        }
        void Disconnect(Client_Wrapper state)
        {
            try
            {
                //a lock is needed because a server or viewer can possibly disconnect at a very close time so I cannot have both calling at the same time
                lock(DisconectLock)
                {
                    if(state.SocketObject.Disconnected)
                        return;//already done
                    state.SocketObject.Disconnected = true;

                    Debug.WriteLine("Disconnecting " + state.ClientObject.Firewall_IP + ":" + state.ClientObject.Firewall_Port.ToString());
                    Debug.Assert(Connected[state.ClientObject.Src_ID] != null);
                    Connected[state.ClientObject.Src_ID].SocketObject.SocketObject.Dispose();
                    Connected[state.ClientObject.Src_ID] = null;

                    var otherid = state.ClientObject.Dst_ID;
                    if(otherid >= 0)
                    {
                        Debug.WriteLine("Disconnecting " + Connected[otherid].ClientObject.Firewall_IP + ":" + Connected[otherid].ClientObject.Firewall_Port.ToString());
                        Connected[otherid].SocketObject.SocketObject.Dispose();
                        Connected[otherid].SocketObject.Disconnected = true;
                        Connected[otherid] = null;
                    }
                }
                //now that the cleanup is done in this area, tell the manager, which will free up the hew IDS 
                ClientManager.Remove(state.ClientObject);

            } catch(Exception e)
            {
                Debug.WriteLine("Error In Disconnect");
                Debug.WriteLine(e.Message);
            }
        }
    }
}