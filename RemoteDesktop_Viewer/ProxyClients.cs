using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using Microsoft.AspNet.SignalR.Client;
using System.Net;
using RemoteDesktop_Viewer.Code;
using RemoteDesktop_CSLibrary;

namespace RemoteDesktop_Viewer
{
    public partial class ProxyClients : UserControl
    {
        public delegate void OnConnectAttemptHandler(string ip_or_host, int id);
        public event OnConnectAttemptHandler OnConnectAttemptEvent;

        public ProxyAuth _ProxyAuth = null;
        public ProxyAuth ProxyAuth
        {
            get { return _ProxyAuth; }
            set
            {
                _ProxyAuth = value;
                if(value != null)
                    StartService();
                else
                    StopService();

            }
        }
        private Microsoft.AspNet.SignalR.Client.HubConnection _Hub = null;
        private IHubProxy _ProxyHub = null;

        public ProxyClients()
        {
            InitializeComponent();
            listView1.AutoResizeColumns(ColumnHeaderAutoResizeStyle.HeaderSize);
            this.HandleDestroyed += ProxyClients_HandleDestroyed;

        }

        void ProxyClients_HandleDestroyed(object sender, EventArgs e)
        {
            StopService();
        }

        private void StartService()
        {
            try
            {

                if(_ProxyAuth != null)
                {
                    if(_ProxyAuth.Authenticated)
                    {
                        _Hub = new Microsoft.AspNet.SignalR.Client.HubConnection(Settings.URIScheme + Settings.ProxyServer);
                        _ProxyHub = _Hub.CreateHubProxy(Settings.SignalRHubName);
                        _ProxyHub.On<List<Client_Pair>>("AvailableClients", ReceivedClients);

                        if(ProxyAuth.UsingWindowsAuth)
                        {
                            _Hub.Credentials = System.Net.CredentialCache.DefaultNetworkCredentials;
                        } else
                        {
                            _Hub.CookieContainer = new CookieContainer();
                            _Hub.CookieContainer.Add(_ProxyAuth.AuthCookie);
                        }
                        _Hub.Start().Wait();
                    }
                }
            } catch(Exception e)
            {
                Debug.WriteLine(e.Message);
            }
        }
        private void ReceivedClients(List<Client_Pair> clients)
        {
            listView1.UIThread(() =>
            {
                listView1.Items.Clear();//clear all items
                foreach(var item in clients)
                {
                    var left = new List<string>() { "*", "*", "*", "<------>" };
                    var right = new List<string>() { "*", "*", "*" };
                    if(item.Pair == null)
                        continue;
                    var first = item.Pair.FirstOrDefault();
                    if(first != null)
                    {
                        left.Clear();
                        left.Add(first.ID.ToString());
                        left.Add(first.IP);
                        left.Add(first.ConnectTime.ToShortTimeString());
                        left.Add("<------>");
                    }
                    var sec = (item.Pair.Count == 2 ? item.Pair.LastOrDefault() : null);
                    if(sec != null)
                    {
                        right.Clear();
                        right.Add(sec.ID.ToString());
                        right.Add(sec.IP);
                        right.Add(sec.ConnectTime.ToShortTimeString());
                    }
                    left.AddRange(right);
                    listView1.Items.Add(new ListViewItem(left.ToArray()));
                }
                if(clients.Any())
                    listView1.AutoResizeColumns(ColumnHeaderAutoResizeStyle.ColumnContent);
                else
                    listView1.AutoResizeColumns(ColumnHeaderAutoResizeStyle.HeaderSize);

            });

        }
        private void StopService()
        {
            if(_Hub != null)
                _Hub.Dispose();
            _ProxyAuth = null;
        }
        private void toolStripMenuItem1_Click(object sender, EventArgs e)
        {
            if(listView1.SelectedItems.Count > 0)
            {
                var id = listView1.SelectedItems[0].Text;
                int idout = 0;
                if(Int32.TryParse(id, out idout))
                {
                    Debug.WriteLine("Attempting connect to " + Settings.ProxyServer + " id: " + idout);
                    if(OnConnectAttemptEvent != null)
                    {
                        var splits = Settings.ProxyServer.Split(':');
                        OnConnectAttemptEvent(splits.FirstOrDefault(), idout);
                    }

                }
            }

        }
    }
}
