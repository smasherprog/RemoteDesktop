using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace RemoteDesktop_Viewer.Code
{
    public static class CursorManager
    {
        private static Cursor[] CursorList= new Cursor[] {
            Cursors.Arrow, Cursors.IBeam,  Cursors.WaitCursor, Cursors.Cross,
            Cursors.UpArrow, Cursors.SizeAll,Cursors.Arrow,
            Cursors.SizeNWSE,Cursors.SizeNESW, Cursors.SizeWE, 
            Cursors.SizeNS, Cursors.SizeAll,Cursors.No, Cursors.Hand,Cursors.AppStarting,  
            Cursors.Help};
        private enum Mouse_Types
        {
            ARROW = 32512,
            IBEAM = 32513,
            WAIT = 32514,
            CROSS = 32515,
            UPARROW = 32516,
            SIZE = 32640,
            ICON = 32641,
            SIZENWSE = 32642,
            SIZENESW = 32643,
            SIZEWE = 32644,
            SIZENS = 32645,
            SIZEALL = 32646,
            NO = 32648,
            HAND = 32649,
            APPSTARTING = 32650,
            HELP = 32651
        };
        
        public static Cursor Get_Cursor(int key)
        {
            int index=0;
            Debug.WriteLine("Changing cursor to " + key);
            foreach(int item in Enum.GetValues(typeof(Mouse_Types))){
                if(key == item)
                {
                    Debug.WriteLine("found at index "+ index);
                    return CursorList[index];
                }
                index += 1;     
            }
            return Cursors.Arrow;
        }
    }
}
