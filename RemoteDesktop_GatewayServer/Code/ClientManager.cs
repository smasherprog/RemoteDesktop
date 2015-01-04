using RemoteDesktop_CSLibrary;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Security.Cryptography;
using System.Web;

namespace RemoteDesktop_GatewayServer.Code
{
    public class ClientManager : IDisposable
    {

        public delegate void OnClientConnectHandler(List<Client> c);
        public delegate void OnClientDisconnectHandler(List<Client> c);
        public delegate void OnPairedHandler(List<Client> c);

        public event OnClientDisconnectHandler OnClientDisconnectEvent;
        public event OnClientConnectHandler OnClientConnectEvent;
        public event OnPairedHandler OnPairedEvent;

        private System.Collections.Concurrent.ConcurrentQueue<int> Ids = new System.Collections.Concurrent.ConcurrentQueue<int>();
        private object ClientsLock = new object();
        private Client_Wrapper[] _Clients = new Client_Wrapper[Constants.MAXCLIENTS];
        private System.Timers.Timer _TimeoutTimer;

        public IEnumerable<Client> Clients
        {
            get
            {
                return _Clients.Where(a => a != null).Select(a => a.ClientObject);
            }
        }

        public ClientManager()
        {
            for(var i = 0; i < Constants.MAXCLIENTS; i++)
                Ids.Enqueue(i);
            _TimeoutTimer = new System.Timers.Timer();
            _TimeoutTimer.Interval = 5000;//every 5 seconds check to possibly remove a server that is in a reserved state but has never completed the connection, or viewers who have not connected either
            _TimeoutTimer.Elapsed += _TimeoutTimer_Elapsed;
            _TimeoutTimer.Start();

        }
        public void Dispose()
        {
            if(_TimeoutTimer != null)
            {
                _TimeoutTimer.Stop();
                _TimeoutTimer.Dispose();
                _TimeoutTimer = null;
            }
        }
        void _TimeoutTimer_Elapsed(object sender, System.Timers.ElapsedEventArgs e)
        {
            var li = new List<Client>();
            lock(ClientsLock)
            {
                for(var i = 0; i < _Clients.Length; i++)
                {
                    var obj = _Clients[i];
                    if(obj != null)
                    {
                        var dt = (DateTime.Now - obj.ClientObject.ConnectTime).TotalSeconds;

                        if(dt > Constants.SERVER_DISCONNECT_TIMEOUT && obj.ClientObject.Host == Client.Host_Type.Server && obj.ClientObject.Status != Client.Connection_Status.Paired)//disconnect servers if they have not connected within a timeout
                        {
                            li.Add(obj.ClientObject);
                            obj.SocketObject.ShouldDisconnect = true;// this will cause the server to disconnect, if it is connected
                            _Clients[i] = null;
                            Ids.Enqueue(i);
                        } else if(dt > Constants.VIEWER_DISCONNECT_TIMEOUT && obj.ClientObject.Host == Client.Host_Type.Viewer && obj.ClientObject.Status != Client.Connection_Status.Paired)
                        {
                            obj.SocketObject.ShouldDisconnect = true;// this will cause the viewer to disconnect
                        }

                    }
                }
            }
            if(li.Any() && OnClientDisconnectEvent != null)
                OnClientDisconnectEvent(li);
        }
        public List<Client_Wrapper> Add(ServerSocket c, int dst_id, int src_id)
        {
            //BEGING VALIDATION 
            Debug.WriteLine("Attempting to pair connections dst_id " + dst_id.ToString() + "src_id " + src_id);

            var newclient = new Client_Wrapper();
            newclient.SocketObject = c;
            newclient.ClientObject.ConnectTime = DateTime.Now;


            if(dst_id == -1)
                newclient.ClientObject.Host = Client.Host_Type.Server;
            else if(src_id == -1)
            {
                newclient.ClientObject.Host = Client.Host_Type.Viewer;
                //servers have their info set when they get an ID, viewers dont until they are added
                var remoteIpEndPoint = newclient.SocketObject.SocketObject.RemoteEndPoint as System.Net.IPEndPoint;
                newclient.ClientObject.Firewall_IP = remoteIpEndPoint.Address.ToString();
                newclient.ClientObject.Firewall_Port = remoteIpEndPoint.Port.ToString();

            } else
            {
                c.ShouldDisconnect = true;
                Debug.WriteLine("Disconnecting connection because of unknown client type");
                return null;
            }
            //attempt to pair up!
            //check id range
            if(newclient.ClientObject.Host == Client.Host_Type.Viewer && (dst_id < 0 || dst_id >= Constants.MAXCLIENTS))
            {
                c.ShouldDisconnect = true;
                Debug.WriteLine("Disconnecting connection because a viewer cannot connect to a value less than 0 Or greater than " + Constants.MAXCLIENTS.ToString());
                return null;
            }
            if(newclient.ClientObject.Host == Client.Host_Type.Server && (src_id < 0 || dst_id >= Constants.MAXCLIENTS))
            {
                c.ShouldDisconnect = true;
                Debug.WriteLine("Disconnecting connection because a server cannot have a src_id to a value less than 0Or greater than " + Constants.MAXCLIENTS.ToString());
                return null;
            }
            //ids are going to be corrected below
            newclient.ClientObject.Dst_ID = dst_id;
            newclient.ClientObject.Src_ID = src_id;
            List<Client_Wrapper> ret = null;
            //check if ids are already in use
            //by now the range of the IDs is good and the code and goto the next step
            if(newclient.ClientObject.Host == Client.Host_Type.Viewer)
            {
                ret = AddViewer(newclient);
            } else if(newclient.ClientObject.Host == Client.Host_Type.Server)
            {
                ret = AddServer(newclient);
            } else
            {
                Debug.Assert(false);
            }
            if(ret != null)
            {//either a pairing occured or a client connected
                if(ret.Count == 1)
                {
                    if(OnClientConnectEvent != null)
                        OnClientConnectEvent(ret.Select(a => a.ClientObject).ToList());
                } else if(ret.Count == 2)
                {
                    if(OnPairedEvent != null)
                        OnClientConnectEvent(ret.Select(a => a.ClientObject).ToList());
                } else
                    Debug.Assert(false);//never hit this!!
            }
            return ret;
        }
        List<Client_Wrapper> AddViewer(Client_Wrapper wrapper)
        {

            lock(ClientsLock)
            {//ensure the Remove Function doesnt interleave here!!
                var possibleserver = _Clients[wrapper.ClientObject.Dst_ID];
                if(possibleserver != null)
                {
                    if(possibleserver.ClientObject.Host == Client.Host_Type.Server)
                    {//viewers can only connect to servers... Duh!
                        if(possibleserver.ClientObject.Status != Client.Connection_Status.Pending)
                        {//the server has to be in a pending state in order to connect to it
                            Debug.WriteLine("Viewer attempting to connect to server which is not in a pending state. . . setting viewer to disconnect");
                            wrapper.SocketObject.ShouldDisconnect = true;//set to disconnect
                            return null;
                        }

                        wrapper.ClientObject.Src_ID = GetID();
                        if(wrapper.ClientObject.Src_ID < 0)
                        {//if there are no ids left.. get out
                            Debug.WriteLine("There are no more Ids, setting to disconnect");
                            wrapper.SocketObject.ShouldDisconnect = true;//set to disconnect
                            return null;
                        }
                        Debug.WriteLine("Wiring up viewer and server .. Good Connect  . . ");
                        //good  Connect.. add the viewer and pair them up
                        wrapper.ClientObject.Status = possibleserver.ClientObject.Status = Client.Connection_Status.Paired;
                        possibleserver.ClientObject.Dst_ID = wrapper.ClientObject.Src_ID;//match up the ids
                        _Clients[wrapper.ClientObject.Src_ID] = wrapper;
                        return new List<Client_Wrapper> { possibleserver, wrapper };
                    } else
                    {
                        Debug.WriteLine("Viewer attempting to connect to a non server.. this doesnt make sense.. setting viewer to disconnect");
                        wrapper.SocketObject.ShouldDisconnect = true;//set to disconnect
                        return null;
                    }
                } else
                {//no server to connect to just yet.. allow the connection through, and assign a pending state, there is a timeout for non paired connections which will fire off if no pairing is made
                    wrapper.ClientObject.Src_ID = GetID();
                    if(wrapper.ClientObject.Src_ID < 0)
                    {//if there are no ids left.. get out
                        Debug.WriteLine("There are no more Ids, setting to disconnect");
                        wrapper.SocketObject.ShouldDisconnect = true;//set to disconnect
                        return null;
                    }
                    Debug.WriteLine("Viewer connected, but there is no valid server to pair with.");
                    wrapper.ClientObject.Status = Client.Connection_Status.Pending;
                    _Clients[wrapper.ClientObject.Src_ID] = wrapper;
                    return new List<Client_Wrapper> { wrapper };
                }
            }

        }
        List<Client_Wrapper> AddServer(Client_Wrapper wrapper)
        {
            lock(ClientsLock)
            {//ensure the Remove Function doesnt interleave here!!
                var serverobject = _Clients[wrapper.ClientObject.Src_ID];
                if(serverobject == null)
                {
                    Debug.WriteLine("Server attempting to connect with a bad ID, setting to disconnect.");
                    wrapper.SocketObject.ShouldDisconnect = true;
                    return null;
                }
                if(serverobject.ClientObject.Host != Client.Host_Type.Server)
                {
                    Debug.WriteLine("Server attempting to connect with a bad ID, setting to disconnect. Host != Server");
                    wrapper.SocketObject.ShouldDisconnect = true;
                    return null;
                }
                if(serverobject.ClientObject.Status == Client.Connection_Status.Reserved)
                {
                    serverobject.ClientObject.Status = Client.Connection_Status.Pending;
                }
                serverobject.ClientObject.ConnectTime = DateTime.Now;
                serverobject.SocketObject = wrapper.SocketObject;// make sure to set this
                //search for a viewer, that is trying to connect to this server, and in a pending state
                var posviewer = _Clients.FirstOrDefault(a => a != null && a.ClientObject.Host == Client.Host_Type.Viewer && a.ClientObject.Dst_ID == wrapper.ClientObject.Src_ID && a.ClientObject.Status == Client.Connection_Status.Pending);
                if(posviewer != null)
                {//found a viewer!!! pair up!
                    Debug.WriteLine("Viewer found to pair with server.. Wiring up!");
                    serverobject.ClientObject.Status = posviewer.ClientObject.Status = Client.Connection_Status.Paired;
                    serverobject.ClientObject.Dst_ID = posviewer.ClientObject.Src_ID;
                    return new List<Client_Wrapper> { serverobject, posviewer };
                }
                //no viewer found.. just wait . . . 
                return new List<Client_Wrapper> { serverobject };
            }

        }
        int GetID()
        {
            var id = -1;
            if(Ids.TryDequeue(out id))
                return id;
            return -1;
        }
        public void PendingClientDisconnecting(ServerSocket s)
        {

        }
        public void Remove(Client c)
        {
            if(c.Host == Client.Host_Type.Viewer)
                RemoveViewer(c);
            else if(c.Host == Client.Host_Type.Server)
                RemoveServer(c);
            else
                Debug.Assert(false);//this should never be hit!!
        }

        void RemoveViewer(Client c)
        {
            if(c == null)
                return;

            lock(ClientsLock)
            {//ensure the Remove Function doesnt interleave here!!

                if(c.Status == Client.Connection_Status.Paired && c.Dst_ID > -1 && c.Dst_ID < Constants.MAXCLIENTS)
                {//if the connection is paired and there is a valid server
                    var server = _Clients[c.Dst_ID];
                    if(server != null)
                    {
                        server.ClientObject.ConnectTime = DateTime.Now;
                        server.ClientObject.Status = Client.Connection_Status.Reserved;//servers go into a reserved state so they can reconnect and use the same slot
                        server.ClientObject.Dst_ID = -1;//reset its dst id
                    }

                }
                c.Status = Client.Connection_Status.Disconnected;
             
                Ids.Enqueue(c.Src_ID);//add the id in
                _Clients[c.Src_ID] = null;//clear the slot
                c.Src_ID = c.Dst_ID = -1;
            }
            if(OnClientDisconnectEvent != null)
                OnClientDisconnectEvent(new List<Client> { c });
        }
        void RemoveServer(Client c)
        {
            if(c == null)
                return;

            lock(ClientsLock)
            {//ensure the Remove Function doesnt interleave here!!
                if(c.Status == Client.Connection_Status.Paired && c.Dst_ID > -1 && c.Dst_ID < Constants.MAXCLIENTS)
                {//if the connection is paired and there is a valid server
                    var viewer = _Clients[c.Dst_ID];
                    if(viewer != null)
                    {
                        viewer.ClientObject.Status = Client.Connection_Status.Disconnected;
                        viewer.SocketObject.ShouldDisconnect = true;//set this to disconnect the viewer
                        _Clients[c.Dst_ID] = null;//clear the viewer slot
                        Ids.Enqueue(c.Dst_ID);//add the id in the id
                    }
                }
                c.ConnectTime = DateTime.Now;//this is now used as a timeout value
                c.Status = Client.Connection_Status.Reserved;//servers go into a reserved state so they can reconnect and use the same slot
                c.Dst_ID = -1;//reset the dst id as well
            }
            if(OnClientDisconnectEvent != null)
                OnClientDisconnectEvent(new List<Client> { c });
        }
        private static string ByteArrayToString(byte[] ba)
        {
            var hex = new System.Text.StringBuilder(ba.Length * 2);
            foreach(byte b in ba)
                hex.AppendFormat("{0:x2}", b);
            return hex.ToString();
        }
        //this is called by a server to get an ID slot
        public Client ReserveID(string ip, string computername, string username, string mac, string sessionId)
        {

            var id = -1;
            //check for a previous connection
            var prev = _Clients.FirstOrDefault(a => a != null && a.ClientObject.Mac_Address == mac && a.ClientObject.SessionID == sessionId && a.ClientObject.Host == Client.Host_Type.Server && a.ClientObject.Status == Client.Connection_Status.Reserved);//check for a reconnecting client
            if(prev != null)
            {//update any info, computer name can change, usersname, etc...
                prev.ClientObject.UserName = username;
                prev.ClientObject.ComputerName = computername;
                prev.ClientObject.ConnectTime = DateTime.Now;
                prev.ClientObject.Status = Client.Connection_Status.Reserved;

            } else if(Ids.TryDequeue(out id))
            {//new client, get a new id and fill
                prev = new Client_Wrapper();
                prev.ClientObject.ComputerName = computername;
                prev.ClientObject.UserName = username;
                prev.ClientObject.Mac_Address = mac;
                prev.ClientObject.SessionID = sessionId;
                prev.ClientObject.Host = Client.Host_Type.Server;
                prev.ClientObject.ConnectTime = DateTime.Now;
                prev.ClientObject.Src_ID = id;
                prev.ClientObject.Dst_ID = -1;
                prev.ClientObject.Firewall_IP = ip;
                prev.ClientObject.Status = Client.Connection_Status.Reserved;
                using(var aes = new AesManaged())
                {
                    aes.KeySize = 256;
                    aes.GenerateKey();
                    prev.ClientObject.AES_Session_Key = ByteArrayToString(aes.Key);
                }
                _Clients[id] = prev;

            }
            return prev.ClientObject;
        }

    }
}