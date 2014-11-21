using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;

namespace RemoteDesktop_Viewer.Code
{
    public class Client_Wrapper : IDisposable
    {
        [DllImport("RemoteDesktopViewer_Library.dll")]
        static extern IntPtr Create_Client(IntPtr hwnd);
        [DllImport("RemoteDesktopViewer_Library.dll")]
        static extern void Destroy_Client(IntPtr client);
        [DllImport("RemoteDesktopViewer_Library.dll", CharSet = CharSet.Unicode)]
        static extern void Connect(IntPtr client, string ip_or_host, string port);
        [DllImport("RemoteDesktopViewer_Library.dll")]
        static extern void Draw(IntPtr client, IntPtr hdc);
        [DllImport("RemoteDesktopViewer_Library.dll")]
        static extern void KeyEvent(IntPtr client, int VK, bool down);
        [DllImport("RemoteDesktopViewer_Library.dll")]
        static extern void MouseEvent(IntPtr client, int action, int x, int y, int wheel);
        [DllImport("RemoteDesktopViewer_Library.dll")]
        static extern void SendCAD(IntPtr client);

        IntPtr _Client = IntPtr.Zero;
        ViewPort _Viewport = null;
        public Client_Wrapper(ViewPort viewport)
        {
            _Client = Create_Client(viewport.Handle);
            viewport.OnDraw_CB = Draw;
            _Viewport = viewport;
        }

        public void Connect(string ip_or_host, string port)
        {
            Connect(_Client, "192.168.221.128", "443");
        }
        public void Draw(IntPtr hdc)
        {
            Draw(_Client, hdc);
        }
        public void KeyEvent(int VK, bool down)
        {
            KeyEvent(_Client, VK, down);
        }
        public void MouseEvent(int action, int x, int y, int wheel)
        {
            MouseEvent(_Client, action, x, y, wheel);
        }
        public void SendCAD()
        {
            SendCAD(_Client);
        }
        public void Dispose()
        {
            Destroy_Client(_Client);
            _Viewport.OnDraw_CB = null;
        }

    }
}
