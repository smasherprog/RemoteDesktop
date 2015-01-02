using System;
using Owin;
using Microsoft.Owin;
[assembly: OwinStartup(typeof(RemoteDesktop_GatewayExample.Startup))]
namespace RemoteDesktop_GatewayExample
{
    public class Startup
    {
        public void Configuration(IAppBuilder app)
        {
            app.MapSignalR();
        }
    }
}