using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using RemoteDesktop_Viewer.Code;

namespace RemoteDesktop_Viewer
{
    public partial class ConnectDialog : Form
    {
        private MainViewer _LastMainViewer = null;
        private Connecting _Connecting = null;
        public ConnectDialog()
        {
            InitializeComponent();
            _Connecting = new Connecting();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            _LastMainViewer = new MainViewer();
            _LastMainViewer.Show(this);
            _LastMainViewer.Hide();         
            
            _LastMainViewer.OnConnectEvent += OnConnect;
            _LastMainViewer.OnDisconnectEvent += OnDisconnect;
            _LastMainViewer.Connect(textBox1.Text);
            _Connecting.Show();

            this.Hide();
        }
        private void OnConnect()
        {
            _Connecting.UIThread(()=>{ _Connecting.Hide(); });
            this.UIThread(() => {this.Hide(); });
            _LastMainViewer.UIThread(() =>{ _LastMainViewer.Show(); });
     
        }
        private void OnDisconnect()
        {
            _Connecting.UIThread(() => { _Connecting.Hide(); });
            this.UIThread(() => { this.Show(); });
            _LastMainViewer.UIThread(() => {
                _LastMainViewer.Close();
                _LastMainViewer = null;
            });
            
        }
    }
}
