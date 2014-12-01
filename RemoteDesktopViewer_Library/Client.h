#ifndef CLIENT_H
#define CLIENT_H
#include "..\RemoteDesktop_Library\CommonNetwork.h"

namespace RemoteDesktop{


	class Display;
	class SocketHandler;
	class BaseClient;
	struct Packet_Header;
	class ClipboardMonitor;
	struct Clipboard_Data;

	class Client {

		std::unique_ptr<Display> _Display;
		std::unique_ptr<BaseClient> _NetworkClient;
		std::unique_ptr<ClipboardMonitor> _ClipboardMonitor;

		HWND _HWND;
		void OnDisconnect();
		void OnConnect(std::shared_ptr<SocketHandler>& sh); 
		void _OnClipboardChanged(const Clipboard_Data& c);
		void _Handle_ClipBoard(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);

		void OnReceive(Packet_Header* header, const char* data, std::shared_ptr<SocketHandler>& sh);
		std::wstring _Host, _Port;
		void(__stdcall * _OnConnect)();
		void(__stdcall * _OnDisconnect)();
		void(__stdcall * _OnPrimaryChanged)(int x, int y);

		void SendFileOrFolder(std::string root1, std::string fullpath);

	public:
		Client(HWND hwnd, void(__stdcall * onconnect)(), void(__stdcall * ondisconnect)(), void(__stdcall * oncursorchange)(int), void(__stdcall * onprimchanged)(int, int));
		~Client();	
	
		void Connect(std::wstring host, std::wstring port = L"443");
		void Stop();

		void Draw(HDC hdc);
		void KeyEvent(int VK, bool down) ;
		void MouseEvent(unsigned int action, int x, int y, int wheel=0);

		void SendCAD();
		void SendFile(const char* absolute_path, const char* relative_path);
		Traffic_Stats get_TrafficStats() const;

	};

};


#endif