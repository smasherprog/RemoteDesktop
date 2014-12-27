using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.IO;
using System.Diagnostics;
using System.Runtime.InteropServices;
using RemoteDesktop_Viewer.Code;

namespace RemoteDesktop_Viewer
{
    public partial class FileDownload : UserControl
    {
        [DllImport(RemoteDesktop_Viewer.Code.Settings.DLL_Name, CharSet = CharSet.Ansi)]
        static extern void SendFile(IntPtr client, string absolute_path, string relative_path);

        private string[] _Files = null;
        private System.Threading.Thread _FileSendingThread = null;
        private List<Tuple<string, long>> _PendingFiles = new List<Tuple<string, long>>();

        private long _TotalBytes_to_Transfer = 0;
        private long _TotalBytes_Transfered = 0;

        private int _TotalCount_to_Transfer = 0;
        private int _TotalCount_Transfered = 0;

        private DateTime _Started = DateTime.Now;

        private bool _Running = false;
        public bool Running
        {
            get { return _Running; }
            set
            {
                _Running = value;
                if(!_Running && _FileSendingThread != null)
                {
                    _FileSendingThread.Join(100);
                    _FileSendingThread = null;
                }
            }
        }
        private IntPtr _Client = IntPtr.Zero;

        public delegate void OnDoneHandler(FileDownload f);
        public event OnDoneHandler OnDoneEvent;
        public FileDownload()
        {
            InitializeComponent();
        }

        public FileDownload(string[] files, IntPtr client)
        {
            _Client = client;
            InitializeComponent();
            _Files = files;
            foreach(var item in files)
                AddFileOrDirectory(item);
            _TotalCount_to_Transfer = _PendingFiles.Count;
            progressBar1.Maximum = (int)_TotalBytes_to_Transfer;
            progressBar1.Value = 0;
            label3.Text = _TotalCount_to_Transfer.ToString() + " items to transfer totaling " + RemoteDesktop_CSLibrary.FormatBytes.Format(_TotalBytes_to_Transfer);
            Running = false;
            _FileSendingThread = new System.Threading.Thread(new System.Threading.ThreadStart(SendFilesProc));
            _FileSendingThread.Start();
        }
        private void AddFileOrDirectory(string searchpath)
        {
            if(Directory.Exists(searchpath))
            {
                try
                {
                    var di = new DirectoryInfo(searchpath);
                    _PendingFiles.Add(new Tuple<string, long>(searchpath, 0));
                    foreach(var item in di.GetDirectories())
                    {
                        AddFileOrDirectory(item.FullName);
                    }
                    foreach(var item in di.GetFiles())
                    {
                        _TotalBytes_to_Transfer += item.Length;
                        _PendingFiles.Add(new Tuple<string, long>(item.FullName, item.Length));
                    }
                } catch(Exception e)
                {
                    Debug.WriteLine(e.Message);
                }
            } else
            {
                try
                {
                    var item = new FileInfo(searchpath);
                    _TotalBytes_to_Transfer += item.Length;
                    _PendingFiles.Add(new Tuple<string, long>(searchpath, item.Length));
                } catch(Exception e)
                {
                    Debug.WriteLine(e.Message);
                }
            }
        }
        private string GetRootPath()
        {
            var firstpath = _PendingFiles.FirstOrDefault();
            if(string.IsNullOrEmpty(firstpath.Item1))
                return "";
            if(Directory.Exists(firstpath.Item1))
            {
                try
                {
                    var di = new DirectoryInfo(firstpath.Item1);
                    return di.FullName.Substring(0, di.FullName.Length - di.Name.Length);
                } catch(Exception e)
                {
                    Debug.WriteLine(e.Message);
                }
            } else
            {
                try
                {
                    var di = new FileInfo(firstpath.Item1);
                    return di.FullName.Substring(0, di.FullName.Length - di.Name.Length);

                } catch(Exception e)
                {
                    Debug.WriteLine(e.Message);
                }
            }
            return "";
        }
        private void SendFilesProc()
        {
            _Running = true;
            _Started = DateTime.Now;
            var rootpath = GetRootPath();
            while(_Running && _PendingFiles.Any())
            {
                var dt = DateTime.Now;//send for 30 ms, then goto sleep
                int count = 0;

                for(var i = 0; i < _PendingFiles.Count && (DateTime.Now - dt).TotalMilliseconds < 30; i++)
                {
                    SendFile(_Client, _PendingFiles[i].Item1, _PendingFiles[i].Item1.Remove(0, rootpath.Length));
                    _TotalBytes_Transfered += _PendingFiles[i].Item2;
                    _TotalCount_to_Transfer++;
                    count++;
                }

                var pendfile = _PendingFiles.FirstOrDefault().Item1;
                label1.UIThread(() =>
                {

                    label1.Text = pendfile;
                    var totalseconds = (long)(DateTime.Now - _Started).TotalSeconds;
                    if(totalseconds <= 0)
                        totalseconds = 1;
                    progressBar1.Value = (int)_TotalBytes_Transfered;
                    label2.Text = RemoteDesktop_CSLibrary.FormatBytes.Format(_TotalBytes_Transfered / totalseconds);
                });

                _PendingFiles.RemoveRange(0, count);

                System.Threading.Thread.Sleep(30);
            }
            if(OnDoneEvent != null)
                OnDoneEvent(this);
        }


    }
}
