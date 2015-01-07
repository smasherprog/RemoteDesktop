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
        [UnmanagedFunctionPointer(CallingConvention.StdCall)]
        delegate void _OnFileTransferChanged(int bytes_sent_so_far);

        [DllImport(RemoteDesktop_Viewer.Code.Settings.DLL_Name, CharSet = CharSet.Ansi)]
        static extern void SendFile(IntPtr client, string absolute_path, string relative_path, _OnFileTransferChanged onfilechanged);

        private string[] _Files = null;
        private System.Threading.Thread _FileSendingThread = null;
        private List<Tuple<string, long>> _PendingFiles = new List<Tuple<string, long>>();

        private long _TotalBytes_to_Transfer = 0;
        private long _TotalBytes_Transfered = 0;
        private int _TotalCount_to_Transfer = 0;

        private string _CurrentFile = "";
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
                    _FileSendingThread.Join(500);
                    _FileSendingThread = null;
                }
            }
        }
        private IntPtr _Client = IntPtr.Zero;

        public delegate void OnDoneHandler(FileDownload f);
        public event OnDoneHandler OnDoneEvent;



        private _OnFileTransferChanged _OnFileTransferChanged_CallBack;
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
            _OnFileTransferChanged_CallBack = OnFileTransferChanged;
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
        private void OnFileTransferChanged(int bytes_sent)
        {
            _TotalBytes_Transfered += (long)bytes_sent;
            UpdateTransferUI();
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
        private int UpdateCounter = 0;
        void UpdateTransferUI()
        {
            if(UpdateCounter++ > 10)
            {//dont update tpp often.. gui calls are kind of expensive
                label1.UIThread(() =>
                {
                    label1.Text = _CurrentFile;
                    var totalseconds = (long)(DateTime.Now - _Started).TotalSeconds;
                    if(totalseconds <= 0)
                        totalseconds = 1;
                    progressBar1.Value = (int)_TotalBytes_Transfered;
                    label2.Text = RemoteDesktop_CSLibrary.FormatBytes.Format(_TotalBytes_Transfered / totalseconds) + "/s";
                });
                UpdateCounter = 0;
            }
        }
        private void SendFilesProc()
        {
            try
            {

         
            _Running = true;
            _Started = DateTime.Now;
            var rootpath = GetRootPath();
            for(var i = 0; i < _PendingFiles.Count && _Running; i++)
            {
                _CurrentFile = _PendingFiles[i].Item1;
                SendFile(_Client, _PendingFiles[i].Item1, _PendingFiles[i].Item1.Remove(0, rootpath.Length), _OnFileTransferChanged_CallBack);
                _TotalCount_to_Transfer++;
            }
            _PendingFiles.Clear();
            if(OnDoneEvent != null)
                OnDoneEvent(this);
            }
            catch (Exception e)
            {
                Debug.WriteLine(e.Message);
            }
        }


    }
}
