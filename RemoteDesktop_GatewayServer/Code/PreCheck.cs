using System;
using System.Collections.Generic;
using System.Configuration;
using System.Linq;
using System.Web;

[assembly: WebActivatorEx.PostApplicationStartMethod(typeof(RemoteDesktop_GatewayServer.Code.PreCheck), "Start")]
namespace RemoteDesktop_GatewayServer.Code
{
    public static class PreCheck
    {
        public static void Start(){
            if(string.IsNullOrWhiteSpace(ConfigurationManager.AppSettings["RAT_Gateway_File"]))
                throw new Exception("Missing web.config AppSettings Key for'RAT_Gateway_File'");
            if(string.IsNullOrWhiteSpace(ConfigurationManager.AppSettings["RAT_P2P_File"]))
                throw new Exception("Missing web.config AppSettings Key for'RAT_P2P_File'");
            if(string.IsNullOrWhiteSpace(ConfigurationManager.AppSettings["RAT_Gateway_Listen_Port"]))
                throw new Exception("Missing web.config AppSettings Key for'RAT_Gateway_Listen_Port'");
            if(string.IsNullOrWhiteSpace(ConfigurationManager.AppSettings["RAT_Gateway_External_Connect_Port"]))
                throw new Exception("Missing web.config AppSettings Key for'RAT_Gateway_External_Connect_Port'");
            if(string.IsNullOrWhiteSpace(ConfigurationManager.AppSettings["RAT_GatewayHostName"]))
                throw new Exception("Missing web.config AppSettings Key for'RAT_GatewayHostName'");
     

        }
    }
}