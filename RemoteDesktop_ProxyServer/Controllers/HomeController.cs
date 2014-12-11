using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Web;
using System.Web.Mvc;
using System.Web.Security;

namespace RemoteDesktop_ProxyServer.Controllers
{
    public class HomeController : Controller
    {
        public ActionResult Index()
        {
            return View();
        }
        //ALWAYS REQUIRE HTTPS for release builds
#if !DEBUG
        [RequireHttps]
#endif
        
        [HttpPost]
        public ActionResult Authenticate(string Username, string Password)
        {
      
            Debug.WriteLine("Received auth request " + Username + "  " + Password);
            bool result = false;
            //BELOW IS TESTING OF THE SERVICE.. Code should be replaced with real authentication code
            if(!string.IsNullOrWhiteSpace(Username) && !string.IsNullOrWhiteSpace(Password)){
                result = (Username == "test") && (Password == "abc123");
            }
        
            if(result)
            {
                FormsAuthentication.SetAuthCookie(Username, false);
            }
            return new EmptyResult();
        }
    }
}