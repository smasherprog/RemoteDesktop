using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace RemoteDesktop_Viewer
{
    public partial class ConnectDialog : Form
    {
        Action<string> _CB;
        public ConnectDialog(Action<string> cb)
        {
            InitializeComponent();
            _CB = cb;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            _CB(textBox1.Text);
            this.Close();
        }
    }
}
