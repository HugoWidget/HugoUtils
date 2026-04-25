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

#include <ShlObj.h>
#include <format>
#include <thread>
#include <algorithm>
#include <ranges>

#include "WinUtils/WinUtils.h"
#include "WinUtils/Logger.h"
#include "WinUtils/StrConvert.h"
#if WU_HASHLIB
#include "WinUtils/hashlib/md5.h"
#endif
#include <lmcons.h>

using namespace std;
using namespace WinUtils;
constexpr const wchar_t* MONITOR_WND_CLASS = L"WinUtils_ProcessMonitor_Class";
namespace {
	Logger logger(TS("WinUtils"));

	struct MonitorThreadParam {
		HANDLE exitEvent;
		vector<string_t> targetProcesses;
		int checkInterval;
	};

	LRESULT CALLBACK MonitorWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
		switch (uMsg) {
		case WM_DESTROY:
			PostQuitMessage(0);
			return 0;
		default:
			return DefWindowProcW(hwnd, uMsg, wParam, lParam);
		}
	}

	DWORD WINAPI MonitorThread(LPVOID lpParam) {
		auto param = static_cast<MonitorThreadParam*>(lpParam);
		if (!param) return 1;

		while (WaitForSingleObject(param->exitEvent, 0) != WAIT_OBJECT_0) {
			WinUtils::TerminateMultipleProcesses(param->targetProcesses);
			this_thread::sleep_for(chrono::milliseconds(param->checkInterval));
		}

		delete param;
		return 0;
	}
}
namespace WinUtils {
	bool EnumProcesses(const FindProcCallback& callback) {
		HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hSnapshot == INVALID_HANDLE_VALUE) {
			logger.DLog(LogLevel::Error, format(TS("CreateToolhelp32Snapshot fail: {}"), GetWindowsErrorMsg()));
			return false;
		}

		TF(PROCESSENTRY32) entry {};
		entry.dwSize = sizeof(TF(PROCESSENTRY32));

		if (!TF(Process32First)(hSnapshot, &entry)) {
			logger.DLog(LogLevel::Error, format(TS("TF(Process32First) fail: {}"), GetWindowsErrorMsg()));
			CloseHandle(hSnapshot);
			return false;
		}

		do {
			if (!callback(entry)) break;
		} while (TF(Process32Next)(hSnapshot, &entry));

		CloseHandle(hSnapshot);
		return true;
	}

	std::vector<TF(PROCESSENTRY32)> FindAllProcesses(const FindProcCallback& func) {
		vector<TF(PROCESSENTRY32)> result;
		EnumProcesses([&](const TF(PROCESSENTRY32)& entry) {
			if (func(entry)) result.push_back(entry);
			return true;
			});
		return result;
	}

	std::optional<TF(PROCESSENTRY32)> FindFirstProcess(const FindProcCallback& func) {
		optional<TF(PROCESSENTRY32)> result;
		EnumProcesses([&](const TF(PROCESSENTRY32)& entry) {
			if (func(entry)) { result = entry; return false; }
			return true;
			});
		return result;
	}

	bool EnableDebugPrivilege() {
		HANDLE hToken = nullptr;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
			logger.DLog(LogLevel::Error, format(TS("OpenProcessToken fail: {}"), GetWindowsErrorMsg()));
			return false;
		}

		LUID luid{};
		if (!LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid)) {
			logger.DLog(LogLevel::Error, format(TS("LookupPrivilegeValueW fail: {}"), GetWindowsErrorMsg()));
			CloseHandle(hToken);
			return false;
		}

		TOKEN_PRIVILEGES tp{};
		tp.PrivilegeCount = 1;
		tp.Privileges[0].Luid = luid;
		tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

		bool ret = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr);
		if (!ret || GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
			logger.DLog(LogLevel::Error, format(TS("AdjustTokenPrivileges fail: {}"), GetWindowsErrorMsg()));
			CloseHandle(hToken);
			return false;
		}

		CloseHandle(hToken);
		return true;
	}

	bool IsProcessRunning(string_view_t processName) {
		return FindFirstProcess([&](const TF(PROCESSENTRY32)& entry) {
			return entry.szExeFile == (string_t)processName;
			}).has_value();
	}

	std::vector<DWORD> GetProcessIdsByName(string_view_t processName) {
		vector<DWORD> pids;
		EnumProcesses([&](const TF(PROCESSENTRY32)& entry) {
			if (entry.szExeFile == (string_t)processName) pids.push_back(entry.th32ProcessID);
			return true;
			});
		return pids;
	}

	int TerminateProcessesByName(string_view_t processName) {
		int closedCount = 0;
		EnumProcesses([&](const TF(PROCESSENTRY32)& entry) {
			if (entry.szExeFile != (string_t)processName || entry.th32ProcessID == GetCurrentProcessId())
				return true;

			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
			if (!hProcess) {
				logger.DLog(LogLevel::Error, format(TS("OpenProcess fail [{}:{}]: {}"), processName, entry.th32ProcessID, GetWindowsErrorMsg()));
				return true;
			}

			if (TerminateProcess(hProcess, 0)) {
				logger.DLog(LogLevel::Info, format(TS("Terminate success [{}:{}]"), processName, entry.th32ProcessID));
				closedCount++;
			}
			else logger.DLog(LogLevel::Error, format(TS("Terminate fail [{}:{}]: {}"), processName, entry.th32ProcessID, GetWindowsErrorMsg()));
			CloseHandle(hProcess);
			return true;
			});
		return closedCount;
	}

	int TerminateMultipleProcesses(const std::vector<string_t>& processNames) {
		int totalClosed = 0;
		for (const auto& name : processNames) totalClosed += TerminateProcessesByName(name);
		return totalClosed;
	}

	bool IsWindowTopMost(HWND hwnd) {
		if (!IsWindow(hwnd)) return false;
		return (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
	}

	bool SetWindowTopMost(HWND hwnd, bool isTopMost) {
		if (!IsWindow(hwnd)) return false;
		return SetWindowPos(hwnd, isTopMost ? HWND_TOPMOST : HWND_NOTOPMOST,
			0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}

	bool ForceHideWindow(HWND hwnd) {
		if (!IsWindow(hwnd)) return false;
		ShowWindow(hwnd, SW_MINIMIZE);
		SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);
		LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
		SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
		SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
		return true;
	}

	bool IsWindowFullScreen(HWND hWnd, int tolerance, bool relative) {
		if (!IsWindow(hWnd)) return false;

		RECT windowRect{};
		if (!::GetWindowRect(hWnd, &windowRect)) return false;

		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);

		int windowWidth = windowRect.right - windowRect.left;
		int windowHeight = windowRect.bottom - windowRect.top;

		return IsWindowFullScreen(windowRect.left, windowRect.top, windowWidth, windowHeight, tolerance, relative);
	}

	bool IsWindowFullScreen(int x, int y, int cx, int cy, int tolerance, bool relative) {
		tolerance = (std::max)(tolerance, 0);

		RECT windowRect{};
		windowRect.top = y;
		windowRect.left = x;
		windowRect.right = x + cx;
		windowRect.bottom = y + cy;


		int screenWidth = GetSystemMetrics(SM_CXSCREEN);
		int screenHeight = GetSystemMetrics(SM_CYSCREEN);

		if (relative) {
			int windowWidth = windowRect.right - windowRect.left;
			int windowHeight = windowRect.bottom - windowRect.top;
			return abs(windowWidth - screenWidth) <= tolerance && abs(windowHeight - screenHeight) <= tolerance;
		}
		else {
			return abs(windowRect.left) <= tolerance && abs(windowRect.top) <= tolerance
				&& abs(windowRect.right - screenWidth) <= tolerance && abs(windowRect.bottom - screenHeight) <= tolerance;
		}
	}

	RECT GetWindowRect(HWND hWnd) {
		RECT rect{};
		if (IsWindow(hWnd)) ::GetWindowRect(hWnd, &rect);
		return rect;
	}

	bool GetWindowTitle(HWND hWnd, char_t* outTitle, int maxLength) {
		if (!IsWindow(hWnd) || !outTitle || maxLength <= 0) return false;
		int titleLen = GetWindowTextLengthW(hWnd);
		if (titleLen <= 0) { outTitle[0] = TS('\0'); return false; }
		titleLen = (std::min)(titleLen, maxLength - 1);
		TF(GetWindowText)(hWnd, outTitle, titleLen + 1);
		return true;
	}

	string_t GetWindowTitleString(HWND hWnd) {
		if (!IsWindow(hWnd)) return TS("");
		int titleLen = GetWindowTextLengthW(hWnd);
		if (titleLen <= 0) return TS("");
		string_t title(titleLen, TS('\0'));
		TF(GetWindowText)(hWnd, title.data(), titleLen + 1);
		return title;
	}

	bool IsCurrentProcessAdmin() {
		HANDLE hToken = nullptr;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
			CloseHandle(hToken);
			return false;
		}

		TOKEN_ELEVATION elevation{};
		DWORD dwSize = sizeof(TOKEN_ELEVATION);
		bool ret = GetTokenInformation(hToken, TokenElevation, &elevation, dwSize, &dwSize);
		CloseHandle(hToken);
		return ret && elevation.TokenIsElevated != 0;
	}

	//Directly use wide string 
	bool RequireAdminPrivilege(bool exit) {
		if (IsCurrentProcessAdmin()) return true;
		wchar_t path[MAX_PATH] = { 0 };
		GetModuleFileNameW(nullptr, path, _countof(path));
		wstring exePath = path;
		int argc = 0;
		wstring params = ExtractArguments(TF(GetCommandLine)());

		HINSTANCE hResult = ShellExecuteW(nullptr, L"runas", exePath.c_str(),
			params.c_str(), nullptr, SW_SHOWNORMAL);
		if (reinterpret_cast<INT_PTR>(hResult) > 32) {
			if (exit) ExitProcess(0);
			return true;
		}
		return false;
	}

	void EnsureSingleInstance(string_t title, string_t name, string_t content, string_t extraInfo) {
		if (name.empty()) name = GetCurrentProcessName();
		if (title.empty()) title = TS("Prompt - ") + name;
		if (content.empty()) content = TS("The program is already running!\nClick OK to close the existing instance, click Cancel to exit this run.");

		string_t input = TS("WinUtils_SingleInstance_") + name + extraInfo;
		size_t hash = std::hash<string_t>{}(input);
		string_t mutexName = TS("Global\\") + ConvertString(std::to_string(hash));

		HANDLE hMutex = TF(CreateMutex)(nullptr, TRUE, mutexName.c_str());

		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			int ret = TF(MessageBox)(nullptr, content.c_str(), title.c_str(), MB_OKCANCEL | MB_ICONWARNING);
			if (ret == IDOK) TerminateProcessesByName(name);
			ExitProcess(0);
		}
	}


	// Path Handling
	string_t ExtractArguments(const string_t& cmdLine)
	{
		if (cmdLine.empty()) return L"";
		size_t pos = 0;
		if (cmdLine[0] == L'\"')
		{
			pos = cmdLine.find(L'\"', 1);
			if (pos != wstring::npos) ++pos;
			while (pos < cmdLine.size() && cmdLine[pos] == L' ') ++pos;
		}
		else
		{
			pos = cmdLine.find(L' ');
			if (pos == wstring::npos) return L"";
			++pos;
		}
		return (pos < cmdLine.size()) ? cmdLine.substr(pos) : L"";
	}

	string_t GetCurrentUserName() {
		char_t userName[UNLEN + 1] = {};
		DWORD len = UNLEN;
		TF(GetUserName)(userName, &len);
		return userName;
	}

	string_t GetCurrentProcessPath() {
		char_t path[MAX_PATH] = { 0 };
		TF(GetModuleFileName)(nullptr, path, _countof(path));
		return path;
	}

	string_t GetCurrentProcessDir() {
		return GetDirFromPath(GetCurrentProcessPath());
	}

	string_t GetCurrentProcessName() {
		return GetFileNameFromPath(GetCurrentProcessPath());
	}

	string_t GetFileNameFromPath(string_view_t path) {
		size_t pos = path.find_last_of(TS("\\/"));
		return pos == wstring_view::npos ? string_t(path) : string_t(path.substr(pos + 1));
	}

	string_t GetDirFromPath(string_view_t path) {
		size_t pos = path.find_last_of(TS("\\/"));
		return pos == wstring_view::npos ? TS("") : string_t(path.substr(0, pos + 1));
	}
	bool IsBareFileName(const string_t& path) {
		return (path.find_first_of(TS("\\/")) == string_t::npos) &&
			(path.size() < 2 || path[1] != TS(':')); // not absolute path
	}

	// Normalize absolute path: resolve '.' and '..', assume forward slashes '/'
	string_t NormalizeAbsolutePath(const string_t& path) {
		if (path.empty()) return path;

		std::vector<string_t> components;
		size_t start = 0;
		size_t end = path.find_first_of(TS("\\/"));
		bool preserveLeadingSlash = (!path.empty() && path[0] == TS('/'));

		while (true) {
			string_t comp;
			if (end == string_t::npos) {
				comp = path.substr(start);
			}
			else {
				comp = path.substr(start, end - start);
			}

			if (comp == TS(".")) {
				// ignore current directory
			}
			else if (comp == TS("..")) {
				// handle parent directory: cannot go above drive root or empty root (/)
				if (!components.empty()) {
					bool canPop = true;
					// drive root (e.g., "C:") cannot go up
					if (components.size() == 1 && components[0].size() == 2 && components[0][1] == L':')
						canPop = false;
					// empty root (component for "/") cannot go up
					if (components.size() == 1 && components[0].empty())
						canPop = false;
					if (canPop)
						components.pop_back();
				}
			}
			else {
				// non-empty component, or preserve leading empty component (for paths starting with '/')
				if (!comp.empty() || (components.empty() && preserveLeadingSlash))
					components.push_back(comp);
			}

			if (end == string_t::npos) break;
			start = end + 1;
			end = path.find_first_of(TS("\\/"), start);
		}

		// rebuild path
		string_t result;
		for (size_t i = 0; i < components.size(); ++i) {
			if (i > 0) result += TS('/');
			result += components[i];
		}

		// if original path starts with '/' and result is empty, return "/"
		if (preserveLeadingSlash && result.empty())
			result = TS("/");

		return result;
	}

	void CleanupPath(string_t& path)
	{
		replace(path.begin(), path.end(), TS('\\'), TS('/'));
	}

	// Resolve path, return absolute path. If path is a bare filename, return it directly.
	string_t ResolvePath(const string_t& path, const string_t& baseDir) {
		if (IsBareFileName(path))
			return path; // bare filename, let caller search in PATH

		// determine base directory
		string_t effectiveBase = baseDir.empty() ? GetCurrentProcessDir() : baseDir;
		// ensure base directory ends with '/'
		if (!effectiveBase.empty() && !((string_t)TS("/\\")).contains(effectiveBase.back()))
			effectiveBase.push_back(TS('/'));

		// replace backslashes with forward slashes
		string_t normalizedPath = path;
		CleanupPath(normalizedPath);

		// check if absolute path (starts with drive letter or '/')
		bool isAbsolute = false;
		if (normalizedPath.size() >= 2 && normalizedPath[1] == L':')
			isAbsolute = true;                     // e.g., D:/cmd.exe or D:
		else if (!normalizedPath.empty() && normalizedPath[0] == TS('/'))
			isAbsolute = true;                     // e.g., /usr/bin or //server/share

		string_t fullPath;
		if (isAbsolute) fullPath = normalizedPath;
		else fullPath = effectiveBase + normalizedPath;
		// relative path, prepend base directory
		CleanupPath(fullPath);
		// normalize and return
		return NormalizeAbsolutePath(fullPath);
	}

	// Error handling
	string_t GetWindowsErrorMsg(DWORD error_code) noexcept
	{
		char_t* error_buf = nullptr;
		const DWORD buf_len = TF(FormatMessage)(
			FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr,
			error_code,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<char_t*>(&error_buf),
			0,
			nullptr
			);
		string_t error_msg;
		if (buf_len > 0 && error_buf != nullptr)
		{
			error_msg = error_buf;
			LocalFree(error_buf);
		}
		else error_msg = TS("Unknown error");
		return error_msg;
	}

	HINSTANCE RunExternalProgram(string_t lpFile, string_t lpOperation, string_t lpParameters, string_t lpDirectory, int showCmd) {
		if (lpDirectory.empty()) lpDirectory = GetDirFromPath(lpFile);
		return TF(ShellExecute)(nullptr, lpOperation.empty() ? TS("open") : lpOperation.c_str(),
			lpFile.c_str(), lpParameters.c_str(), lpDirectory.c_str(), showCmd);
	}

	HINSTANCE RunExternalProgram(const LaunchItem& item)
	{
		string_t op = item.runAsAdmin ? TS("runas") : TS("open");
		return RunExternalProgram(item.program, op, item.params, TS(""), item.showWnd);
	}

	int MonitorAndTerminateProcesses(HINSTANCE hInstance, const std::vector<string_t>& targetProcesses, int checkInterval) {
		WNDCLASSW wc{};
		wc.lpfnWndProc = MonitorWindowProc;
		wc.hInstance = hInstance;
		wc.lpszClassName = MONITOR_WND_CLASS;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

		if (RegisterClassW(&wc) == 0) {
			logger.DLog(LogLevel::Error, format(TS("RegisterClassW fail: {}"), GetWindowsErrorMsg()));
			return 1;
		}

		HWND hwnd = CreateWindowExW(0, MONITOR_WND_CLASS, L"Process Monitor", 0,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
			nullptr, nullptr, hInstance, nullptr);
		if (!hwnd) {
			UnregisterClassW(MONITOR_WND_CLASS, hInstance);
			return 1;
		}

		HANDLE hExitEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
		if (!hExitEvent) {
			DestroyWindow(hwnd);
			UnregisterClassW(MONITOR_WND_CLASS, hInstance);
			return 1;
		}

		auto param = new MonitorThreadParam{ hExitEvent, targetProcesses, checkInterval };
		HANDLE hMonitorThread = CreateThread(nullptr, 0, MonitorThread, param, 0, nullptr);
		if (!hMonitorThread) {
			delete param;
			CloseHandle(hExitEvent);
			DestroyWindow(hwnd);
			UnregisterClassW(MONITOR_WND_CLASS, hInstance);
			return 1;
		}

		MSG msg{};
		while (GetMessageW(&msg, nullptr, 0, 0)) {
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		SetEvent(hExitEvent);
		WaitForSingleObject(hMonitorThread, INFINITE);
		CloseHandle(hMonitorThread);
		CloseHandle(hExitEvent);
		DestroyWindow(hwnd);
		UnregisterClassW(MONITOR_WND_CLASS, hInstance);

		return 0;
	}
}