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

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void _OnConnect();

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void _OnDisconnect();

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void _OnCursorChanged(int c_type);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void _OnDisplayChanged(int index, int xoffset, int yoffset, int width, int height);

        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void _OnConnectingAttempt(int attempt, int maxattempts);

        [DllImport(Settings.DLL_Name, CallingConvention = CallingConvention.StdCall)]
        static extern IntPtr Create_Client(IntPtr hwnd, _OnConnect onconnect, _OnDisconnect ondisconnect, _OnCursorChanged oncursorchange, _OnDisplayChanged ondisplaychanged, _OnConnectingAttempt onconnectingattempt);
        [DllImport(Settings.DLL_Name)]
        static extern void Destroy_Client(IntPtr client);
        [DllImport(Settings.DLL_Name, CharSet = CharSet.Unicode)]
        static extern void Connect(IntPtr client, string ip_or_host, string port, int id, string aeskey);
        [DllImport(Settings.DLL_Name)]
        static extern void Draw(IntPtr client, IntPtr hdc);
        [DllImport(Settings.DLL_Name)]
        static extern void KeyEvent(IntPtr client, int VK, bool down);
        [DllImport(Settings.DLL_Name)]
        static extern void MouseEvent(IntPtr client, int action, int x, int y, int wheel);
        [DllImport(Settings.DLL_Name)]
        static extern void SendCAD(IntPtr client);
        [DllImport(Settings.DLL_Name)]
        static extern void SendRemoveService(IntPtr client);
        [DllImport(Settings.DLL_Name, CharSet = CharSet.Unicode)]
        static extern void ElevateProcess(IntPtr client, string username, string password);
        
        [DllImport(Settings.DLL_Name)]
        static extern RemoteDesktop_Viewer.Code.Settings.Traffic_Stats get_TrafficStats(IntPtr client);

        [DllImport(Settings.DLL_Name)]
        static extern void SendSettings(IntPtr client, int img_quality, bool gray, bool shareclip);

        public delegate void OnConnectHandler();
        public delegate void OnDisconnectHandler();
        public delegate void OnCursorChangedHandler(int c_type);
        public delegate void OnConnectingAttemptHandler(int attempt, int maxattempts);

        public event OnConnectHandler OnConnectEvent;
        public event OnDisconnectHandler OnDisconnectEvent;
        public event OnCursorChangedHandler OnCursorChangedEvent;
        public event OnConnectingAttemptHandler OnConnectingAttemptEvent;

        private _OnConnect OnConnect_CallBack;
        private _OnConnectingAttempt OnConnectingAttempt_CallBack;
        private _OnDisconnect OnDisconnect_CallBack;
        private _OnCursorChanged OnCursorChanged_CallBack;
        private _OnDisplayChanged OnDisplayChanged_CallBack;

        private IntPtr _Client = IntPtr.Zero;

        private System.Timers.Timer _TrafficTimer;
        private string _Host_Address;

        private RemoteDesktop_CSLibrary.Client _Proxyd_Client = null;

        public string Host_Address { get { return _Host_Address; } }
        InputListener _InputListener = null;
        private List<FileDownload> _FileDownloadControls = new List<FileDownload>();
        private Rectangle[] _Displays = new Rectangle[4];

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
            OnConnectingAttempt_CallBack = OnConnectingAttempt;

            _Client = Create_Client(viewPort1.Handle, OnConnect_CallBack, OnDisconnect_CallBack, OnCursorChanged_CallBack, OnDisplayChanged_CallBack, OnConnectingAttempt_CallBack);


            button3.MouseEnter += button_MouseEnter;
            button3.MouseLeave += button_MouseLeave;
            button1.MouseEnter += button_MouseEnter;
            button1.MouseLeave += button_MouseLeave;

            button2.MouseEnter += button_MouseEnter;
            button2.MouseLeave += button_MouseLeave;

            button4.MouseEnter += button_MouseEnter;
            button4.MouseLeave += button_MouseLeave;

            for(var i = 0; i < _Displays.Length; i++)
                _Displays[i] = new Rectangle(0, 0, 0, 0);
        }

        void button_MouseLeave(object sender, EventArgs e)
        {
            var but = (Button)sender;
            but.Location = new Point(but.Location.X, -(but.Size.Height / 3 + but.Size.Height / 3));
        }

        void button_MouseEnter(object sender, EventArgs e)
        {
            var but = (Button)sender;
            but.Location = new Point(but.Location.X, 0);
        }


        void _TrafficTimer_Elapsed(object sender, System.Timers.ElapsedEventArgs e)
        {
            var traffic = get_TrafficStats(_Client);

            this.UIThread(() =>
            {
                if(_Proxyd_Client != null)
                {
                    this.Text = "Connected to Proxy: " + _Host_Address + ":443 --> " + _Proxyd_Client.ComputerName + ":" + _Proxyd_Client.UserName + " Out: " + RemoteDesktop_CSLibrary.FormatBytes.Format(traffic.CompressedSendBPS) + "/s In: " + RemoteDesktop_CSLibrary.FormatBytes.Format(traffic.CompressedRecvBPS) + "/s";
                } else
                {
                    this.Text = "Connected to: " + _Host_Address + ":443,  Out: " + RemoteDesktop_CSLibrary.FormatBytes.Format(traffic.CompressedSendBPS) + "/s In: " + RemoteDesktop_CSLibrary.FormatBytes.Format(traffic.CompressedRecvBPS) + "/s";
                }
            });
        }


        private void Form1_DragEnter(object sender, DragEventArgs e)
        {
            // If the data is a file or a bitmap, display the copy cursor. 
            if(e.Data.GetDataPresent(DataFormats.FileDrop))
                e.Effect = DragDropEffects.Copy;
            else
                e.Effect = DragDropEffects.None;

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
        private void OnConnectingAttempt(int attempt, int maxattempts)
        {
            if(OnConnectingAttemptEvent != null)
                OnConnectingAttemptEvent(attempt, maxattempts);
        }
        private void OnDisplayChanged(int index, int xoffset, int yoffset, int width, int height)
        {
            int BorderWidth = (Width - ClientSize.Width) / 2;
            int TitlebarHeight = Height - ClientSize.Height - 2 * BorderWidth;
            int maxwidth = Screen.PrimaryScreen.Bounds.Width - 40;
            int maxheight = Screen.PrimaryScreen.Bounds.Height - 50;
            int viewportwidth = 0;
            int viewportheight = 0;
            _Displays[index] = new Rectangle(xoffset, yoffset, width, height);
            for(var i = 0; i < _Displays.Length; i++)
            {
                viewportheight = Math.Max(viewportheight, _Displays[i].Height + _Displays[i].Top);
                viewportwidth += _Displays[i].Width + _Displays[i].Left;
            }
            this.UIThread(() =>
            {
                viewPort1.Size = new Size(viewportwidth, viewportheight);
                this.Size = new Size(Math.Min(maxwidth, viewportwidth), Math.Min(maxheight, viewportheight));

            });
        }
        private void Form1_DragDrop(object sender, DragEventArgs e)
        {
            var f = new FileDownload((string[])e.Data.GetData(DataFormats.FileDrop), _Client);
            f.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            f.Left = this.Size.Width - f.Size.Width - 25;
            f.Top = 5;
            f.OnDoneEvent += f_OnDoneEvent;
            f.Show();
            Controls.Add(f);
            f.BringToFront();
            viewPort1.SendToBack();
            _FileDownloadControls.Add(f);
        }

        void f_OnDoneEvent(FileDownload f)
        {
            f.UIThread(() =>
            {
                f.Hide();
                Controls.Remove(f);
                _FileDownloadControls.Remove(f);
            });
        }

        public void Connect(string proxy_host, RemoteDesktop_CSLibrary.Client c)
        {
            for(var i = 0; i < _Displays.Length; i++)
                _Displays[i] = new Rectangle(0, 0, 0, 0);

            _Host_Address = proxy_host;
            _Proxyd_Client = c;
            if(c == null)
                Connect(_Client, proxy_host, Settings.Port, -1, "");
            else
                Connect(_Client, proxy_host, Settings.Port, c.Src_ID, c.AES_Session_Key);
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
        private static DateTime MouseThrottle = DateTime.Now;
        //when transferring files, limit mouse messages they cause severe congestion!
        void MouseEvent(int action, int x, int y, int wheel)
        {
            if((action == InputListener.WM_MOUSEWHEEL || action == InputListener.WM_MOUSEMOVE) && _FileDownloadControls.Any())
            {//limit mouse move messages to 10 per second
                if((DateTime.Now - MouseThrottle).Milliseconds > 100)
                {
                    MouseEvent(_Client, action, x, y, wheel);
                    MouseThrottle = DateTime.Now;
                }
            } else
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

            foreach(var item in _FileDownloadControls)
                item.Running = false;
            _FileDownloadControls.Clear();
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

        private void button3_Click(object sender, EventArgs e)
        {
            SendCAD(_Client);
        }


        private void button1_Click(object sender, EventArgs e)
        {
            var result = MessageBox.Show("Remove Service?", "Are you sure that you want to remove the service from the target machine? This will completely remove the service deleting all associated files. ", MessageBoxButtons.OKCancel);
            if(result == System.Windows.Forms.DialogResult.OK)
            {
                SendRemoveService(_Client);
            }
        }

        private void button2_Click(object sender, EventArgs e)
        {
            var f = new SettingsDialog();
            f.OnSettingsChangedEvent += OnSettingsChanged;
            f.ShowDialog(this);
        }
        private void OnSettingsChanged(RemoteDesktop_Viewer.Code.Settings.Settings_Header h)
        {
            SendSettings(_Client, h.Image_Quality, h.GrayScale, h.ShareClip);
        }

        private void button4_Click(object sender, EventArgs e)
        {
            var f = new AdminLogin();
            f.OnLoginEvent += f_OnLoginEvent;
            FormClosing += (a, c) => { f.Close(); };
            f.ShowDialog(this);
        }

        void f_OnLoginEvent(string username, string password)
        {
            ElevateProcess(_Client, username, password);
        }
    }
}
