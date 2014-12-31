using System;
using System.Collections.Generic;
using System.Configuration;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Web;
using System.Web.Mvc;
using System.Web.Security;

namespace RemoteDesktop_GatewayServer.Controllers
{
    //ALWAYS REQUIRE HTTPS for release builds
#if !DEBUG
    [RequireHttps]
#endif
    public class RAT_SupportController : Controller
    {
        private string GetIP4Address()
        {
            if (Request.UserHostAddress == "::1") return "127.0.0.1";
            string IP4Address = String.Empty;
            foreach (IPAddress IPA in Dns.GetHostAddresses(Request.UserHostAddress))
            {
                if (IPA.AddressFamily.ToString() == "InterNetwork")
                {
                    IP4Address = IPA.ToString();
                    break;
                }
            }

            if (IP4Address != String.Empty) return IP4Address;
            foreach (IPAddress IPA in Dns.GetHostAddresses(Dns.GetHostName()))
            {
                if (IPA.AddressFamily.ToString() == "InterNetwork")
                {
                    IP4Address = IPA.ToString();
                    break;
                }
            }

            return IP4Address;
        }

        public ActionResult Index()
        {
            return View();
        }
        public ActionResult GetID(string computername, string username, string mac, string session)
        {
            Debug.WriteLine("Received GetID request " + computername + "  " + username + "  " + mac + "  " + session);
            var c = RemoteDesktop_GatewayServer.Signalr.ProxyWatcher.ReserveID(GetIP4Address(), computername, username, mac, session);
            if (c == null) return Content("");
            var st = c.Src_ID.ToString() + "\n";
            st += c.AES_Session_Key;
            return Content(st);
        }
        private void SetSettings(RemoteDesktop_GatewayServer.Code.ResourceLayout res)
        {
            res.IDS_STRINGSERVICE_NAME = ConfigurationManager.AppSettings["RAT_ServiceName"];
            res.IDS_STRINGSERVICE_DISPLAY_NAME = ConfigurationManager.AppSettings["RAT_Serive_Display_Name"];
            res.IDS_STRINGDEFAULTPORT = ConfigurationManager.AppSettings["RAT_Gateway_External_Connect_Port"];
            res.IDS_STRINGDEFAULTGATEWAY = ConfigurationManager.AppSettings["RAT_GatewayHostName"];
            res.IDS_STRINGDEFAULTPROXYGETSESSIONURL = ConfigurationManager.AppSettings["RAT_Gateway_GetSessionInfoURL"];
            res.IDS_STRINGDISCLAIMERMESSAGE = ConfigurationManager.AppSettings["RAT_Disclaimer"];
            res.IDS_STRINGUNIQUE_ID = Guid.NewGuid().ToString();
        }
        public ActionResult GetP2P_File()
        {
            string path = ConfigurationManager.AppSettings["RAT_P2P_File"];
            if(string.IsNullOrWhiteSpace(path))
                throw new Exception("Missing RAT P2P File");
            var realpath = "";
            if (path.StartsWith("~")) realpath = HttpContext.Server.MapPath(path);
            else realpath = path;
            var res = RemoteDesktop_GatewayServer.Code.UpdateEXE.LoadSettings(realpath);
            SetSettings(res);
            return File(RemoteDesktop_GatewayServer.Code.UpdateEXE.Update(realpath, res), "application/exe", Path.GetFileName(realpath));
        }
        public ActionResult GetGateway_File()
        {
            string path = ConfigurationManager.AppSettings["RAT_Gateway_File"];
            if(string.IsNullOrWhiteSpace(path))
                throw new Exception("Missing RAT Gateway File");
            var realpath = "";
            if (path.StartsWith("~")) realpath = HttpContext.Server.MapPath(path);
            else realpath = path;
            var res = RemoteDesktop_GatewayServer.Code.UpdateEXE.LoadSettings(realpath);
            SetSettings(res);

            return File(RemoteDesktop_GatewayServer.Code.UpdateEXE.Update(realpath, res), "application/exe", Path.GetFileName(realpath));
        }

        [HttpPost]
        public ActionResult Authenticate(string Username, string Password)
        {
            Debug.WriteLine("Received auth request " + Username + "  " + Password);
            bool result = false;
            //BELOW IS TESTING OF THE SERVICE.. Code should be replaced with real authentication code
            if (!string.IsNullOrWhiteSpace(Username) && !string.IsNullOrWhiteSpace(Password))
            {
                result = (Username == "test") && (Password == "abc123");
            }

            if (result)
            {
                FormsAuthentication.SetAuthCookie(Username, false);
            }
            return new EmptyResult();
        }
    }
}