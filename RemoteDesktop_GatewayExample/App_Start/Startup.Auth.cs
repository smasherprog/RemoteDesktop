using System;
using Owin;
using RemoteDesktop_GatewayExample.Models;

namespace RemoteDesktop_GatewayExample
{
    public partial class Startup
    {

        public void ConfigureAuth(IAppBuilder app)
        {
            app.MapSignalR();
        }
    }
}