Remote Dekstop Viewer, similiar to VNC.<br/>
This is my own creation of a VNC-Like application written in c++. <br/>
<br/>
I originally wrote this in c#, but the performance was pretty bad and unreliable. So, I am rewriting it in c++.
For the c# version you can check my other projects. It is a stable project, but does not deliver the performance that I needed.
<br/>
the difference in performance so far is staggering with the c++ version of the server running about 4-10X as fast and the veiwer even moreso.

TO USE:

Open RemoteDesktopViewer.sln and build the RemoteDesktop_Viewer project. This results in the program you use to View the desktop you want to connect to.

Open RemoteDesktopServer.sln and build the RemoteDesktopServer project. This results in the program you run in order to allow viewers to connect to the machine.

After building, run the server, then run the viewer. In the dialog box, enter 127.0.0.1 and press connect.  Please Note, that when connected to yourself, you cant really do much because the keyboard and mouse will goto the viewer, which will send to the server and move your mouse relative to the primary monitor.

If you really want to test this, get another machine and run the server on it, then use the viewer to connect to that IP address. VMPlayer is free and can get you up and running fast with another machine to test this on.
