using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace RemoteDesktop_Viewer
{
    public partial class AdminLogin : Form
    {
        public delegate void OnLoginHandler(string username, string password);

        public event OnLoginHandler OnLoginEvent;
        public AdminLogin()
        {
            InitializeComponent();
            Password.KeyUp += Password_KeyUp;
        }

        void Password_KeyUp(object sender, KeyEventArgs e)
        {
            if(e.KeyCode == Keys.Enter)
            {
                button1.PerformClick();
                e.Handled = true;
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if(OnLoginEvent != null)
            {
                if(string.IsNullOrWhiteSpace(Username.Text) || string.IsNullOrWhiteSpace(Password.Text))
                    return;
                if(Username.Text.Length < 3 || Password.Text.Length < 3)
                    return;
                OnLoginEvent(Username.Text, Password.Text);
            }

        }

    }
}
