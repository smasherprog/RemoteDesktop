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
    public partial class Connecting : Form
    {
        int counter = 0;
        public Connecting()
        {
            InitializeComponent();
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if(counter>4) counter =0;
            var s = "Connecting ";
            for(var i=0; i< counter;i++)s += " .";
            label1.Text = s;
            counter++;
            progressBar1.Value = counter;
            progressBar1.Update();
        }
    }
}
