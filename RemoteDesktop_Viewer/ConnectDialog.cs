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
        private ProxyClients _ProxyClients = null;
        private Login _Login = null;

        public ConnectDialog()
        {
            InitializeComponent();
            _Connecting = new Connecting();
            textBox1.KeyUp += textBox1_KeyUp;
            tabControl1.SelectedIndexChanged += tabControl1_SelectedIndexChanged;
            this.FormClosed += ConnectDialog_FormClosed;
            _ProxyClients = new ProxyClients();

            _ProxyClients.Dock = System.Windows.Forms.DockStyle.Fill;
            _ProxyClients.Location = new System.Drawing.Point(3, 3);
            _ProxyClients.Margin = new System.Windows.Forms.Padding(0);
            _ProxyClients.Name = "_ProxyClients";
            _ProxyClients.Size = new System.Drawing.Size(295, 34);
            _ProxyClients.TabIndex = 0;
            _ProxyClients.OnConnectAttemptEvent += _ProxyClients_OnConnectAttemptEvent;
            _ProxyClients.OnDisconnectEvent += _ProxyClients_OnDisconnectEvent;
            _Login = new Login();

            _Login.Dock = System.Windows.Forms.DockStyle.Fill;
            _Login.Location = new System.Drawing.Point(3, 3);
            _Login.Margin = new System.Windows.Forms.Padding(0);
            _Login.Name = "_Login";
            _Login.Size = new System.Drawing.Size(295, 34);
            _Login.TabIndex = 0;
            _Login.OnLoginSuccessEvent += _Login_OnLoginSuccessEvent;

            this.Shown += ConnectDialog_Shown;
        }

        void ConnectDialog_Shown(object sender, EventArgs e)
        {
            this.Activate();
        }

  

        void _ProxyClients_OnConnectAttemptEvent(string ip_or_host, RemoteDesktop_CSLibrary.Client c)
        {
            Connect(ip_or_host, c);
        }



        void ConnectDialog_FormClosed(object sender, FormClosedEventArgs e)
        {
            if(_ProxyClients != null)
                _ProxyClients.Dispose();
            if(_Login != null)
                _Login.Dispose();
        }

        void textBox1_KeyUp(object sender, KeyEventArgs e)
        {
            if(e.KeyCode == Keys.Enter)
                button1.PerformClick();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if(string.IsNullOrWhiteSpace(textBox1.Text))
                return;
            if(textBox1.Text.Length < 4)
                return;
       
            Connect(textBox1.Text, null);
        }
        private void Connect(string iporhost, RemoteDesktop_CSLibrary.Client c)
        {
            _LastMainViewer = new MainViewer();
            _LastMainViewer.Show(this);
            _LastMainViewer.Hide();

            _LastMainViewer.OnConnectEvent += OnConnect;
            _LastMainViewer.OnDisconnectEvent += OnDisconnect;
            _LastMainViewer.OnConnectingAttemptEvent += _LastMainViewer_OnConnectingAttemptEvent;
            _LastMainViewer.Connect(iporhost, c);
            _Connecting.Show();
            _Connecting.Hide();
            _Connecting.FormClosing += _Connecting_FormClosing;
            this.Hide();
        }

        void _Connecting_FormClosing(object sender, FormClosingEventArgs e)
        {
            e.Cancel = true;
            OnDisconnect();
        }
        private void _LastMainViewer_OnConnectingAttemptEvent(int attempt, int maxattempts)
        {
            Debug.WriteLine("Connecting " + attempt + "  " + maxattempts);
            _Connecting.MaxAttempts = maxattempts;
            _Connecting.ConnectAttempt = attempt;

            _Connecting.UIThread(() => { _Connecting.Show(); });
            this.UIThread(() => { this.Hide(); });
            _LastMainViewer.UIThread(() => { _LastMainViewer.Hide(); });

        }
        private void OnConnect()
        {
            _Connecting.UIThread(() => { _Connecting.Hide(); });
            this.UIThread(() => { this.Hide(); });
            _LastMainViewer.UIThread(() => { _LastMainViewer.Show(); });

        }
        private void OnDisconnect()
        {
            _Connecting.UIThread(() => { _Connecting.Hide(); });
            this.UIThread(() => { this.Show(); });
            _LastMainViewer.UIThread(() =>
            {
                _LastMainViewer.Close();
                _LastMainViewer = null;
            });

        }
        void _ProxyClients_OnDisconnectEvent()
        {
            tabPage2.UIThread(() =>
            {
            tabPage2.Controls.Clear();
            tabPage2.Controls.Add(_Login);
            this.Size = new Size(600, 220);
            });

        }
        void _Login_OnLoginSuccessEvent(ProxyAuth auth)
        {
            tabPage2.Controls.Clear();
            tabPage2.Controls.Add(_ProxyClients);
            _ProxyClients.ProxyAuth = auth;

        }
        void tabControl1_SelectedIndexChanged(object sender, System.EventArgs e)
        {
            var selected = (sender as TabControl).SelectedIndex;
            if(selected == 0)
            {
                this.Size = new Size(325, 100);
            } else
            {
                this.Size = new Size(600, 220);
                if(_ProxyClients.ProxyAuth == null)
                {
                    tabPage2.Controls.Clear();
                    tabPage2.Controls.Add(_Login);
                }
            }
        }

    }
}
