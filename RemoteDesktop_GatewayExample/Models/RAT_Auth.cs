using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;

namespace RemoteDesktop_GatewayExample.Models
{
    public static class RAT_Auth
    {
        public static void Setup()
        {
            RemoteDesktop_GatewayServer.Signalr.AuthorizeClaimsAttribute.OnAuthorizeHubConnectionEvent += AuthorizeClaimsAttribute_OnAuthorizeHubConnectionEvent;
            RemoteDesktop_GatewayServer.Signalr.AuthorizeClaimsAttribute.OnAuthorizeHubMethodInvocationEvent += AuthorizeClaimsAttribute_OnAuthorizeHubMethodInvocationEvent;
           // RemoteDesktop_GatewayServer.Signalr.AuthorizeClaimsAttribute.OnUserAuthorizedEvent += AuthorizeClaimsAttribute_OnUserAuthorizedEvent;
        }

        static bool AuthorizeClaimsAttribute_OnUserAuthorizedEvent(System.Security.Principal.IPrincipal user)
        {
            throw new NotImplementedException();
        }

        static bool AuthorizeClaimsAttribute_OnAuthorizeHubMethodInvocationEvent(Microsoft.AspNet.SignalR.Hubs.IHubIncomingInvokerContext hubIncomingInvokerContext, bool appliesToMethod)
        {
            return false;//
        }

        static bool AuthorizeClaimsAttribute_OnAuthorizeHubConnectionEvent(Microsoft.AspNet.SignalR.Hubs.HubDescriptor hubDescriptor, Microsoft.AspNet.SignalR.IRequest request)
        {
            return request.User.Identity.IsAuthenticated || request.User.IsInRole("Rat_Viewer_Group");
         
        }
    }
}