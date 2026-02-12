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
	// Process Enumeration
	bool EnumProcesses(const FindProcCallback& callback);
	std::vector<PROCESSENTRY32W> FindAllProcesses(const FindProcCallback& func);
	std::optional<PROCESSENTRY32W> FindFirstProcess(const FindProcCallback& func);
	bool EnableDebugPrivilege();

	// Process Status & Operations
	bool IsProcessRunning(std::wstring_view processName);
	std::vector<DWORD> GetProcessIdsByName(std::wstring_view processName);
	int TerminateProcessesByName(std::wstring_view processName);
	int TerminateMultipleProcesses(const std::vector<std::wstring>& processNames);

	// Window Operations
	bool IsWindowTopMost(HWND hwnd);
	bool SetWindowTopMost(HWND hwnd, bool isTopMost);
	bool ForceHideWindow(HWND hwnd);
	bool IsWindowFullScreen(HWND hWnd, int tolerance = 0, bool relative = false);
	RECT GetWindowRect(HWND hWnd);
	bool GetWindowTitle(HWND hWnd, WCHAR* outTitle, int maxLength);
	std::wstring GetWindowTitleString(HWND hWnd);

	// Privilege & Instance Management
	bool IsCurrentProcessAdmin();
	bool RequireAdminPrivilege(bool exit = true);
	void EnsureSingleInstance(std::wstring title = L"", std::wstring name = L"", std::wstring content = L"", std::wstring extraInfo= L"");

	// Path Handling
	std::wstring GetCurrentProcessPath();
	std::wstring GetCurrentProcessDir();
	std::wstring GetCurrentProcessName();
	std::wstring GetFileNameFromPath(std::wstring_view path);
	std::wstring GetDirFromPath(std::wstring_view path);

	// Error Handling
	std::wstring GetWindowsErrorMsg(DWORD error_code = GetLastError()) noexcept;

	// Program Execution & Monitoring
	HINSTANCE RunExternalProgram(std::wstring lpFile, std::wstring lpOperation = L"open", std::wstring lpParameters = L"", std::wstring lpDirectory = L"");
	int MonitorAndTerminateProcesses(HINSTANCE hInstance, const std::vector<std::wstring>& targetProcesses, int checkInterval = 1000);
}