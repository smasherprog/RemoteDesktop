using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using RemoteDesktop_Viewer.Code;
using System.Diagnostics;
using System.Net;

namespace RemoteDesktop_Viewer
{
    public partial class Login : UserControl
    {
        public delegate void OnLoginSuccessHandler(ProxyAuth _auth);
        public event OnLoginSuccessHandler OnLoginSuccessEvent;
        public Login()
        {
            InitializeComponent();
            textBox2.KeyUp += textBox1_KeyUp;
            var task = System.Threading.Tasks.Task.Factory.StartNew(() => { RemoteDesktop_Viewer.Code.ProxyAuth.PreloadDLLS(); });
        }

        void textBox1_KeyUp(object sender, KeyEventArgs e)
        {
            if(e.KeyCode == Keys.Enter)
            {
                button1.PerformClick();
                e.Handled = true;
            }
                
        }

        private void button1_Click(object sender, EventArgs e)
        {
            label4.Text = "";
            if(string.IsNullOrWhiteSpace(textBox1.Text) && !checkBox1.Checked)
            {
                label4.Text = "You Must enter a Login. ";
            }
            if(string.IsNullOrWhiteSpace(textBox2.Text) && !checkBox1.Checked)
            {
                label4.Text += "You Must enter a Password. ";
            }
            if(string.IsNullOrWhiteSpace(label4.Text))
            {
                var a = new ProxyAuth(textBox1.Text, textBox2.Text, checkBox1.Checked);
                if(a.Authenticated)
                {
                    if(OnLoginSuccessEvent != null)
                        OnLoginSuccessEvent(a);
                } else
                    label4.Text = "Failed to authenticate, try again.";
            }
        }

        void connection_Error(Exception obj)
        {
            Debug.WriteLine(obj.Message);
        }

        private void checkBox1_CheckedChanged(object sender, EventArgs e)
        {
            if(this.checkBox1.Checked)
            {
                textBox1.Text = textBox2.Text = "";
                textBox1.Enabled = textBox2.Enabled = false;
            } else
            {
                textBox1.Enabled = textBox2.Enabled = true;
            }
        }
    }
}
