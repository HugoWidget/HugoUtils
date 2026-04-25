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
#pragma once
#include "WinUtils/WinPch.h"

#include <Windows.h>

#include <tlhelp32.h>
#include <vector>
#include <string>
#include <functional>
#include <optional>
#include <chrono>

#include "WinUtilsDef.h"
#include "AWDef.h"
using FindProcCallback = std::function<bool(const TF(PROCESSENTRY32)&)>;

namespace WinUtils {
	struct LaunchItem {
		string_t program;
		string_t params;
		string_t workDir;
		bool runAsAdmin = false;
		int showWnd = SW_NORMAL;
	};

	// Process Enumeration
	WUAPI bool EnumProcesses(const FindProcCallback& callback);
	WUAPI std::vector<TF(PROCESSENTRY32)> FindAllProcesses(const FindProcCallback& func);
	WUAPI std::optional<TF(PROCESSENTRY32)> FindFirstProcess(const FindProcCallback& func);
	WUAPI bool EnableDebugPrivilege();

	// Process Status & Operations
	WUAPI bool IsProcessRunning(string_view_t processName);
	WUAPI std::vector<DWORD> GetProcessIdsByName(string_view_t processName);
	WUAPI int TerminateProcessesByName(string_view_t processName);
	WUAPI int TerminateMultipleProcesses(const std::vector<string_t>& processNames);

	// Window Operations
	WUAPI bool IsWindowTopMost(HWND hwnd);
	WUAPI bool SetWindowTopMost(HWND hwnd, bool isTopMost);
	WUAPI bool ForceHideWindow(HWND hwnd);
	WUAPI bool IsWindowFullScreen(HWND hWnd, int tolerance = 0, bool relative = false);
	WUAPI bool IsWindowFullScreen(int x, int y, int cx, int cy, int tolerance, bool relative);
	WUAPI RECT GetWindowRect(HWND hWnd);
	WUAPI bool GetWindowTitle(HWND hWnd, char_t* outTitle, int maxLength);
	WUAPI string_t GetWindowTitleString(HWND hWnd);

	// Privilege & Instance Management
	WUAPI bool IsCurrentProcessAdmin();
	WUAPI bool RequireAdminPrivilege(bool exit = true);
	WUAPI void EnsureSingleInstance(string_t title = TS(""), string_t name = TS(""), string_t content = TS(""), string_t extraInfo = TS(""));

	// Path Handling
	string_t ExtractArguments(const string_t& cmdLine);
	WUAPI string_t GetCurrentProcessPath();
	WUAPI string_t GetCurrentProcessDir();
	WUAPI string_t GetCurrentProcessName();
	WUAPI string_t GetFileNameFromPath(string_view_t path);
	WUAPI string_t GetDirFromPath(string_view_t path);
	WUAPI bool IsBareFileName(const string_t& path);
	WUAPI string_t NormalizeAbsolutePath(const string_t& path);
	WUAPI void CleanupPath(string_t& path);
	WUAPI string_t ResolvePath(const string_t& path, const string_t& baseDir = TS(""));

	// Error Handling
	WUAPI string_t GetWindowsErrorMsg(DWORD error_code = GetLastError()) noexcept;

	// Program Execution & Monitoring
	WUAPI HINSTANCE RunExternalProgram(string_t lpFile, string_t lpOperation = TS("open"), string_t lpParameters = TS(""), string_t lpDirectory = TS(""), int showCmd = SW_NORMAL);
	WUAPI HINSTANCE RunExternalProgram(const LaunchItem& item);
	WUAPI int MonitorAndTerminateProcesses(HINSTANCE hInstance, const std::vector<string_t>& targetProcesses, int checkInterval = 1000);
}