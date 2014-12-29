#ifndef RAIIHDESKTOP123_h
#define RAIIHDESKTOP123_h

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <memory>
#include <utility>
#include <type_traits>


namespace RemoteDesktop{

	typedef std::unique_ptr<std::remove_pointer<HANDLE>::type, decltype(&::CloseHandle)> RAIIHANDLE_TYPE;
#define RAIIHANDLE(handle) std::unique_ptr<std::remove_pointer<HANDLE>::type, decltype(&::CloseHandle)>(handle, &::CloseHandle)

	typedef std::unique_ptr<std::remove_pointer<HMODULE>::type, decltype(&::FreeLibrary)> RAIIHMODULE_TYPE;
#define RAIIHMODULE(handle) std::unique_ptr<std::remove_pointer<HMODULE>::type, decltype(&::FreeLibrary)>(handle, &::FreeLibrary)

	typedef std::unique_ptr<std::remove_pointer<HDESK>::type, decltype(&::CloseDesktop)> RAIIHDESKTOP_TYPE;
#define RAIIHDESKTOP(handle) std::unique_ptr<std::remove_pointer<HDESK>::type, decltype(&::CloseDesktop)>(handle, &::CloseDesktop)

	typedef std::unique_ptr<std::remove_pointer<SC_HANDLE>::type, decltype(&::CloseServiceHandle)> RAIISC_HANDLE_TYPE;
#define RAIISC_HANDLE(handle) std::unique_ptr<std::remove_pointer<SC_HANDLE>::type, decltype(&::CloseServiceHandle)>(handle, &::CloseServiceHandle)

	typedef std::unique_ptr<std::remove_pointer<HMENU>::type, decltype(&::DestroyMenu)> RAIIHMENU_TYPE;
#define RAIIHMENU(handle) std::unique_ptr<std::remove_pointer<HMENU>::type, decltype(&::DestroyMenu)>(handle, &::DestroyMenu)

	typedef std::unique_ptr<std::remove_pointer<HICON>::type, decltype(&::DestroyIcon)> RAIIHICON_TYPE;
#define RAIIHICON(handle) std::unique_ptr<std::remove_pointer<HICON>::type, decltype(&::DestroyIcon)>(handle, &::DestroyIcon)
	
	typedef std::unique_ptr<std::remove_pointer<HDC>::type, decltype(&::DeleteDC)> RAIIHDC_TYPE;
#define RAIIHDC(handle) std::unique_ptr<std::remove_pointer<HDC>::type, decltype(&::DeleteDC)>(handle, &::DeleteDC)

	typedef std::unique_ptr<std::remove_pointer<HBITMAP>::type, decltype(&::DeleteObject)> HBITMAP_TYPE;
#define RAIIHBITMAP(handle) std::unique_ptr<std::remove_pointer<HBITMAP>::type, decltype(&::DeleteObject)>(handle, &::DeleteObject)

	
}

#endif