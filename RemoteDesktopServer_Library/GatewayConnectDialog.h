#ifndef GATEWAYCONNECTDIALOG_H
#define GATEWAYCONNECTDIALOG_H
#include <thread>

namespace RemoteDesktop{
	class GatewayConnect_Dialog{
		std::thread _DialogThread;
	
		HWND _Hwnd = nullptr;
		HFONT _HFont = nullptr;

		void _Run();
		static LRESULT CALLBACK DlgProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam);//so the function has access to private members
		void _UpdateText();
			int _ID;

	public:
		GatewayConnect_Dialog();
		~GatewayConnect_Dialog();
		
		bool IsOpen() const {return _Hwnd != nullptr;}
		void Show();
		void Show(int id);
		void Close();
	};
}

#endif