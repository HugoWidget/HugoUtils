#include "WinUtils.h"
#include "Logger.h"
#include <ShlObj.h>
#include <format>
#include <thread>
using namespace std;
constexpr const wchar_t* MONITOR_WND_CLASS = L"WinUtils_ProcessMonitor_Class";

struct MonitorThreadParam {
	HANDLE exitEvent;
	vector<wstring> targetProcesses;
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

bool WinUtils::EnumProcesses(const FindProcCallback& callback) {
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		WuDebug(LogLevel::Error, format(L"CreateToolhelp32Snapshot fail: {}", GetWindowsErrorMsg()));
		return false;
	}

	PROCESSENTRY32W entry{};
	entry.dwSize = sizeof(PROCESSENTRY32W);

	if (!Process32FirstW(hSnapshot, &entry)) {
		WuDebug(LogLevel::Error, format(L"Process32FirstW fail: {}", GetWindowsErrorMsg()));
		CloseHandle(hSnapshot);
		return false;
	}

	do {
		if (!callback(entry)) break;
	} while (Process32NextW(hSnapshot, &entry));

	CloseHandle(hSnapshot);
	return true;
}

std::vector<PROCESSENTRY32W> WinUtils::FindAllProcesses(const FindProcCallback& func) {
	vector<PROCESSENTRY32W> result;
	EnumProcesses([&](const PROCESSENTRY32W& entry) {
		if (func(entry)) result.push_back(entry);
		return true;
		});
	return result;
}

std::optional<PROCESSENTRY32W> WinUtils::FindFirstProcess(const FindProcCallback& func) {
	optional<PROCESSENTRY32W> result;
	EnumProcesses([&](const PROCESSENTRY32W& entry) {
		if (func(entry)) { result = entry; return false; }
		return true;
		});
	return result;
}

bool WinUtils::EnableDebugPrivilege() {
	HANDLE hToken = nullptr;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		WuDebug(LogLevel::Error, format(L"OpenProcessToken fail: {}", GetWindowsErrorMsg()));
		return false;
	}

	LUID luid{};
	if (!LookupPrivilegeValueW(nullptr, SE_DEBUG_NAME, &luid)) {
		WuDebug(LogLevel::Error, format(L"LookupPrivilegeValueW fail: {}", GetWindowsErrorMsg()));
		CloseHandle(hToken);
		return false;
	}

	TOKEN_PRIVILEGES tp{};
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

	bool ret = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr);
	if (!ret || GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
		WuDebug(LogLevel::Error, format(L"AdjustTokenPrivileges fail: {}", GetWindowsErrorMsg()));
		CloseHandle(hToken);
		return false;
	}

	CloseHandle(hToken);
	return true;
}

bool WinUtils::IsProcessRunning(std::wstring_view processName) {
	return FindFirstProcess([&](const PROCESSENTRY32W& entry) {
		return _wcsicmp(entry.szExeFile, processName.data()) == 0;
		}).has_value();
}

std::vector<DWORD> WinUtils::GetProcessIdsByName(std::wstring_view processName) {
	vector<DWORD> pids;
	EnumProcesses([&](const PROCESSENTRY32W& entry) {
		if (_wcsicmp(entry.szExeFile, processName.data()) == 0) pids.push_back(entry.th32ProcessID);
		return true;
		});
	return pids;
}

int WinUtils::TerminateProcessesByName(std::wstring_view processName) {
	int closedCount = 0;
	EnumProcesses([&](const PROCESSENTRY32W& entry) {
		if ((wstring)entry.szExeFile != processName || entry.th32ProcessID == GetCurrentProcessId())
			return true;

		HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
		if (!hProcess) {
			WuDebug(LogLevel::Error, format(L"OpenProcess fail [{}:{}]: {}", processName, entry.th32ProcessID, GetWindowsErrorMsg()));
			return true;
		}

		if (TerminateProcess(hProcess, 0)) {
			Logger::Inst().Info(format(L"Terminate success [{}:{}]", processName, entry.th32ProcessID));
			closedCount++;
		}
		else WuDebug(LogLevel::Error, format(L"Terminate fail [{}:{}]: {}", processName, entry.th32ProcessID, GetWindowsErrorMsg()));
		CloseHandle(hProcess);
		return true;
		});
	return closedCount;
}

int WinUtils::TerminateMultipleProcesses(const std::vector<std::wstring>& processNames) {
	int totalClosed = 0;
	for (const auto& name : processNames) totalClosed += TerminateProcessesByName(name);
	return totalClosed;
}

bool WinUtils::IsWindowTopMost(HWND hwnd) {
	if (!IsWindow(hwnd)) return false;
	return (GetWindowLongPtrW(hwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
}

bool WinUtils::SetWindowTopMost(HWND hwnd, bool isTopMost) {
	if (!IsWindow(hwnd)) return false;
	return SetWindowPos(hwnd, isTopMost ? HWND_TOPMOST : HWND_NOTOPMOST,
		0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

bool WinUtils::ForceHideWindow(HWND hwnd) {
	if (!IsWindow(hwnd)) return false;
	ShowWindow(hwnd, SW_MINIMIZE);
	SetWindowPos(hwnd, HWND_BOTTOM, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOOWNERZORDER | SWP_NOSENDCHANGING);
	LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
	SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exStyle | WS_EX_LAYERED);
	SetLayeredWindowAttributes(hwnd, 0, 0, LWA_ALPHA);
	return true;
}

bool WinUtils::IsWindowFullScreen(HWND hWnd, int tolerance, bool relative) {
	if (!IsWindow(hWnd)) return false;
	tolerance = (std::max)(tolerance, 0);

	RECT windowRect{};
	if (!::GetWindowRect(hWnd, &windowRect)) return false;

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

RECT WinUtils::GetWindowRect(HWND hWnd) {
	RECT rect{};
	if (IsWindow(hWnd)) ::GetWindowRect(hWnd, &rect);
	return rect;
}

bool WinUtils::GetWindowTitle(HWND hWnd, WCHAR* outTitle, int maxLength) {
	if (!IsWindow(hWnd) || !outTitle || maxLength <= 0) return false;
	int titleLen = GetWindowTextLengthW(hWnd);
	if (titleLen <= 0) { outTitle[0] = L'\0'; return false; }
	titleLen = (std::min)(titleLen, maxLength - 1);
	GetWindowTextW(hWnd, outTitle, titleLen + 1);
	return true;
}

std::wstring WinUtils::GetWindowTitleString(HWND hWnd) {
	if (!IsWindow(hWnd)) return L"";
	int titleLen = GetWindowTextLengthW(hWnd);
	if (titleLen <= 0) return L"";
	wstring title(titleLen, L'\0');
	GetWindowTextW(hWnd, title.data(), titleLen + 1);
	return title;
}

bool WinUtils::IsCurrentProcessAdmin() {
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

bool WinUtils::RequireAdminPrivilege(bool exit) {
	if (IsCurrentProcessAdmin()) return false;

	wstring appPath = GetCurrentProcessPath();
	HINSTANCE hResult = ShellExecuteW(nullptr, L"runas", appPath.c_str(),
		GetCommandLineW(), nullptr, SW_SHOWNORMAL);

	if (reinterpret_cast<INT_PTR>(hResult) <= 32) return false;
	if (exit) ExitProcess(0);
	return true;
}

void WinUtils::EnsureSingleInstance(std::wstring title, std::wstring name, std::wstring content) {
	if (name.empty()) name = GetCurrentProcessName();
	if (title.empty()) title = L"提示 - " + name;
	if (content.empty()) content = L"程序已在运行中！\n点击“确定”关闭已有实例，点击“取消”退出本次运行。";

	wstring mutexName = L"WinUtils_SingleInstance_" + name;
	HANDLE hMutex = CreateMutexW(nullptr, TRUE, mutexName.c_str());

	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		int ret = MessageBoxW(nullptr, content.c_str(), title.c_str(), MB_OKCANCEL | MB_ICONWARNING);
		if (ret == IDOK) TerminateProcessesByName(name);
		if(hMutex)ReleaseMutex(hMutex);
		ExitProcess(0);
	}
}

std::wstring WinUtils::GetCurrentProcessPath() {
	WCHAR path[MAX_PATH] = { 0 };
	GetModuleFileNameW(nullptr, path, _countof(path));
	return path;
}

std::wstring WinUtils::GetCurrentProcessDir() {
	return GetDirFromPath(GetCurrentProcessPath());
}

std::wstring WinUtils::GetCurrentProcessName() {
	return GetFileNameFromPath(GetCurrentProcessPath());
}

std::wstring WinUtils::GetFileNameFromPath(std::wstring_view path) {
	size_t pos = path.find_last_of(L"\\/");
	return pos == wstring_view::npos ? wstring(path) : wstring(path.substr(pos + 1));
}

std::wstring WinUtils::GetDirFromPath(std::wstring_view path) {
	size_t pos = path.find_last_of(L"\\/");
	return pos == wstring_view::npos ? L"" : wstring(path.substr(0, pos + 1));
}

// 错误处理
std::wstring WinUtils::GetWindowsErrorMsg(DWORD error_code) noexcept
{
	wchar_t* error_buf = nullptr;
	const DWORD buf_len = FormatMessageW(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		error_code,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<wchar_t*>(&error_buf),
		0,
		nullptr
	);

	std::wstring error_msg;
	if (buf_len > 0 && error_buf != nullptr)
	{
		error_msg = error_buf;
		LocalFree(error_buf);
	}
	else error_msg = L"未知错误";
	return error_msg;
}

HINSTANCE WinUtils::RunExternalProgram(std::wstring lpFile, std::wstring lpOperation, std::wstring lpParameters, std::wstring lpDirectory) {
	if (lpDirectory.empty()) lpDirectory = GetDirFromPath(lpFile);
	return ShellExecuteW(nullptr, lpOperation.empty() ? L"open" : lpOperation.c_str(),
		lpFile.c_str(), lpParameters.c_str(), lpDirectory.c_str(), SW_NORMAL);
}

int WinUtils::MonitorAndTerminateProcesses(HINSTANCE hInstance, const std::vector<std::wstring>& targetProcesses, int checkInterval) {
	WNDCLASSW wc{};
	wc.lpfnWndProc = MonitorWindowProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = MONITOR_WND_CLASS;
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

	if (RegisterClassW(&wc) == 0) {
		WuDebug(LogLevel::Error, format(L"RegisterClassW fail: {}", GetWindowsErrorMsg()));
		return 1;
	}

	HWND hwnd = CreateWindowExW(0, MONITOR_WND_CLASS, L"进程监控器", 0,
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