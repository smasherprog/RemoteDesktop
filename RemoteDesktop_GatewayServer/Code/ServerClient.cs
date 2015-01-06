using RemoteDesktop_CSLibrary;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Net.Sockets;
using System.Web;

namespace RemoteDesktop_GatewayServer.Code
{
    public static class Constants{
        public const int MAXCLIENTS = 32;
        public const int VIEWER_DISCONNECT_TIMEOUT = 10;//seconds
        public const int SERVER_DISCONNECT_TIMEOUT = 500;//seconds
        public const int GATEWAY_HEADER_SIZE = 138;//in bytes  dst_id and src_id and encryption keys
        public const int WSAEWOULDBLOCK = 10035;
        public const int WSAEMSGSIZE = 10049;
    }
  
    public class ServerSocket
    {
        public bool Disconnected = false;   
        public bool ShouldDisconnect = false;
        public DateTime LastTimeHeard = DateTime.Now;
        public const int BufferSize = 1024 * 1024;
        public int BufferCount = 0;
        public byte[] buffer = new byte[BufferSize];
        public Socket SocketObject = null;
        public ServerSocket OtherSocketObject = null;
    }

    public class Client_Wrapper
    {
        public Client ClientObject = new Client();
        public ServerSocket SocketObject = new ServerSocket();
    }
}