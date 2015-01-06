using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Security;

namespace RemoteDesktop_GatewayExample.Models
{
    public class FormsAuthHandler : IHttpModule
    {
        public FormsAuthHandler()
        {

        }
        public void Init(HttpApplication app)
        {
            app.AuthenticateRequest += app_AuthenticateRequest;
        }

        void app_AuthenticateRequest(object sender, EventArgs e)
        {
            if(FormsAuthentication.CookiesSupported == true)
            {
                var objApp = (HttpApplication) sender ;
                if(objApp.Request.Cookies[FormsAuthentication.FormsCookieName] != null)
                {
                    try
                    {
                        //let us take out the username now                
                        var cookie = FormsAuthentication.Decrypt(objApp.Request.Cookies[FormsAuthentication.FormsCookieName].Value);
                        if(string.IsNullOrWhiteSpace(cookie.Name) || string.IsNullOrWhiteSpace(cookie.UserData))
                            return;//no good
                        string roles = cookie.UserData;
                        HttpContext.Current.User = new System.Security.Principal.GenericPrincipal(
                          new System.Security.Principal.GenericIdentity(cookie.Name, "Forms"), roles.Split(';'));
                    } catch(Exception)
                    {
                    
                    }
                }
            }
        }
        public void Dispose()
        {

        }
    }
}