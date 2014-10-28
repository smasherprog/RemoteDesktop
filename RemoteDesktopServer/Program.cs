using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;

namespace RemoteDesktopServer
{
    class Program
    {
        static void Main(string[] args)
        {
            using(var s = new Server())
            {
                s.Listen();
                Debug.WriteLine("Running Server");
                var k = Console.ReadKey();
            }

        }
    }
}
