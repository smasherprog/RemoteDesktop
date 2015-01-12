#ifndef NEWCONNECTDIALOG_H
#define NEWCONNECTDIALOG_H
#include <thread>
#include <functional>
#include "..\RemoteDesktop_Library\Delegate.h"

namespace RemoteDesktop{
	class NewConnect_Dialog{
		std::thread _DialogThread;

		HWND _Hwnd = nullptr;
		HFONT _HFont = nullptr;

		void _Run();
		static LRESULT CALLBACK DlgProc(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam);//so the function has access to private members
		void _UpdateText();
		std::wstring _Name;

	public:
		NewConnect_Dialog();
		~NewConnect_Dialog();

		bool IsOpen() const { return _Hwnd != nullptr; }
		void Show();

		void Show(std::wstring name);
		void Close();
		Delegate<void, std::wstring> OnAllow;
		Delegate<void, std::wstring> OnDeny;
	};
}

#endif