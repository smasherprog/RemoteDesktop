#ifndef CLIENT_H
#define CLIENT_H
#include "BaseClient.h"
#include <mutex>

namespace RemoteDesktop{
#if defined _DEBUG
	class CConsole;
#endif

	class ImageCompression;
	class HBITMAP_wrapper;
	class Client : public BaseClient{
#if defined _DEBUG
		std::unique_ptr<CConsole> _DebugConsole;
#endif
		std::unique_ptr<ImageCompression> _ImageCompression;
		std::unique_ptr<HBITMAP_wrapper> _HBITMAP_wrapper;	
		HWND _HWND;
		std::mutex _DrawLock;
		std::vector<int> _DownKeys;

	public:
		Client(HWND hwnd);
		virtual ~Client() override;	
		virtual void OnDisconnect(SocketHandler& sh) override;
		virtual void OnConnect(SocketHandler& sh) override;
		virtual void OnReceive(SocketHandler& sh)  override;

		virtual void Draw(HDC hdc)  override;
		virtual void KeyEvent(int VK, bool down)  override;
	};

};


#endif