using RemoteDesktop_Viewer.Code;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.Security.Principal;
using System.Windows.Forms;

namespace RemoteDesktop_Viewer
{
    static class Program
    {
        [STAThread]
        static void Main()
        {
            using(var ass = new EmbeddedAssembly())
            {
                Application.EnableVisualStyles();
                Application.SetCompatibleTextRenderingDefault(false);
                Application.Run(new ConnectDialog());
            }
        }
    }
}
