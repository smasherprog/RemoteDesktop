using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;

namespace RemoteDesktop_GatewayServer.Code
{

    public class SignalrAuthIPrincipal : System.Security.Principal.IPrincipal
    {
        public System.Security.Principal.IIdentity Identity { get; private set; }
        private List<string> _Roles { get; set; }
        public SignalrAuthIPrincipal(string username, List<string> roles)
        {
            this.Identity = new System.Security.Principal.GenericIdentity(username);
       
            _Roles = roles;
        }

        public bool IsInRole(string role)
        {
            return Identity != null && Identity.IsAuthenticated &&
               !string.IsNullOrWhiteSpace(role) && _Roles.Any(a => a.ToLower() == role.ToLower());
        }
    }
}