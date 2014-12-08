using Microsoft.AspNet.SignalR;
using Microsoft.AspNet.SignalR.Hubs;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Security.Claims;
using System.Web;

namespace RemoteDesktop_ProxyServer.Signalr
{
    [AttributeUsage(AttributeTargets.Class, Inherited = false, AllowMultiple = false)]
    public class AuthorizeClaimsAttribute : AuthorizeAttribute
    {
        protected override bool UserAuthorized(System.Security.Principal.IPrincipal user)
        {
            Debug.WriteLine("UserAuthorized");
            if(user == null)
                return false;
            var principal = (ClaimsPrincipal)user;

            if(principal != null)
            {
                Claim authenticated = principal.FindFirst(ClaimTypes.Authentication);
                return authenticated.Value == "true" ? true : false;
            } else
            {
                return false;
            }
        }
        public override bool AuthorizeHubConnection(HubDescriptor hubDescriptor, IRequest request)
        {
            Debug.WriteLine("AuthorizeHubConnection");
            //write custom logic here to allow or deny 
            return true;
        }

        public override bool AuthorizeHubMethodInvocation(IHubIncomingInvokerContext hubIncomingInvokerContext, bool appliesToMethod)
        {
            Debug.WriteLine("AuthorizeHubMethodInvocation");
            return true;
        }
    }
 
}