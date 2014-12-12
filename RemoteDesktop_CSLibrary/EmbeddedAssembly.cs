using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Threading.Tasks;

namespace RemoteDesktop_Viewer.Code
{

    public class EmbeddedAssembly : IDisposable
    {
        List<string> _Files = new List<string>();
        public void Load(string embeddedResource, string fileName)
        {
            byte[] ba = null;
            using(Stream stm =  Assembly.GetEntryAssembly().GetManifestResourceStream(embeddedResource))
            {
                if(stm == null)
                    throw new Exception(embeddedResource + " is not found in Embedded Resources.");
                var f = Path.GetDirectoryName(System.Reflection.Assembly.GetEntryAssembly().Location) + "\\" + fileName.Split('.')[1] + ".dll";
                ba = new byte[(int)stm.Length];
                stm.Read(ba, 0, (int)stm.Length);
                File.WriteAllBytes(f, ba);
                _Files.Add(f);
            }
        }
        public void Dispose()
        {
            foreach(var item in _Files)
            {
                try
                {
                    File.Delete(item);
                } catch(Exception e)
                {
                    Debug.WriteLine(e.Message);
                }
            }
        }
    }
}
