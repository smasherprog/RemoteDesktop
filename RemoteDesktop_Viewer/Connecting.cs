using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using RemoteDesktop_Viewer.Code;

namespace RemoteDesktop_Viewer
{
    public partial class Connecting : Form
    {
        int counter = 0;
        private int _MaxAttempts = 1;
        public int MaxAttempts
        {
            get { return _MaxAttempts; }
            set
            {
                _MaxAttempts = value;
                progressBar1.UIThread(() =>
                {
                    progressBar1.Maximum = _MaxAttempts;
                    progressBar1.Update();
                });
            }
        }

        private int _ConnectAttempt = 1;
        public int ConnectAttempt
        {
            get { return _ConnectAttempt; }
            set
            {
                _ConnectAttempt = value;
                var s = "Connection Attempt -- " + _ConnectAttempt.ToString() + " of " + _MaxAttempts.ToString() + " --";
                
                label1.UIThread(() => { label1.Text = s; });

                counter++;
                progressBar1.UIThread(() =>
                {
                    progressBar1.Value = _ConnectAttempt;
                    progressBar1.Update();
                });
            }
        }
        public Connecting()
        {
            InitializeComponent();
        }

   
    }
}
