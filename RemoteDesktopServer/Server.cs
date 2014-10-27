using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;

namespace RemoteDesktopServer
{
    public class Server: IDisposable
    {
        private IntPtr _Server = IntPtr.Zero;

        public RemoteDesktopServer.Server_PInvoke.OnConnectCallback OnConnectEvent;

        public Server(ushort port=443)
        {
            _Server = Server_PInvoke.CreateServer();
            OnConnectEvent = Server_OnConnectEvent;
            Server_PInvoke.SetOnConnectCallback(_Server, OnConnectEvent);
        }

        void Server_OnConnectEvent()
        {
            Debug.WriteLine("client connected");
        }
        public void Listen(ushort port = 443)
        {
            Server_PInvoke.Listen(_Server, port);
   
        }




        public void Dispose()
        {
            Server_PInvoke.DestroyServer(_Server);
        }
    }
}
