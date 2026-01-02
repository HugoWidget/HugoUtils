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
	// 进程遍历
	bool EnumProcesses(const FindProcCallback& callback);
	std::vector<PROCESSENTRY32W> FindAllProcesses(const FindProcCallback& func);
	std::optional<PROCESSENTRY32W> FindFirstProcess(const FindProcCallback& func);
	bool EnableDebugPrivilege();

	// 进程状态/操作
	bool IsProcessRunning(std::wstring_view processName);
	std::vector<DWORD> GetProcessIdsByName(std::wstring_view processName);
	int TerminateProcessesByName(std::wstring_view processName);
	int TerminateMultipleProcesses(const std::vector<std::wstring>& processNames);

	// 窗口操作
	bool IsWindowTopMost(HWND hwnd);
	bool SetWindowTopMost(HWND hwnd, bool isTopMost);
	bool ForceHideWindow(HWND hwnd);
	bool IsWindowFullScreen(HWND hWnd, int tolerance = 0, bool relative = false);
	RECT GetWindowRect(HWND hWnd);
	bool GetWindowTitle(HWND hWnd, WCHAR* outTitle, int maxLength);
	std::wstring GetWindowTitleString(HWND hWnd);

	// 权限/实例
	bool IsCurrentProcessAdmin();
	bool RequireAdminPrivilege(bool exit = true);
	void EnsureSingleInstance(std::wstring title = L"", std::wstring name = L"", std::wstring content = L"");

	// 路径处理
	std::wstring GetCurrentProcessPath();
	std::wstring GetCurrentProcessDir();
	std::wstring GetCurrentProcessName();
	std::wstring GetFileNameFromPath(std::wstring_view path);
	std::wstring GetDirFromPath(std::wstring_view path);

	// 错误处理
	std::wstring GetWindowsErrorMsg(DWORD error_code = GetLastError()) noexcept;

	// 程序运行/监控
	HINSTANCE RunExternalProgram(std::wstring lpFile, std::wstring lpOperation = L"open", std::wstring lpParameters = L"", std::wstring lpDirectory = L"");
	int MonitorAndTerminateProcesses(HINSTANCE hInstance, const std::vector<std::wstring>& targetProcesses, int checkInterval = 1000);
}