using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;
using System.Web.Security;

namespace RemoteDesktop_GatewayExample.Controllers
{
#if !DEBUG
    [RequireHttps]
#endif
    public class SupportController : RemoteDesktop_GatewayServer.Controllers.RAT_SupportBaseController
    {

        public new ActionResult GetID(string computername, string username, string mac, string session)
        {
            return base.GetID(computername, username, mac, session);
        }

        public new ActionResult GetP2P_File()
        {
            return base.GetP2P_File();
        }
        public new ActionResult GetGateway_File()
        {
            return base.GetGateway_File();

        }

        public new ActionResult Authenticate(string Username, string Password)
        {
            //BOGUS TEST BELOW>>> CHANGE TO DO REAL AUTH
            bool result = !string.IsNullOrWhiteSpace(Username) && !string.IsNullOrWhiteSpace(Password);

            if(result)
            {
                FormsAuthenticationTicket ticket = new FormsAuthenticationTicket(
                 1,
                 Username,
                 DateTime.Now,
                 DateTime.Now.AddHours(5),//expiration date
                 false,
                 "Rat_Viewer_Group",
                 FormsAuthentication.FormsCookiePath);
                string encTicket = FormsAuthentication.Encrypt(ticket);

                // Create the cookie.
                Response.Cookies.Add(new HttpCookie(FormsAuthentication.FormsCookieName, encTicket));
                //add cookie to response
            }
            return new EmptyResult();
        }
    }
}