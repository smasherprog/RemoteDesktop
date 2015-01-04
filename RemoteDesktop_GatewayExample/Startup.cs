using Microsoft.Owin;
using Owin;

[assembly: OwinStartupAttribute(typeof(RemoteDesktop_GatewayExample.Startup))]
namespace RemoteDesktop_GatewayExample
{
    public partial class Startup
    {
        public void Configuration(IAppBuilder app)
        {
            ConfigureAuth(app);
        }
    }
}
