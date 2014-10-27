using System;
using System.Collections.Generic;
using System.Configuration.Install;
using System.Linq;
using System.Reflection;
using System.ServiceProcess;
using System.Text;


namespace DesktopSharingStarter
{
    class Program
    {
        static void Main(string[] args)
        {
            var ServicesToRun = new List<ServiceBase>();
            ServicesToRun.Add(new Client_Service.Service1());

            var par = string.Concat(args);

            //if started by a user
            if(Environment.UserInteractive)
            {
#if DEBUG
                //if in debug mode, run the services as a non service application
                WindowsService.Interactive.Run(ServicesToRun.ToArray());
#else
                //otherwise, install the application

                switch(par)
                {
                    case "--install":
                        ManagedInstallerClass.InstallHelper(new string[] { Assembly.GetExecutingAssembly().Location });
                        break;
                    case "--uninstall":
                        ManagedInstallerClass.InstallHelper(new string[] { "/u", Assembly.GetExecutingAssembly().Location });
                        break;
                    default:
                        Console.Error.WriteLine("Insvalid Paramters!");
                        break;
                }
#endif

            } else
            {

                ServiceBase.Run(ServicesToRun.ToArray());
            }

        }
    }
}
