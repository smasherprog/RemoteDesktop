using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace RemoteDesktop_Viewer.Code
{
    public static class ControlExtensions
    {
        public static void UIThread(this Control c, Action code)
        {
            if(c.InvokeRequired)
            {
                c.BeginInvoke(code);
            } else
            {
                code.Invoke();
            }
        }
    }
}
