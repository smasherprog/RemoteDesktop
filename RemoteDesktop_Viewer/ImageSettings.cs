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

        public ImageSettings()
        {
            InitializeComponent();
            checkBox1.CheckedChanged += checkBox1_CheckedChanged;
            trackBar1.ValueChanged += trackBar1_ValueChanged;
        }

        void trackBar1_ValueChanged(object sender, EventArgs e)
        {
            if(OnSettingsChangedEvent != null)
                OnSettingsChangedEvent(trackBar1.Value, checkBox1.Checked);
        }

        void checkBox1_CheckedChanged(object sender, EventArgs e)
        {
            if(OnSettingsChangedEvent != null)
                OnSettingsChangedEvent(trackBar1.Value, checkBox1.Checked);
        }
   
    }
}
