using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

namespace RemoteDesktop_Viewer.Code
{
    public class ViewPort : Panel
    {

        public Action<IntPtr> OnDraw_CB;

        public ViewPort()
        {

            this.SetStyle(ControlStyles.Selectable, true);
            // MouseEnter += ViewPort_MouseEnter;
            // MouseLeave += ViewPort_MouseLeave;
            SetStyle(ControlStyles.AllPaintingInWmPaint, true);
            SetStyle(ControlStyles.ContainerControl, false);
            SetStyle(ControlStyles.OptimizedDoubleBuffer, true);
            SetStyle(ControlStyles.Opaque, true);
           
          
        }

        void ViewPort_MouseLeave(object sender, EventArgs e)
        {
            Cursor.Show();
        }

        void ViewPort_MouseEnter(object sender, EventArgs e)
        {
            Cursor.Hide();
        }
        protected override void OnClick(EventArgs e)
        {
            this.Focus();
            base.OnClick(e);
        }
        protected override void OnPaintBackground(PaintEventArgs pevent)
        {

        }

        protected override void OnPaint(PaintEventArgs e)
        {
            if(OnDraw_CB != null)
            {
                e.Graphics.InterpolationMode = System.Drawing.Drawing2D.InterpolationMode.Low;
                e.Graphics.PixelOffsetMode = System.Drawing.Drawing2D.PixelOffsetMode.Default;
                e.Graphics.SmoothingMode = System.Drawing.Drawing2D.SmoothingMode.None;
                OnDraw_CB(e.Graphics.GetHdc());
                e.Graphics.ReleaseHdc();
            } else
            {
                using(var drawFont = new System.Drawing.Font("Arial", 16))
                using(var drawBrush = new System.Drawing.SolidBrush(System.Drawing.Color.Black))
                {
                    var s = this.Size;
                    s.Height /= 2;
                    s.Width /= 2;
                    s.Width -= 40;
                    e.Graphics.Clear(System.Drawing.Color.CornflowerBlue);
                    e.Graphics.DrawString("Waiting to connect . . ", drawFont, drawBrush, s.Width, s.Height);
                }
            }
        }
    }
}
