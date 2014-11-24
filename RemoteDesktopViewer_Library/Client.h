#ifndef CLIENT_H
#define CLIENT_H

namespace RemoteDesktop{

	class ImageCompression;
	class Display;
	class SocketHandler;
	class BaseClient;
	struct Packet_Header;

	class Client {

		std::unique_ptr<ImageCompression> _ImageCompression;
		std::unique_ptr<Display> _Display;
		std::unique_ptr<BaseClient> _NetworkClient;

		HWND _HWND;
		void OnDisconnect();
		void OnConnect(std::shared_ptr<SocketHandler>& sh);
		void OnReceive(Packet_Header* header, const char* data, std::shared_ptr<SocketHandler>& sh);
		std::wstring _Host, _Port;
		void(__stdcall * _OnConnect)();
		void(__stdcall * _OnDisconnect)(); 
		void SendFileOrFolder(std::string root1, std::string fullpath);

	public:
		Client(HWND hwnd, void(__stdcall * onconnect)(), void(__stdcall * ondisconnect)(), void(__stdcall * oncursorchange)(int));
		~Client();	
	
		void Connect(std::wstring host, std::wstring port = L"443");
		void Stop();

		void Draw(HDC hdc);
		void KeyEvent(int VK, bool down) ;
		void MouseEvent(unsigned int action, int x, int y, int wheel=0);

		void SendCAD();
		void SendFile(const char* absolute_path, const char* root_path);


	};

};


#endif