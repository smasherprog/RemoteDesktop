<h3>Note:</h3>
<p>I no longer support this library as it was written for windows only. There are however great libraries available which allow cross platform development that I am embracing. Please help with the new project for a better VNC-like application designed to run on Windows, Mac and Linux.</p>
<h3>Goto https://github.com/smasherprog/Remote_Access_Library</h3>
<br/><br/>
Remote Dekstop Viewer, similiar to VNC. (so far VISTA Through Win10 are supported) <br/>
This is my own creation of a VNC-Like application written in c++. <br/>
<br/>
I originally wrote this in c#, but the performance was pretty bad and unreliable. So, I am rewriting it in c++.
For the c# version you can check my other projects. It is a stable project, but does not deliver the performance that I needed.
<br/>
the difference in performance so far is staggering with the c++ version of the server running about 4-10X as fast and the veiwer even moreso.
<br/>
There are three projects:<br/>
Viewer --Used to connect to a Server and view desktop, transfer files, control the mouse, etc<br/>
Server --Ran on a machine that you want to connect to. <br/>
Gateway Server --this is used to bridge connections between Viewer and Server when one or both are behind firewalls and cannot directly connect to each other.<br/>
<br/>
Before Building, you must unzip the jpegturbo libraries. I prebuilt debug/release for 32 and 64 bit Static /MT options. The .lib files are in libjpeg-turbo\prebuiltlibs_movetolibfolder.zip.  Unzip the 86 and 64 folders into the ROOT Folder\lib\       So, when building projects the libraries will be linked in properly. <br/>
<br/>
After this, you are ready to go!<br/>
<br/>
To use the Viewer, just open the solution, ensure the RemoteDesktop_Viewer project is set as startup project and build. THe output will be a program that you can use to connect to other computers, but you need to build the server first!<br/>
<br/>
To build the server, open the server solution, set the RemoteDesktop_ServerP2P as startup project and build. There are two EXE's that are build in the server solution: one is used for the Gateway Server, the P2P one is used when the Gateway server is not needed. <br/>

TO USE:

After building, run the server, then run the viewer. In the dialog box, enter 127.0.0.1 and press connect.  Please Note, that when connected to yourself, you cant really do much because the keyboard and mouse will goto the viewer, which will send to the server so control is a bit strange. 

NOTE: IF YOU RUN THE SERVER, IT WILL INSTALL ITSELF AS A SERVICE IF IT HAS SUFFICENT PRIVILEGES!!! This is by design...

If you really want to test this, get another machine and run the server on it, then use the viewer to connect to that IP address. VMPlayer is free and can get you up and running fast with another machine to test this on.
