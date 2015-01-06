using Microsoft.AspNet.SignalR;
using Microsoft.AspNet.SignalR.Hubs;
using RemoteDesktop_GatewayServer.Code;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Security.Claims;
using System.Security.Principal;
using System.Web;

namespace RemoteDesktop_GatewayServer.Signalr
{
    [AttributeUsage(AttributeTargets.Class, Inherited = false, AllowMultiple = false)]
    public class AuthorizeClaimsAttribute : AuthorizeAttribute
    {
        public delegate bool OnUserAuthorizedHandler(System.Security.Principal.IPrincipal user);
        public delegate bool OnAuthorizeHubConnectionHandler(HubDescriptor hubDescriptor, IRequest request);
        public delegate bool OnAuthorizeHubMethodInvocationHandler(IHubIncomingInvokerContext hubIncomingInvokerContext, bool appliesToMethod);

        public static event OnUserAuthorizedHandler OnUserAuthorizedEvent;
        public static event OnAuthorizeHubConnectionHandler OnAuthorizeHubConnectionEvent;
        public static event OnAuthorizeHubMethodInvocationHandler OnAuthorizeHubMethodInvocationEvent;

        protected override bool UserAuthorized(System.Security.Principal.IPrincipal user)
        {
            if (OnUserAuthorizedEvent != null) return OnUserAuthorizedEvent(user);

            Debug.WriteLine("UserAuthorized");
            if (user == null)
                return false;
            var principal = (ClaimsPrincipal)user;

            if (principal != null)
            {
                Claim authenticated = principal.FindFirst(ClaimTypes.Authentication);
                return authenticated.Value == "true" ? true : false;
            }
            else
            {
                return false;
            }
        }
        public override bool AuthorizeHubConnection(HubDescriptor hubDescriptor, IRequest request)
        {
            if (OnAuthorizeHubConnectionEvent != null) return OnAuthorizeHubConnectionEvent(hubDescriptor, request);
            return request.User.Identity.IsAuthenticated;
        }

        public override bool AuthorizeHubMethodInvocation(IHubIncomingInvokerContext hubIncomingInvokerContext, bool appliesToMethod)
        {
            if (OnAuthorizeHubMethodInvocationEvent != null) return OnAuthorizeHubMethodInvocationEvent(hubIncomingInvokerContext, appliesToMethod);
            return false;
        }
    }

}