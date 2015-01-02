using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;

namespace RemoteDesktop_GatewayExample.Controllers
{
    public class HomeController : Controller
    {
        // GET: Home
        public ActionResult Index()
        {
            var testing = new RemoteDesktop_CSLibrary.Client();
            return View();
        }
    }
}