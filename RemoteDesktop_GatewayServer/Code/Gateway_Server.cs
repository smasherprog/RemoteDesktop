using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Web;

namespace RemoteDesktop_GatewayServer.Code
{
    public class Gateway_Server : IDisposable
    {
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void _OnConnect();

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void _OnDisconnect();

        [DllImport(RemoteDesktop_CSLibrary.Config.DLL_Name, CallingConvention = CallingConvention.StdCall)]
        static extern IntPtr Create_Server(_OnConnect onconnect, _OnDisconnect ondisconnect);

        [DllImport(RemoteDesktop_CSLibrary.Config.DLL_Name, CallingConvention = CallingConvention.StdCall)]
        static extern IntPtr Destroy_Server(IntPtr server);

        [DllImport(RemoteDesktop_CSLibrary.Config.DLL_Name, CallingConvention = CallingConvention.StdCall, CharSet = CharSet.Unicode)]
        static extern IntPtr Listen(IntPtr server, string ip_or_host, string port);

        public delegate void OnConnectHandler();
        public delegate void OnDisconnectHandler();

        public event OnConnectHandler OnConnectEvent;
        public event OnDisconnectHandler OnDisconnectEvent;

        private _OnConnect OnConnect_CallBack;
        private _OnDisconnect OnDisconnect_CallBack;

        private IntPtr _Server = IntPtr.Zero;


        public Gateway_Server()
        {
            var port = System.Configuration.ConfigurationManager.AppSettings["RAT_Gateway_Listen_Port"];
            if(string.IsNullOrWhiteSpace(port))
                port = "5939";         
            
            OnConnect_CallBack = OnConnect;
            OnDisconnect_CallBack = OnDisconnect;

            _Server = Create_Server(OnConnect_CallBack, OnDisconnect_CallBack);
            Listen(_Server, "", port);

        }
    
        public void Dispose()
        {
            Destroy_Server(_Server);
            _Server = IntPtr.Zero;
        }
        private void OnConnect()
        {

        }
        private void OnDisconnect()
        {

        }
    }
}