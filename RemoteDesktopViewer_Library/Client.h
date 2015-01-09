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

		std::shared_ptr<Display> _Display;
		std::shared_ptr<BaseClient> _NetworkClient;
		std::shared_ptr<ClipboardMonitor> _ClipboardMonitor;

		HWND _HWND;

		void OnDisconnect();
		void OnConnect(std::shared_ptr<SocketHandler>& sh); 
		void _OnClipboardChanged(const Clipboard_Data& c);
		void _Handle_ClipBoard(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);

		void OnReceive(Packet_Header* header, const char* data, std::shared_ptr<SocketHandler>& sh);
		std::wstring _Host, _Port;
		void(__stdcall * _OnConnect)();
		void(__stdcall * _OnDisconnect)();
		void(__stdcall * _OnDisplaysChanged)(int index, int xoffset, int yoffset, int width, int height);
		void(__stdcall * _OnConnectingAttempt)(int attempt, int maxattempt);

		void SendFileOrFolder(std::string root1, std::string fullpath);
		MouseEvent_Header _LastMouseEvent;

	public:
		Client(HWND hwnd, void(__stdcall * onconnect)(), void(__stdcall * ondisconnect)(), void(__stdcall * oncursorchange)(int), void(__stdcall * ondisplaychanged)(int, int, int, int, int), void(__stdcall * onconnectingattempt)(int, int));
		~Client();	
	
		void Connect(std::wstring host, std::wstring port, int dst_id, std::wstring aeskey);
		void Stop();

		void Draw(HDC hdc);
		void KeyEvent(int VK, bool down) ;
		void MouseEvent(unsigned int action, int x, int y, int wheel=0);

		void ElevateProcess(wchar_t* username, wchar_t* password);
		void SendRemoveService();
		void SendCAD();
		void SendFile(const char* absolute_path, const char* relative_path, void(__stdcall * onfilechanged)(int));
		void SendSettings(Settings_Header h);

		Traffic_Stats get_TrafficStats() const;

	};

};


#endif