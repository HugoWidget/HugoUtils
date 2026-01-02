#include "Console.h"
using namespace WinUtils;
bool Console::attach()
{
	if (GetStdHandle(STD_INPUT_HANDLE))return true;
	if (AttachConsole(ATTACH_PARENT_PROCESS)) {
		redirect();
		hasConsole = true;
		return true;
	}
	return false;
}

bool Console::alloc()
{
	if (AllocConsole()) {
		redirect();
		hasConsole = true;
		return true;
	}
	return false;
}

bool Console::require()
{
	if (GetStdHandle(STD_INPUT_HANDLE))return true;
	if (!attach())return alloc();
	return false;
}


bool Console::isUnderConsole()
{
	return hasConsole;
}

bool Console::isOwnConsole()
{
	HWND consoleWnd = GetConsoleWindow();
	DWORD dwProcessId;
	GetWindowThreadProcessId(consoleWnd, &dwProcessId);
	return GetCurrentProcessId() == dwProcessId;
}

void Console::setLocale()
{
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	setlocale(LC_ALL, "zh-CN.UTF-8");
}

bool Console::setVisible(bool visible)
{
	HWND consoleWnd = GetConsoleWindow();
	return (bool)SetWindowPos(consoleWnd, 0, 0, 0, 0, 0, (visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) | SWP_NOMOVE | SWP_NOSIZE);
}

void Console::redirect()
{
	freopen_s(&stream, "CON", "r", stdin);
	freopen_s(&stream, "CON", "w", stdout);
}
