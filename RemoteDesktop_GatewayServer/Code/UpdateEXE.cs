using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Web;

namespace RemoteDesktop_GatewayServer.Code
{
    public class ResourceLayout
    {
        public ResourceLayout(){
            IDS_STRINGSERVICE_NAME = "RemoteDesktop_svc";
            IDS_STRINGSERVICE_DISPLAY_NAME = "RAT Service Name";
            IDS_STRINGDEFAULTPORT = "443";
            IDS_STRINGDEFAULTGATEWAY = "localhost";
            IDS_STRINGDEFAULTPROXYGETSESSIONURL = "http://localhost:3406/RAT_Support/GetID";
            IDS_STRINGDISCLAIMERMESSAGE = "Do you agree to allow a support technician to connection to your computer?";
            IDS_STRINGUNIQUE_ID = "s";
        }

        public string IDS_STRINGSERVICE_NAME {get; set;} 
        public string IDS_STRINGSERVICE_DISPLAY_NAME {get; set;} 
        public string IDS_STRINGDEFAULTPORT {get; set;} 
        public string IDS_STRINGDEFAULTGATEWAY {get; set;}
        public string IDS_STRINGDEFAULTPROXYGETSESSIONURL { get; set; }
        public string IDS_STRINGDISCLAIMERMESSAGE { get; set; }
        public string IDS_STRINGUNIQUE_ID { get; set; } 

    }
    public static class UpdateEXE
    {
        [System.Flags]
        enum LoadLibraryFlags : uint
        {
            DONT_RESOLVE_DLL_REFERENCES = 0x00000001,
            LOAD_IGNORE_CODE_AUTHZ_LEVEL = 0x00000010,
            LOAD_LIBRARY_AS_DATAFILE = 0x00000002,
            LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE = 0x00000040,
            LOAD_LIBRARY_AS_IMAGE_RESOURCE = 0x00000020,
            LOAD_WITH_ALTERED_SEARCH_PATH = 0x00000008
        }

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr BeginUpdateResource(string pFileName, [MarshalAs(UnmanagedType.Bool)]bool bDeleteExistingResources);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool EndUpdateResource(IntPtr hUpdate, bool fDiscard);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool UpdateResource(IntPtr hUpdate, uint lpType, uint lpName, ushort wLanguage, byte[] lpData, uint cbData);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr LoadLibraryEx(string lpFileName, IntPtr hReservedNull, LoadLibraryFlags dwFlags);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern bool FreeLibrary(IntPtr hModule);
        [DllImport("kernel32.dll")]
        static extern IntPtr FindResource(IntPtr hModule, string lpName, string lpType);

        [DllImport("kernel32.dll", SetLastError = true)]
        static extern IntPtr LoadResource(IntPtr hModule, IntPtr hResInfo);
        [DllImport("kernel32.dll")]
        static extern IntPtr LockResource(IntPtr hResData);
        [DllImport("kernel32.dll", SetLastError = true)]
        static extern uint SizeofResource(IntPtr hModule, IntPtr hResInfo);

        private const uint RT_CURSOR = 0x00000001;
        private const uint RT_BITMAP = 0x00000002;
        private const uint RT_ICON = 0x00000003;
        private const uint RT_MENU = 0x00000004;
        private const uint RT_DIALOG = 0x00000005;
        private const uint RT_STRING = 0x00000006;
        private const uint RT_FONTDIR = 0x00000007;
        private const uint RT_FONT = 0x00000008;
        private const uint RT_ACCELERATOR = 0x00000009;
        private const uint RT_RCDATA = 0x0000000a;
        private const uint RT_MESSAGETABLE = 0x0000000b;



        [DllImport("kernel32.dll", EntryPoint = "EnumResourceNamesW", CharSet = CharSet.Unicode, SetLastError = true)]
        static extern bool EnumResourceNamesWithName(
          IntPtr hModule,
          string lpszType,
          EnumResNameDelegate lpEnumFunc,
          IntPtr lParam);

        [DllImport("kernel32.dll", EntryPoint = "EnumResourceNamesW", CharSet = CharSet.Unicode, SetLastError = true)]
        static extern bool EnumResourceNamesWithID(
          IntPtr hModule,
          uint lpszType,
          EnumResNameDelegate lpEnumFunc,
          IntPtr lParam);

        private delegate bool EnumResNameDelegate(
          IntPtr hModule,
          IntPtr lpszType,
          IntPtr lpszName,
          IntPtr lParam);

        private static bool IS_INTRESOURCE(IntPtr value)
        {
            if(((uint)value) > ushort.MaxValue)
                return false;
            return true;
        }
        private static uint GET_RESOURCE_ID(IntPtr value)
        {
            if(IS_INTRESOURCE(value) == true)
                return (uint)value;
            throw new System.NotSupportedException("value is not an ID!");
        }
        private static string GET_RESOURCE_NAME(IntPtr value)
        {
            if(IS_INTRESOURCE(value) == true)
                return value.ToString();
            return Marshal.PtrToStringUni((IntPtr)value);
        }
        static bool EnumRes(IntPtr hModule, IntPtr lpszType, IntPtr lpszName, IntPtr lParam)
        {
            Debug.WriteLine("Type: "+GET_RESOURCE_NAME(lpszType));
            Debug.WriteLine("Name: "+ GET_RESOURCE_NAME(lpszName));
            return true;
        }
        private static void Append(MemoryStream ms, string st)
        {
            ms.Write(BitConverter.GetBytes((ushort)st.Length), 0, 2);
            ms.Write(System.Text.Encoding.Unicode.GetBytes(st), 0, st.Length * 2);
        }
        private static string GetNext(byte[] b, int offset)
        {
            var stringlen = BitConverter.ToInt16(b, offset);
            offset+=2;
            return System.Text.Encoding.Unicode.GetString(b, offset, stringlen*2);
        }
        private static List<string> Split(byte[] ms)
        {
            int offset = 2;//first is always a null char
            var strings = new List<string>();
            while (offset < ms.Length)
            {
                var st = GetNext(ms, offset);
                offset += 2 + (st.Length * 2);//unicode characters take 2 bytes per char plus the size of a ushort
                strings.Add(st);
            }
            return strings;
        }
        public static ResourceLayout LoadSettings(string path)
        {
            var ret =  new ResourceLayout();
            IntPtr ipModule = IntPtr.Zero;
            try
            {
                ipModule = LoadLibraryEx(path, IntPtr.Zero, LoadLibraryFlags.LOAD_LIBRARY_AS_DATAFILE);
                if (ipModule == IntPtr.Zero)
                    return ret;
                var ipResInfo = FindResource(ipModule, "#8", "#6");//Why 8 here? I dont know, it was the only RT_STRING type listed in the exe. 6 is RT_STRING type
                if (ipResInfo == IntPtr.Zero)
                    return ret;

                var hResLoad = LoadResource(ipModule, ipResInfo);
                if (hResLoad == IntPtr.Zero)
                    return ret;
                var lpResLock = LockResource(hResLoad);
                if (lpResLock == IntPtr.Zero)
                    return ret;
                var dwsize = SizeofResource(ipModule, ipResInfo);
                byte[] y = new byte[dwsize];

                Marshal.Copy(lpResLock, y, 0, (int)dwsize);
                if (ipModule != IntPtr.Zero)
                    FreeLibrary(ipModule);
                var splits = Split(y);
                int offset=0;

                ret.IDS_STRINGSERVICE_NAME = splits[offset++];
                ret.IDS_STRINGSERVICE_DISPLAY_NAME = splits[offset++];
                ret.IDS_STRINGDEFAULTPORT = splits[offset++];
                ret.IDS_STRINGDEFAULTGATEWAY = splits[offset++];
                ret.IDS_STRINGDEFAULTPROXYGETSESSIONURL = splits[offset++];

                ret.IDS_STRINGDISCLAIMERMESSAGE = splits[offset++];
                ret.IDS_STRINGUNIQUE_ID = splits[offset++];

            }
            catch (Exception e)
            {
                Debug.WriteLine(e.Message);
            }
            finally
            {
                if (ipModule != IntPtr.Zero) FreeLibrary(ipModule);
            }
            return ret;
        }
        public static byte[] Update(string path, ResourceLayout newsettings)
        {
            //BELOW is work in progress. The string read from a resource is a single block in unicode. The first char is a null char, then each string starts with 1 16 bit int of the length
            //the next is the string, then 16 bit len, then the string, etc..
            IntPtr ipModule = IntPtr.Zero;
            try
            {
                ipModule = LoadLibraryEx(path, IntPtr.Zero, LoadLibraryFlags.LOAD_LIBRARY_AS_DATAFILE);
                if(ipModule == IntPtr.Zero)
                    return new byte[0];
                //Scan resource for RT_STRING types...
                //if(EnumResourceNamesWithID(ipModule, RT_STRING, new EnumResNameDelegate(EnumRes), IntPtr.Zero) == false)
                //{
                //    Debug.WriteLine("gle: {0}", Marshal.GetLastWin32Error());
                //}
                var ipResInfo = FindResource(ipModule, "#8", "#6");//Why 8 here? I dont know, it was the only RT_STRING type listed in the exe. 6 is RT_STRING type
                if(ipResInfo == IntPtr.Zero)
                    return new byte[0];
          
                var hResLoad = LoadResource(ipModule, ipResInfo);
                if(hResLoad == IntPtr.Zero)
                    return new byte[0];
                var lpResLock = LockResource(hResLoad);
                if(lpResLock == IntPtr.Zero)
                    return new byte[0];
                var dwsize = SizeofResource(ipModule, ipResInfo);
                byte[] y = new byte[dwsize];
               
                Marshal.Copy(lpResLock, y, 0, (int)dwsize);
                var size = BitConverter.ToInt16(y, 0);
                var st = System.Text.Encoding.Unicode.GetString(y);
                if(ipModule != IntPtr.Zero)
                    FreeLibrary(ipModule);
                ipModule = IntPtr.Zero;
                var hUpdateRes = BeginUpdateResource(path, false);
                if(hUpdateRes == IntPtr.Zero)
                    return new byte[0];
          
                var ms = new MemoryStream();
                ms.Write(System.Text.Encoding.Unicode.GetBytes("\0"), 0, 2);

                Append(ms, newsettings.IDS_STRINGSERVICE_NAME);
                Append(ms, newsettings.IDS_STRINGSERVICE_DISPLAY_NAME);
                Append(ms, newsettings.IDS_STRINGDEFAULTPORT);
                Append(ms, newsettings.IDS_STRINGDEFAULTGATEWAY);
                Append(ms, newsettings.IDS_STRINGDEFAULTPROXYGETSESSIONURL);
                Append(ms, newsettings.IDS_STRINGDISCLAIMERMESSAGE);
                Append(ms, newsettings.IDS_STRINGUNIQUE_ID);


                var b = ms.ToArray();
                st = System.Text.Encoding.Unicode.GetString(b);

                var ciInfo = new CultureInfo("en-US");
                ushort uLcid = (ushort)ciInfo.LCID;
                if(!UpdateResource(hUpdateRes, 6, 8, uLcid, b, (uint)b.Length))
                    return new byte[0];
                if(!EndUpdateResource(hUpdateRes, false))
                {
                    Debug.WriteLine(Marshal.GetLastWin32Error());
                    
                    return new byte[0];
                }
                    
            } catch(Exception e)
            {
                Debug.WriteLine(e.Message);
            } finally
            {
                if(ipModule != IntPtr.Zero) FreeLibrary(ipModule);
            }
            return File.ReadAllBytes(path);
        }
    }
}