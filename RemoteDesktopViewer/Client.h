#ifndef CLIENT_H
#define CLIENT_H

namespace RemoteDesktop{
#if defined _DEBUG
	class CConsole;
#endif

	class ImageCompression;
	class Display;
	class SocketHandler;
	class BaseClient;
	struct Packet_Header;

	class Client {
#if defined _DEBUG
		std::unique_ptr<CConsole> _DebugConsole;
#endif
		std::unique_ptr<ImageCompression> _ImageCompression;
		std::unique_ptr<Display> _Display;
		std::unique_ptr<BaseClient> _NetworkClient;

		std::vector<int> _DownKeys;
		HWND _HWND;
		void OnDisconnect(std::shared_ptr<SocketHandler>& sh);
		void OnConnect(std::shared_ptr<SocketHandler>& sh);
		void OnReceive(Packet_Header* header, const char* data, std::shared_ptr<SocketHandler>& sh);
		std::wstring _Host, _Port;

	public:
		Client(HWND hwnd);
		~Client();	
	
		void Connect(std::wstring host, std::wstring port = L"443");
		void Stop();

		void Draw(HDC hdc);
		void KeyEvent(int VK, bool down) ;
		void MouseEvent(unsigned int action, int x, int y, int wheel=0);
		bool SetCursor();
		void SendCAD();
		void UpdateWindowTitle();
	};

};


#endif