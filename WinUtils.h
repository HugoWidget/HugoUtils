#pragma once
#include <Windows.h>
#include <tlhelp32.h>
#include <vector>
#include <string>
#include <functional>
#include <optional>
#include <chrono>
#include "WinUtilsDef.h"

using FindProcCallback = std::function<bool(const PROCESSENTRY32W&)>;

namespace WinUtils {
	struct LaunchItem {
		string_t program;
		string_t params;
		string_t workDir;
		bool runAsAdmin = false;
		int showWnd = SW_NORMAL;
	};

	// Process Enumeration
	bool EnumProcesses(const FindProcCallback& callback);
	std::vector<PROCESSENTRY32W> FindAllProcesses(const FindProcCallback& func);
	std::optional<PROCESSENTRY32W> FindFirstProcess(const FindProcCallback& func);
	bool EnableDebugPrivilege();

	// Process Status & Operations
	bool IsProcessRunning(string_view_t processName);
	std::vector<DWORD> GetProcessIdsByName(string_view_t processName);
	int TerminateProcessesByName(string_view_t processName);
	int TerminateMultipleProcesses(const std::vector<string_t>& processNames);

	// Window Operations
	bool IsWindowTopMost(HWND hwnd);
	bool SetWindowTopMost(HWND hwnd, bool isTopMost);
	bool ForceHideWindow(HWND hwnd);
	bool IsWindowFullScreen(HWND hWnd, int tolerance = 0, bool relative = false);
	RECT GetWindowRect(HWND hWnd);
	bool GetWindowTitle(HWND hWnd, char_t* outTitle, int maxLength);
	string_t GetWindowTitleString(HWND hWnd);

	// Privilege & Instance Management
	bool IsCurrentProcessAdmin();
	bool RequireAdminPrivilege(bool exit = true);
	void EnsureSingleInstance(string_t title = TS(""), string_t name = TS(""), string_t content = TS(""), string_t extraInfo= TS(""));

	// Path Handling
	string_t GetCurrentProcessPath();
	string_t GetCurrentProcessDir();
	string_t GetCurrentProcessName();
	string_t GetFileNameFromPath(string_view_t path);
	string_t GetDirFromPath(string_view_t path); 
    bool IsBareFileName(const string_t& path);
    string_t NormalizeAbsolutePath(const string_t& path);
	void CleanupPath(string_t& path);
    string_t ResolvePath(const string_t& path, const string_t& baseDir = TS(""));

	// Error Handling
	string_t GetWindowsErrorMsg(DWORD error_code = GetLastError()) noexcept;

	// Program Execution & Monitoring
	HINSTANCE RunExternalProgram(string_t lpFile, string_t lpOperation = TS("open"), string_t lpParameters = TS(""), string_t lpDirectory = TS(""), int showCmd=SW_NORMAL);
	HINSTANCE RunExternalProgram(const LaunchItem& item);
	int MonitorAndTerminateProcesses(HINSTANCE hInstance, const std::vector<string_t>& targetProcesses, int checkInterval = 1000);
}