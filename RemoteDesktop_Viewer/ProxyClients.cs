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

namespace RemoteDesktop_Viewer
{
    public partial class ProxyClients : UserControl
    {
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
                        _Hub = new Microsoft.AspNet.SignalR.Client.HubConnection("http://localhost:1466/");
                        _ProxyHub = _Hub.CreateHubProxy("ProxyHub");
                        _ProxyHub.On<List<Client>>("AvailableClients", ReceivedClients);

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
        private void ReceivedClients(List<Client> clients)
        {
            listView1.UIThread(() =>
            {
                listView1.Items.Clear();//clear all items
                foreach(var item in clients)
                {
                    listView1.Items.Add(new ListViewItem(new string[] { item.Name, item.IP, item.ConnectTime.ToShortTimeString(), item.Status }));
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
                var name = listView1.SelectedItems[0].Text.ToLower();
                Debug.WriteLine(name);

            }

        }
    }
}
