using RemoteDesktop_Viewer.Code;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

namespace RemoteDesktop_Viewer
{
    public partial class MainViewer : Form
    {
        [StructLayout(LayoutKind.Sequential)]
        public struct Traffic_Stats
        {
            public long CompressedSendBytes;
            public long CompressedRecvBytes;//overall lifetime totals 
            public long UncompressedSendBytes;

            public long UncompressedRecvBytes;//overall lifetime totals 

            public long CompressedSendBPS;
            public long CompressedRecvBPS;
            public long UncompressedSendBPS;
            public long UncompressedRecvBPS;
        }

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void _OnConnect();

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void _OnDisconnect();

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void _OnCursorChanged(int c_type);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void _OnDisplayChanged(int x, int y);

        [DllImport("RemoteDesktopViewer_Library.dll", CallingConvention = CallingConvention.StdCall)]
        static extern IntPtr Create_Client(IntPtr hwnd, _OnConnect onconnect, _OnDisconnect ondisconnect, _OnCursorChanged oncursorchange, _OnDisplayChanged ondisplaychanged);
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
        [DllImport("RemoteDesktopViewer_Library.dll", CharSet = CharSet.Ansi)]
        static extern void SendFile(IntPtr client, string absolute_path, string relative_path);
        [DllImport("RemoteDesktopViewer_Library.dll")]
        static extern Traffic_Stats get_TrafficStats(IntPtr client);

        public delegate void OnConnectHandler();
        public delegate void OnDisconnectHandler();
        public delegate void OnCursorChangedHandler(int c_type);

        public event OnConnectHandler OnConnectEvent;
        public event OnDisconnectHandler OnDisconnectEvent;
        public event OnCursorChangedHandler OnCursorChangedEvent;

        private _OnConnect OnConnect_CallBack;
        private _OnDisconnect OnDisconnect_CallBack;
        private _OnCursorChanged OnCursorChanged_CallBack;
        private _OnDisplayChanged OnDisplayChanged_CallBack;

        private IntPtr _Client = IntPtr.Zero;
        private object _PendingFiles_Lock = new object();
        private List<List<string>> _PendingFiles = new List<List<string>>();
        private bool Running = false;
        private System.Threading.Thread _FileSendingThread = null;
        private System.Timers.Timer _TrafficTimer;
        private string _Host_Address;
        public string Host_Address { get { return _Host_Address; } }
        InputListener _InputListener = null;
        public MainViewer()
        {
            InitializeComponent();
            FormClosing += MainViewer_FormClosing;
            FormClosed += MainViewer_FormClosed;
            _InputListener = new InputListener(viewPort1.Handle);
            _InputListener.InputKeyEvent = KeyEvent;
            _InputListener.InputMouseEvent = MouseEvent;
            DragDrop += new DragEventHandler(Form1_DragDrop);
            DragEnter += new DragEventHandler(Form1_DragEnter);

            Application.AddMessageFilter(_InputListener);
            viewPort1.OnDraw_CB = Draw;

            //Must keep references 
            OnConnect_CallBack = OnConnect;
            OnDisconnect_CallBack = OnDisconnect;
            OnCursorChanged_CallBack = OnCursorChanged;
            OnDisplayChanged_CallBack = OnDisplayChanged;

            _Client = Create_Client(viewPort1.Handle, OnConnect_CallBack, OnDisconnect_CallBack, OnCursorChanged_CallBack, OnDisplayChanged_CallBack);
            Running = true;

        }

        void _TrafficTimer_Elapsed(object sender, System.Timers.ElapsedEventArgs e)
        {
            var traffic = get_TrafficStats(_Client);
            var outcompressionratio = 0.0f;
            var incompressionratio = 0.0f;
            //below i change the range of the data shown and convert it to a percent, For example, .003 changes to 3.0, then -97.0, then 97.0 
            //I want to display the Effectivness of the compression below... In other words, the compression ratio is 97% effective.
            if(traffic.UncompressedSendBPS > 0)
                outcompressionratio = ((((float)traffic.CompressedSendBPS / (float)traffic.UncompressedSendBPS) * 100.0f) - 100.0f) * -1.0f;
            if(traffic.UncompressedRecvBPS > 0)
                incompressionratio = ((((float)traffic.CompressedRecvBPS / (float)traffic.UncompressedRecvBPS) * 100.0f) - 100.0f) * -1.0f;

            this.UIThread(() =>
            {
                this.Text = "Connected to: " + _Host_Address + ":443,  Out: " + FormatBytes(traffic.CompressedSendBPS) + "/s In: " + FormatBytes(traffic.CompressedRecvBPS) + "/s Comp Rate Out: " + string.Format("{0:0.00}", outcompressionratio) + "% In: " + string.Format("{0:0.00}", incompressionratio) + "%";
            });
        }


        private void Form1_DragEnter(object sender, DragEventArgs e)
        {
            // If the data is a file or a bitmap, display the copy cursor. 
            if(e.Data.GetDataPresent(DataFormats.Bitmap) ||
               e.Data.GetDataPresent(DataFormats.FileDrop))
            {
                e.Effect = DragDropEffects.Copy;
            } else
            {
                e.Effect = DragDropEffects.None;
            }
        }
        private void OnCursorChanged(int c_type)
        {
            if(OnCursorChangedEvent != null)
                OnCursorChangedEvent(c_type);
            viewPort1.UIThread(() =>
            {
                viewPort1.Cursor = CursorManager.Get_Cursor(c_type);
            });
        }
        private void OnDisplayChanged(int x, int y)
        {
            var maxh = y + 10;
            var maxx = x + 14;
            if(maxx > Screen.PrimaryScreen.Bounds.Width - 40)
                maxx = Screen.PrimaryScreen.Bounds.Width - 40;
            if(maxh > Screen.PrimaryScreen.Bounds.Height - 50)
                maxh = Screen.PrimaryScreen.Bounds.Height - 50;
            viewPort1.UIThread(() =>
            {
                viewPort1.Size = new Size(x, y);
            });
    
            this.UIThread(() => {
                Rectangle screenRectangle = RectangleToScreen(this.ClientRectangle);

                int titleHeight = screenRectangle.Top - this.Top;
                this.Size = new Size(maxx, titleHeight + maxh); 
            });
        }
        private void Form1_DragDrop(object sender, DragEventArgs e)
        {
            lock(_PendingFiles_Lock)
            {
                var li = new List<string>();
                foreach(var item in (string[])e.Data.GetData(DataFormats.FileDrop))
                    AddFileOrDirectory(item, li);
                _PendingFiles.Add(li);
                if(_FileSendingThread == null)
                {
                    _FileSendingThread = new System.Threading.Thread(new System.Threading.ThreadStart(SendFilesProc));
                    _FileSendingThread.Start();
                }

            }
        }

        private void AddFileOrDirectory(string searchpath, List<string> filelist)
        {
            if(Directory.Exists(searchpath))
            {
                try
                {
                    var di = new DirectoryInfo(searchpath);
                    filelist.Add(searchpath);
                    foreach(var item in di.GetDirectories())
                    {
                        AddFileOrDirectory(item.FullName, filelist);
                    }
                    foreach(var item in di.GetFiles())
                    {
                        AddFileOrDirectory(item.FullName, filelist);
                    }
                } catch(Exception e)
                {
                    Debug.WriteLine(e.Message);
                }
            } else
            {
                try
                {
                    filelist.Add(searchpath);
                } catch(Exception e)
                {
                    Debug.WriteLine(e.Message);
                }
            }
        }

        private void SendFilesProc()
        {
            var rootpath = "";
            var startedbatch = false;
            while(Running)
            {
                lock(_PendingFiles_Lock)
                {
                    _PendingFiles.RemoveAll(a => a.Count == 0);//make sure there are no empty lists in here that would just be silly
                    if(_PendingFiles.Any(a => a.Count > 0))
                    {
                        if(!startedbatch)
                            rootpath = GetRootPath(_PendingFiles.FirstOrDefault());
                        if(string.IsNullOrEmpty(rootpath))
                        {//something is messed up with this file path.. get it out and move on
                            rootpath = "";
                            startedbatch = false;
                            _PendingFiles.RemoveAt(0);
                            continue;// continue loop
                        }
                        startedbatch = true;
                        var filelist = _PendingFiles.FirstOrDefault();
                        var dt = DateTime.Now;//send for 30 ms, then goto sleep
                        int count = 0;
                        for(var i = 0; i < filelist.Count && (DateTime.Now - dt).TotalMilliseconds < 30; i++)
                        {
                            var tempfile = filelist[i].Remove(0, rootpath.Length);
                            SendFile(_Client, filelist[i], tempfile);
                            count++;
                        }
                        filelist.RemoveRange(0, count);
                        if(!filelist.Any())
                            _PendingFiles.Remove(filelist);//remove since there is nothing left to do
                    } else
                    {
                        rootpath = "";
                        startedbatch = false;
                    }
                }
                System.Threading.Thread.Sleep(30);
            }
        }
        private string GetRootPath(List<string> p)
        {
            var firstpath = p.FirstOrDefault();
            if(string.IsNullOrEmpty(firstpath))
                return "";
            if(Directory.Exists(firstpath))
            {
                try
                {
                    var di = new DirectoryInfo(firstpath);
                    return di.FullName.Substring(0, di.FullName.Length - di.Name.Length);
                } catch(Exception e)
                {
                    Debug.WriteLine(e.Message);
                }
            } else
            {
                try
                {
                    var di = new FileInfo(firstpath);
                    return di.FullName.Substring(0, di.FullName.Length - di.Name.Length);

                } catch(Exception e)
                {
                    Debug.WriteLine(e.Message);
                }
            }
            return "";
        }
        public void Connect(string ip_or_host)
        {
            _Host_Address = ip_or_host;
            Connect(_Client, ip_or_host, "443");
        }
        static int counter = 0;
        static DateTime timer = DateTime.Now;
        public void Draw(IntPtr hdc)
        {
            if((DateTime.Now - timer).TotalMilliseconds > 1000)
            {
                Debug.WriteLine("FPS: " + counter);
                counter = 1;
                timer = DateTime.Now;
            } else
                counter += 1;
            Draw(_Client, hdc);
        }
        void KeyEvent(int VK, bool down)
        {
            KeyEvent(_Client, VK, down);
        }
        private void OnConnect()
        {
            Debug.WriteLine("Onconnect in viewer");
            this.UIThread(() => { this.Text = "Connected to: " + _Host_Address + ":443"; });

            if(OnConnectEvent != null)
                OnConnectEvent();

            StopTrafficTimer();
            _TrafficTimer = new System.Timers.Timer(1000);
            _TrafficTimer.Elapsed += _TrafficTimer_Elapsed;
            _TrafficTimer.Start();
        }
        private void OnDisconnect()
        {
            StopTrafficTimer();
            if(OnDisconnectEvent != null)
                OnDisconnectEvent();
            Debug.WriteLine("OnDisconnect in viewer");
        }
        void MouseEvent(int action, int x, int y, int wheel)
        {
            MouseEvent(_Client, action, x, y, wheel);
        }
        void StopTrafficTimer()
        {
            if(_TrafficTimer != null)
            {
                _TrafficTimer.Stop();
                _TrafficTimer.Dispose();
            }
        }
        void MainViewer_FormClosed(object sender, FormClosedEventArgs e)
        {
            StopTrafficTimer();
            Running = false;
            if(_FileSendingThread != null)
                _FileSendingThread.Join(5000);
            _FileSendingThread = null;
            if(_Client != IntPtr.Zero)
                Destroy_Client(_Client);
            _Client = IntPtr.Zero;
            viewPort1.OnDraw_CB = null;
            Application.RemoveMessageFilter(_InputListener);
        }
        bool ClosedCalled = false;
        void MainViewer_FormClosing(object sender, FormClosingEventArgs e)
        {
            if(!ClosedCalled)
            {
                ClosedCalled = true;
                OnDisconnect();
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            OnDisconnect();
        }

        private void button3_Click(object sender, EventArgs e)
        {
            SendCAD(_Client);
        }
        private static string FormatBytes(long bytes)
        {
            const long scale = 1024;
            string[] orders = new string[] { "TB", "GB", "MB", "KB", "Bytes" };
            var max = (long)Math.Pow(scale, (orders.Length - 1));
            foreach(string order in orders)
            {
                if(bytes > max)
                    return string.Format("{0:##.##} {1}", Decimal.Divide(bytes, max), order);
                max /= scale;
            }
            return "0 Bytes";
        }
    }
}
