using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;

namespace RemoteDesktop_Viewer.Code
{
    public class InputListener : System.Windows.Forms.IMessageFilter
    {
        public const int WM_KEYDOWN = 0x100;
        public const int WM_KEYUP = 0x101;

        public const int WM_MOUSEMOVE = 512;
        public const int WM_LBUTTONDOWN = 513;
        public const int WM_LBUTTONUP = 514;
        public const int WM_LBUTTONDBLCLK = 515;
        public const int WM_RBUTTONDOWN = 516;
        public const int WM_RBUTTONUP = 517;
        public const int WM_RBUTTONDBLCLK = 518;
        public const int WM_MBUTTONDOWN = 519;
        public const int WM_MBUTTONUP = 520;
        public const int WM_MBUTTONDBLCLK = 521;
        public const int WM_MOUSEWHEEL = 522;

        public enum WinFormMouseEventFlags : int
        {
            LEFTDOWN = WM_LBUTTONDOWN,
            LEFTUP = WM_LBUTTONUP,
            LEFTDCLICK = WM_LBUTTONDBLCLK,
            MIDDLEDOWN = WM_MBUTTONDOWN,
            MIDDLEUP = WM_MBUTTONUP,
            MIDDLEDCLICK = WM_MBUTTONDBLCLK,
            MOVE = WM_MOUSEMOVE,
            RIGHTDOWN = WM_RBUTTONDOWN,
            RIGHTUP = WM_RBUTTONUP,
            RIGHTDCLICK = WM_RBUTTONDBLCLK,
            WHEEL = WM_MOUSEWHEEL,
            XDOWN = WM_LBUTTONDOWN,
            XUP = WM_LBUTTONUP
        }



        public Action<int, int, int, int> InputMouseEvent;
        public Action<int, bool> InputKeyEvent;

        private DateTime KeyboardSecondCounter = DateTime.Now;
        private DateTime MouseSecondCounter = DateTime.Now;

        private const int InputPerSec = 30;
        private List<int> Keys_Down;
        private int _LastMsg, _LastX, _LastY, _Lastwheel;

        private IntPtr Handle;
        public InputListener(IntPtr handle)
        {
            Handle = handle;
            Keys_Down = new List<int>();
            _LastMsg = _LastX = _LastY = _Lastwheel = 0;
        }

        public bool PreFilterMessage(ref System.Windows.Forms.Message m)
        {
            if(Handle == m.HWnd)
            {
                if(m.Msg == WM_KEYDOWN || m.Msg == WM_KEYUP)
                {
                    if(InputKeyEvent != null)
                    {
                        var temp = unchecked(IntPtr.Size == 8 ? (int)m.WParam.ToInt64() : (int)m.WParam.ToInt32());
                        var kdown = true;
                        if(m.Msg == WM_KEYDOWN)
                        {
                            //Debug.WriteLine("KeyDown");
                            if(Keys_Down.Contains(temp))
                            {
                                if((DateTime.Now - KeyboardSecondCounter).TotalMilliseconds < InputPerSec) return false;
                                else  KeyboardSecondCounter = DateTime.Now;
                            } else Keys_Down.Add(temp);
                            
                            kdown = true;
                        } else
                        {
                            kdown = false;
                            Keys_Down.Remove(temp);//else its an up, so remove it
                        }
                        InputKeyEvent(temp, kdown);
                        return true;
                    }
                }
                if(InputMouseEvent != null && ((int[])Enum.GetValues(typeof(WinFormMouseEventFlags))).Contains(m.Msg))
                {
                    if(m.Msg == WM_MOUSEMOVE)
                    {
                        if((DateTime.Now - MouseSecondCounter).TotalMilliseconds < InputPerSec)
                            return false;
                        else
                            MouseSecondCounter = DateTime.Now;
                    }
                    var p = GetPoint(m.LParam);
                    var wheel = 0;
                    if(m.Msg == WM_MOUSEWHEEL)
                    {
                        uint xy = unchecked(IntPtr.Size == 8 ? (uint)m.WParam.ToInt64() : (uint)m.WParam.ToInt32());
                        wheel = unchecked((short)(xy >> 16));
                    }
                    if(_LastMsg != m.Msg || p.X != _LastX || p.Y != _LastY || _Lastwheel != wheel)
                    {
                        InputMouseEvent(m.Msg, p.X, p.Y, wheel);
                        _LastMsg = m.Msg;
                        _LastX = p.X;
                        _LastY = p.Y;
                        _Lastwheel = wheel;
                        Debug.WriteLine("Sending Mouse event " + m.Msg);
                    }

                }
            }
            return false;
        }
        private int GetInt(IntPtr ptr)
        {
            return IntPtr.Size == 8 ? unchecked((int)ptr.ToInt64()) : ptr.ToInt32();
        }
        private System.Drawing.Point GetPoint(IntPtr _xy)
        {
            uint xy = unchecked(IntPtr.Size == 8 ? (uint)_xy.ToInt64() : (uint)_xy.ToInt32());
            int x = unchecked((short)xy);
            int y = unchecked((short)(xy >> 16));
            return new System.Drawing.Point(x, y);
        }
    }
}
