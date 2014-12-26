using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace RemoteDesktop_CSLibrary
{
    public static class FormatBytes
    {
        public static string Format(long bytes)
        {
            const long scale = 1024;
            string[] orders = new string[] { "TB", "GB", "MB", "KB", "Bytes" };
            var max = (long)Math.Pow(scale, (orders.Length - 1));
            foreach(string order in orders)
            {
                if(bytes > max)
                    return string.Format("{0:##.##} {1}", Decimal.Divide(bytes, max), order);
                max /= scale;
            }
            return "0 Bytes";
        }
    }
}
