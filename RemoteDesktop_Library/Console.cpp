#include "stdafx.h"
#include <conio.h>
#include <FCNTL.H>
#include <io.h>
#include "Console.h"

static BOOL WINAPI MyConsoleCtrlHandler(DWORD dwCtrlEvent) { return dwCtrlEvent == CTRL_C_EVENT; }

RemoteDesktop::CConsole::CConsole() : m_OwnConsole(false) {
	if (!AllocConsole()) return;

	SetConsoleCtrlHandler(MyConsoleCtrlHandler, TRUE);
	RemoveMenu(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE, MF_BYCOMMAND);
	const int in = _open_osfhandle(INT_PTR(GetStdHandle(STD_INPUT_HANDLE)), _O_TEXT);
	const int out = _open_osfhandle(INT_PTR(GetStdHandle(STD_OUTPUT_HANDLE)), _O_TEXT);
	m_OldStdin = *stdin;
	m_OldStdout = *stdout;
	auto sr = *stdout;
	*stdin = *_fdopen(in, "r");
	*stdout = *_fdopen(out, "w");

	m_OwnConsole = true;
}

RemoteDesktop::CConsole::~CConsole() {
	if (m_OwnConsole) {
		fclose(stdout);
		fclose(stdin);
		*stdout = m_OldStdout;
		*stdin = m_OldStdin;
		SetConsoleCtrlHandler(MyConsoleCtrlHandler, FALSE);
		FreeConsole();
	}
}