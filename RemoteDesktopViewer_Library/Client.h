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
	class INetwork;

	class Client {

		std::shared_ptr<Display> _Display;
		std::shared_ptr<INetwork> _NetworkClient;
		std::shared_ptr<ClipboardMonitor> _ClipboardMonitor;

		HWND _HWND;

		void OnDisconnect();
		void OnConnect(std::shared_ptr<SocketHandler>& sh); 
		void _OnClipboardChanged(const Clipboard_Data& c);
		void _Handle_ClipBoard(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void _Handle_ResolutionChange(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void _Handle_UpdateRegion(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void _Handle_MouseChanged(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void _Handle_UACBlocked(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void _Handle_ElevateFailed(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);
		void _Handle_ElevateSuccess(Packet_Header* header, const char* data, std::shared_ptr<RemoteDesktop::SocketHandler>& sh);


		void OnReceive(Packet_Header* header, const char* data, std::shared_ptr<SocketHandler>& sh);
		std::wstring _Host, _Port;
		void(__stdcall * _OnConnect)() = nullptr;
		void(__stdcall * _OnDisconnect)() = nullptr;
		void(__stdcall * _OnElevateFailed)() = nullptr;
		void(__stdcall * _OnElevateSuccess)() = nullptr;
		void(__stdcall * _OnDisplaysChanged)(int index, int xoffset, int yoffset, int width, int height) = nullptr;
		void(__stdcall * _OnConnectingAttempt)(int attempt, int maxattempt) = nullptr;

		void SendFileOrFolder(std::string root1, std::string fullpath);
		MouseEvent_Header _LastMouseEvent;
		std::weak_ptr<RemoteDesktop::SocketHandler> Socket;

	public:
		Client(HWND hwnd, void(__stdcall * onconnect)(), void(__stdcall * ondisconnect)(), void(__stdcall * oncursorchange)(int), void(__stdcall * ondisplaychanged)(int, int, int, int, int), void(__stdcall * onconnectingattempt)(int, int));
		~Client();	
	
		void Connect( std::wstring port,std::wstring host, int dst_id, std::wstring aeskey);
		void Stop();

		void Draw(HDC hdc);
		void KeyEvent(int VK, bool down) ;
		void MouseEvent(unsigned int action, int x, int y, int wheel=0);

		void ElevateProcess(wchar_t* username, wchar_t* password);
		void SendRemoveService();
		void SendCAD();
		void SendFile(const char* absolute_path, const char* relative_path, void(__stdcall * onfilechanged)(int));
		void SendSettings(Settings_Header h);

		void SetOnElevateFailed(void(__stdcall * func)()){ _OnElevateFailed = func; }
		void SetOnElevateSuccess(void(__stdcall * func)()){ _OnElevateSuccess = func; }

		Traffic_Stats get_TrafficStats() const;

	};

};


#endif