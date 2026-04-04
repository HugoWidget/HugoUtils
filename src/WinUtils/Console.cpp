/*
 * The MIT License (MIT)
 * Copyright (c) 2026 howdy213
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include "WinUtils/WinPch.h"

#include <iostream>

#include "WinUtils/Console.h"
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
	std::wcin.imbue(std::locale(""));
	std::wcout.imbue(std::locale(""));
	std::wcerr.imbue(std::locale(""));
	/*
	SetConsoleCP(CP_UTF8);
	SetConsoleOutputCP(CP_UTF8);
	setlocale(LC_ALL, "zh-CN.UTF-8");
	*/
}

bool Console::setVisible(bool visible)
{
	HWND consoleWnd = GetConsoleWindow();
	return (bool)SetWindowPos(consoleWnd, 0, 0, 0, 0, 0, (visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW) | SWP_NOMOVE | SWP_NOSIZE);
}
bool Console::setVirtualConsole(bool enable)
{
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut != INVALID_HANDLE_VALUE) {
		DWORD mode = 0;
		if (GetConsoleMode(hOut, &mode)) {
			if (enable)mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			else mode &= !ENABLE_VIRTUAL_TERMINAL_PROCESSING;
			return SetConsoleMode(hOut, mode);
		}
	}
	return false;
}
void Console::redirect()
{
	freopen_s(&stream, "CON", "r", stdin);
	freopen_s(&stream, "CON", "w", stdout);
}
