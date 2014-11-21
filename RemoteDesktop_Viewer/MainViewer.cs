using RemoteDesktop_Viewer.Code;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;


namespace RemoteDesktop_Viewer
{
    public partial class MainViewer : Form
    {
        InputListener _InputListener = null;
        Client_Wrapper _Client = null;
        public MainViewer()
        {
            InitializeComponent();
            FormClosing += MainViewer_FormClosing;

            _InputListener = new InputListener(viewPort1.Handle);
            _InputListener.InputKeyEvent = KeyEvent;
            _InputListener.InputMouseEvent = MouseEvent;
            Application.AddMessageFilter(_InputListener);
        }
        void KeyEvent(int k, bool b)
        {
            if(_Client != null)
                _Client.KeyEvent(k, b);
        }
        void MouseEvent(int action, int x, int y, int wheel)
        {
            if(_Client != null)
                _Client.MouseEvent(action, x, y, wheel);
        }
        void MainViewer_FormClosing(object sender, FormClosingEventArgs e)
        {
            if(_Client != null)
                _Client.Dispose();
            _Client = null;
        }
        private void Connect(string ip_or_host)
        {
            if(_Client != null)
                _Client.Dispose();
            _Client = new Client_Wrapper(viewPort1);
            _Client.Connect(ip_or_host, "443");
        }
        private void button1_Click(object sender, EventArgs e)
        {
            var c = new ConnectDialog(Connect);
            c.ShowDialog(this);
        }

        private void button2_Click(object sender, EventArgs e)
        {
            if(_Client != null)
                _Client.Dispose();
            _Client = null;
        }

        private void button3_Click(object sender, EventArgs e)
        {
            if(_Client != null)
                _Client.SendCAD();
        }

    }
}
