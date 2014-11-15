#ifndef CLIENT_H
#define CLIENT_H
#include "BaseClient.h"

namespace RemoteDesktop{
#if defined _DEBUG
	class CConsole;
#endif

	class ImageCompression;
	class Display;

	class Client : public BaseClient{
#if defined _DEBUG
		std::unique_ptr<CConsole> _DebugConsole;
#endif
		std::unique_ptr<ImageCompression> _ImageCompression;
		std::unique_ptr<Display> _Display;

		std::vector<int> _DownKeys;
		HWND _HWND;

	public:
		Client(HWND hwnd);
		virtual ~ Client() override;	
		virtual void OnDisconnect(SocketHandler& sh) override;
		virtual void OnConnect(SocketHandler& sh) override;
		virtual void OnReceive(SocketHandler& sh)  override;

		virtual void Draw(HDC hdc) override;
		virtual void KeyEvent(int VK, bool down)  override;
		virtual void MouseEvent(unsigned int action, int x, int y, int wheel=0)  override;
		virtual bool SetCursor()  override;

		void SendCAD();
	};

};


#endif