using Microsoft.Owin;
using Owin;

[assembly: OwinStartup(typeof(RemoteDesktop_ProxyServer.Startup))]
namespace RemoteDesktop_ProxyServer
{
    public class Startup
    {
        public void Configuration(IAppBuilder app)
        {
            app.MapSignalR();
            
        }
    }
}