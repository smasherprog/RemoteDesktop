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
    public partial class ImageSettings : Form
    {
        public delegate void OnSettingsChangedHandler(int quality, bool grayascale);

        public event OnSettingsChangedHandler OnSettingsChangedEvent;
        static private bool Grayscale = false;
        static private int Quality = 75;

        public ImageSettings()
        {
            InitializeComponent();
            checkBox1.CheckedChanged += checkBox1_CheckedChanged;
            trackBar1.ValueChanged += trackBar1_ValueChanged;
            checkBox1.Checked = Grayscale;
            Quality = trackBar1.Value;
        }

        void trackBar1_ValueChanged(object sender, EventArgs e)
        {
            Grayscale = checkBox1.Checked;
            Quality = trackBar1.Value;
            if(OnSettingsChangedEvent != null)
                OnSettingsChangedEvent(trackBar1.Value, checkBox1.Checked);
        }

        void checkBox1_CheckedChanged(object sender, EventArgs e)
        {
            Grayscale = checkBox1.Checked;
            Quality = trackBar1.Value;
            if(OnSettingsChangedEvent != null)
                OnSettingsChangedEvent(trackBar1.Value, checkBox1.Checked);
        }
   
    }
}
