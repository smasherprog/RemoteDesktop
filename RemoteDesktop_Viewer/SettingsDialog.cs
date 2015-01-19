using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace RemoteDesktop_Viewer
{
    public partial class SettingsDialog : Form
    {
      
        public delegate void OnSettingsChangedHandler(RemoteDesktop_CSLibrary.Settings_Header h);

        public event OnSettingsChangedHandler OnSettingsChangedEvent;

        static RemoteDesktop_CSLibrary.Settings_Header Settings = new RemoteDesktop_CSLibrary.Settings_Header
        {
            GrayScale = false,
            Image_Quality = 75,
            ShareClip = true
        };

        public SettingsDialog()
        {
            InitializeComponent();
  
            checkBox1.Checked = Settings.GrayScale;
            trackBar1.Value = Settings.Image_Quality;
            checkBox2.Checked = Settings.ShareClip;         
            checkBox1.CheckedChanged += checkBox1_CheckedChanged;
            checkBox2.CheckedChanged += checkBox1_CheckedChanged;
            trackBar1.ValueChanged += trackBar1_ValueChanged;
 
        }

        void trackBar1_ValueChanged(object sender, EventArgs e)
        {
            Settings.Image_Quality = trackBar1.Value;
            Settings.GrayScale = checkBox1.Checked;
            Settings.ShareClip = checkBox2.Checked;

            if (OnSettingsChangedEvent != null)
                OnSettingsChangedEvent(Settings);
        }

        void checkBox1_CheckedChanged(object sender, EventArgs e)
        {
            Settings.Image_Quality = trackBar1.Value;
            Settings.GrayScale = checkBox1.Checked;
            Settings.ShareClip = checkBox2.Checked;

            if (OnSettingsChangedEvent != null)
                OnSettingsChangedEvent(Settings);
        }

    }
}
