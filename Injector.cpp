#pragma once
#include "Injector.h"
#include "Logger.h"
#include <algorithm>
#include <format>
using namespace std;
using namespace WinUtils;
#define QUEUE_USER_APC_SPECIAL_USER_APC ((HANDLE)0x1)

typedef _Function_class_(PS_APC_ROUTINE) VOID NTAPI PS_APC_ROUTINE(
	_In_opt_ PVOID ApcArgument1,
	_In_opt_ PVOID ApcArgument2,
	_In_opt_ PVOID ApcArgument3
);
typedef PS_APC_ROUTINE* PPS_APC_ROUTINE;

typedef NTSTATUS(NTAPI* NtQueueApcThreadExFunc)(
	_In_ HANDLE ThreadHandle,
	_In_opt_ HANDLE ReserveHandle,
	_In_ PPS_APC_ROUTINE ApcRoutine,
	_In_opt_ PVOID ApcArgument1,
	_In_opt_ PVOID ApcArgument2,
	_In_opt_ PVOID ApcArgument3
	);

wstring ToLower(wstring_view str) noexcept {
	wstring res(str);
	transform(res.begin(), res.end(), res.begin(), ::towlower);
	return res;
}

void CleanupRemoteResources(HANDLE hProcess, LPVOID remoteMem) noexcept {
	if (remoteMem) VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
	if (hProcess) CloseHandle(hProcess);
}

// 获取指定进程PID列表
vector<DWORD> Injector::GetProcessPIDs(const wstring& processName)
{
	vector<DWORD> pids;
	HANDLE hSnapshot = CreateProcessSnapshot();
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		WuDebug(LogLevel::Error, L"创建进程快照失败");
		return pids;
	}

	PROCESSENTRY32W pe32{};
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	if (Process32FirstW(hSnapshot, &pe32)) {
		do {
			if (wcscmp(pe32.szExeFile, processName.c_str()) == 0) {
				pids.push_back(pe32.th32ProcessID);
			}
		} while (Process32NextW(hSnapshot, &pe32));
	}
	else {
		WuDebug(LogLevel::Error, format(L"枚举进程失败，错误码: {}", GetLastError()));
	}

	CloseHandle(hSnapshot);
	return pids;
}

// 检查PID是否存活
bool Injector::CheckPIDAlive(DWORD pid)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (!hProcess) return false;

	CloseHandle(hProcess);
	return true;
}

// 注入DLL到指定PID
bool Injector::InjectDLL(DWORD pid, const wstring& dllPath)
{
	// 打开目标进程
	HANDLE hProcess = OpenProcess(
		PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
		FALSE, pid);
	if (!hProcess) {
		WuDebug(LogLevel::Error, format(L"OpenProcess失败(PID:{})，错误码: {}", pid, GetLastError()));
		return false;
	}

	// 分配远程内存
	const size_t dllPathSize = (dllPath.length() + 1) * sizeof(wchar_t);
	LPVOID remoteMem = VirtualAllocEx(hProcess, nullptr, dllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!remoteMem) {
		WuDebug(LogLevel::Error, format(L"VirtualAllocEx失败(PID:{})，错误码: {}", pid, GetLastError()));
		CleanupRemoteResources(hProcess, nullptr);
		return false;
	}

	// 写入DLL路径
	if (!WriteProcessMemory(hProcess, remoteMem, dllPath.c_str(), dllPathSize, nullptr)) {
		WuDebug(LogLevel::Error, format(L"WriteProcessMemory失败(PID:{})，错误码: {}", pid, GetLastError()));
		CleanupRemoteResources(hProcess, remoteMem);
		return false;
	}

	// 获取LoadLibraryW地址
	HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
	PVOID loadLibAddr = hKernel32 ? GetProcAddress(hKernel32, "LoadLibraryW") : nullptr;
	if (!loadLibAddr) {
		WuDebug(LogLevel::Error, format(L"获取LoadLibraryW地址失败，错误码: {}", GetLastError()));
		CleanupRemoteResources(hProcess, remoteMem);
		return false;
	}

	// 创建远程线程注入
	HANDLE hRemoteThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibAddr, remoteMem, 0, nullptr);
	if (!hRemoteThread) {
		WuDebug(LogLevel::Error, format(L"CreateRemoteThread失败(PID:{})，错误码: {}", pid, GetLastError()));
		CleanupRemoteResources(hProcess, remoteMem);
		return false;
	}

	// 等待注入完成并清理
	WaitForSingleObject(hRemoteThread, INFINITE);
	CloseHandle(hRemoteThread);
	CleanupRemoteResources(hProcess, remoteMem);

	WuDebug(LogLevel::Info, format(L"DLL注入成功(PID:{})", pid));
	return true;
}

// 通过APC方式注入DLL
bool Injector::InjectDLLViaAPC(DWORD pid, const wstring& dllPath)
{
	if (!CheckPIDAlive(pid)) {
		WuDebug(LogLevel::Warn, format(L"PID {} 不存在或已退出", pid));
		return false;
	}

	// 打开目标进程
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	if (!hProcess) {
		WuDebug(LogLevel::Error, format(L"OpenProcess失败(PID:{})，错误码: {}", pid, GetLastError()));
		return false;
	}

	// 分配并写入远程内存
	const size_t dllPathSize = (dllPath.length() + 1) * sizeof(wchar_t);
	LPVOID remoteBuffer = VirtualAllocEx(hProcess, nullptr, dllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!remoteBuffer) {
		WuDebug(LogLevel::Error, format(L"VirtualAllocEx失败(PID:{})，错误码: {}", pid, GetLastError()));
		CleanupRemoteResources(hProcess, nullptr);
		return false;
	}

	if (!WriteProcessMemory(hProcess, remoteBuffer, dllPath.c_str(), dllPathSize, nullptr)) {
		WuDebug(LogLevel::Error, format(L"WriteProcessMemory失败(PID:{})，错误码: {}", pid, GetLastError()));
		CleanupRemoteResources(hProcess, remoteBuffer);
		return false;
	}

	// 获取LoadLibraryExW地址
	HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
	PPS_APC_ROUTINE loadDllFunc = hKernel32 ? (PPS_APC_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryExW") : nullptr;
	if (!loadDllFunc) {
		WuDebug(LogLevel::Error, format(L"获取LoadLibraryExW地址失败，错误码: {}", GetLastError()));
		CleanupRemoteResources(hProcess, remoteBuffer);
		return false;
	}

	// 获取NtQueueApcThreadEx地址
	HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
	NtQueueApcThreadExFunc ntQueueApcThreadEx = hNtdll ? (NtQueueApcThreadExFunc)GetProcAddress(hNtdll, "NtQueueApcThreadEx") : nullptr;
	if (!ntQueueApcThreadEx) {
		WuDebug(LogLevel::Error, format(L"获取NtQueueApcThreadEx地址失败，错误码: {}", GetLastError()));
		CleanupRemoteResources(hProcess, remoteBuffer);
		return false;
	}

	// 遍历目标进程线程
	HANDLE hThreadSnap = CreateThreadSnapshot();
	if (hThreadSnap == INVALID_HANDLE_VALUE) {
		WuDebug(LogLevel::Error, format(L"创建线程快照失败，错误码: {}", GetLastError()));
		CleanupRemoteResources(hProcess, remoteBuffer);
		return false;
	}

	THREADENTRY32 te32{};
	te32.dwSize = sizeof(THREADENTRY32);
	bool injectSuccess = false;

	if (Thread32First(hThreadSnap, &te32)) {
		do {
			if (te32.th32OwnerProcessID != pid) continue;

			WuDebug(LogLevel::Debug, format(L"找到目标线程(TID:{})，尝试注入APC...", te32.th32ThreadID));

			HANDLE hThread = OpenThread(THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
			if (!hThread) {
				WuDebug(LogLevel::Warn, format(L"OpenThread失败(TID:{})，错误码: {}", te32.th32ThreadID, GetLastError()));
				continue;
			}

			// 调用APC注入
			NTSTATUS status = ntQueueApcThreadEx(
				hThread, QUEUE_USER_APC_SPECIAL_USER_APC, loadDllFunc,
				remoteBuffer, nullptr, (PVOID)LOAD_WITH_ALTERED_SEARCH_PATH);

			CloseHandle(hThread);

			if (status == 0) {
				WuDebug(LogLevel::Info, format(L"APC注入成功(TID:{})！", te32.th32ThreadID));
				injectSuccess = true;
				break;
			}
			else {
				WuDebug(LogLevel::Error, format(L"NtQueueApcThreadEx失败(TID:{})，状态码: 0x{:X}", te32.th32ThreadID, status));
			}
		} while (Thread32Next(hThreadSnap, &te32));
	}
	else {
		WuDebug(LogLevel::Error, format(L"Thread32First失败，错误码: {}", GetLastError()));
	}

	// 清理资源
	CloseHandle(hThreadSnap);
	//CleanupRemoteResources(hProcess, remoteBuffer);//释放会导致注入进程崩溃
	return injectSuccess;
}

// 监控进程并自动注入
void Injector::MonitorAndInject(const wstring& dllPath, const wstring& processName, DWORD checkInterval)
{
	vector<DWORD> injectedPIDs;
	WuDebug(LogLevel::Info, format(L"开始监控进程: {}，注入DLL: {}，检查间隔: {}ms", processName, dllPath, checkInterval));
	WuDebug(LogLevel::Info, L"按Ctrl+C退出...");

	while (true) {
		vector<DWORD> currentPIDs = GetProcessPIDs(processName);
		for (DWORD pid : currentPIDs) {
			if (find(injectedPIDs.begin(), injectedPIDs.end(), pid) == injectedPIDs.end()) {
				WuDebug(LogLevel::Info, format(L"发现新进程(PID:{})，尝试注入...", pid));
				if (InjectDLL(pid, dllPath)) {
					injectedPIDs.push_back(pid);
				}
				else {
					WuDebug(LogLevel::Error, format(L"注入失败(PID:{})", pid));
				}
			}
		}
		injectedPIDs.erase(remove_if(injectedPIDs.begin(), injectedPIDs.end(),
			[](DWORD pid) { return !CheckPIDAlive(pid); }), injectedPIDs.end());

		Sleep(checkInterval);
	}
}

// 获取指定PID的所有模块
vector<pair<HMODULE, wstring>> Injector::GetProcessModules(DWORD pid)
{
	vector<pair<HMODULE, wstring>> modules;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);

	if (hSnapshot == INVALID_HANDLE_VALUE) {
		WuDebug(LogLevel::Error, format(L"创建模块快照失败(PID:{})，错误码: {}", pid, GetLastError()));
		return modules;
	}

	MODULEENTRY32W me32{};
	me32.dwSize = sizeof(MODULEENTRY32W);

	if (Module32FirstW(hSnapshot, &me32)) {
		do {
			modules.emplace_back(me32.hModule, me32.szModule);
		} while (Module32NextW(hSnapshot, &me32));
	}
	else {
		WuDebug(LogLevel::Error, format(L"枚举模块失败(PID:{})，错误码: {}", pid, GetLastError()));
	}

	CloseHandle(hSnapshot);
	return modules;
}

// 卸载指定PID的DLL
bool Injector::UninjectDLL(DWORD pid, const wstring& dllPath)
{
	// 标准化DLL文件名
	wstring dllFileName = dllPath.substr(dllPath.find_last_of(L"\\/") + 1);
	dllFileName = ToLower(dllFileName);

	// 打开目标进程
	HANDLE hProcess = OpenProcess(
		PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION,
		FALSE, pid);
	if (!hProcess) {
		WuDebug(LogLevel::Error, format(L"OpenProcess失败(PID:{})，错误码: {}", pid, GetLastError()));
		return false;
	}

	// 查找目标DLL模块
	HMODULE targetModule = nullptr;
	auto modules = GetProcessModules(pid);
	for (const auto& [hModule, moduleName] : modules) {
		if (ToLower(moduleName) == dllFileName) {
			targetModule = hModule;
			break;
		}
	}

	if (!targetModule) {
		WuDebug(LogLevel::Error, format(L"未找到目标DLL(PID:{})：{}", pid, dllFileName));
		CloseHandle(hProcess);
		return false;
	}

	// 卸载DLL
	HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
	LPVOID freeLibAddr = hKernel32 ? GetProcAddress(hKernel32, "FreeLibrary") : nullptr;
	if (!freeLibAddr) {
		WuDebug(LogLevel::Error, format(L"获取FreeLibrary地址失败，错误码: {}", GetLastError()));
		CloseHandle(hProcess);
		return false;
	}

	HANDLE hRemoteThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)freeLibAddr, targetModule, 0, nullptr);
	if (!hRemoteThread) {
		WuDebug(LogLevel::Error, format(L"CreateRemoteThread失败(PID:{})，错误码: {}", pid, GetLastError()));
		CloseHandle(hProcess);
		return false;
	}

	// 等待卸载完成
	WaitForSingleObject(hRemoteThread, INFINITE);
	CloseHandle(hRemoteThread);
	CloseHandle(hProcess);

	WuDebug(LogLevel::Info, format(L"DLL卸载成功(PID:{})：{}", pid, dllFileName));
	return true;
}

// 注入所有匹配的进程
vector<DWORD> Injector::InjectToAllProcesses(
	const wstring& processName,
	const wstring& dllPath,
	const vector<DWORD>& excludeProcess)
{
	vector<DWORD> successPIDs;
	HANDLE hSnapshot = CreateProcessSnapshot();
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		WuDebug(LogLevel::Error, format(L"创建进程快照失败，错误码: {}", GetLastError()));
		return successPIDs;
	}

	const wstring targetProcess = ToLower(processName);
	const DWORD currentPID = GetCurrentProcessId();
	PROCESSENTRY32W pe32{};
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	if (Process32FirstW(hSnapshot, &pe32)) {
		do {
			if (pe32.th32ProcessID == 0 || pe32.th32ProcessID == currentPID ||
				find(excludeProcess.begin(), excludeProcess.end(), pe32.th32ProcessID) != excludeProcess.end()) {
				continue;
			}
			if (ToLower(pe32.szExeFile) != targetProcess) continue;
			WuDebug(LogLevel::Info, format(L"尝试注入进程: {} (PID:{})", pe32.szExeFile, pe32.th32ProcessID));
			if (InjectDLL(pe32.th32ProcessID, dllPath)) {
				successPIDs.push_back(pe32.th32ProcessID);
				WuDebug(LogLevel::Info, format(L"注入成功: {} (PID:{})", pe32.szExeFile, pe32.th32ProcessID));
			}
			else WuDebug(LogLevel::Error, format(L"注入失败: {} (PID:{})", pe32.szExeFile, pe32.th32ProcessID));
		} while (Process32NextW(hSnapshot, &pe32));
	}
	else {
		WuDebug(LogLevel::Error, format(L"枚举进程失败，错误码: {}", GetLastError()));
	}

	CloseHandle(hSnapshot);
	return successPIDs;
}

// 创建进程快照
HANDLE Injector::CreateProcessSnapshot()
{
	return CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
}

// 创建线程快照
HANDLE Injector::CreateThreadSnapshot()
{
	return CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
}

// 从所有匹配进程卸载DLL
vector<DWORD> Injector::UninjectFromAllProcesses(
	const wstring& processName,
	const wstring& dllPath,
	const vector<DWORD>& excludeProcess)
{
	vector<DWORD> successPIDs;
	HANDLE hSnapshot = CreateProcessSnapshot();
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		WuDebug(LogLevel::Error, format(L"创建进程快照失败，错误码: {}", GetLastError()));
		return successPIDs;
	}

	const wstring targetProcess = ToLower(processName);
	const DWORD currentPID = GetCurrentProcessId();
	PROCESSENTRY32W pe32{};
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	if (Process32FirstW(hSnapshot, &pe32)) {
		do {
			if (pe32.th32ProcessID == 0 || pe32.th32ProcessID == currentPID ||
				find(excludeProcess.begin(), excludeProcess.end(), pe32.th32ProcessID) != excludeProcess.end()) {
				continue;
			}
			if (ToLower(pe32.szExeFile) != targetProcess) continue;
			WuDebug(LogLevel::Info, format(L"尝试卸载进程DLL: {} (PID:{})", pe32.szExeFile, pe32.th32ProcessID));
			if (UninjectDLL(pe32.th32ProcessID, dllPath)) {
				successPIDs.push_back(pe32.th32ProcessID);
			}
			else  WuDebug(LogLevel::Error, format(L"卸载失败: {} (PID:{})", pe32.szExeFile, pe32.th32ProcessID));
		} while (Process32NextW(hSnapshot, &pe32));
	}
	else {
		WuDebug(LogLevel::Error, format(L"枚举进程失败，错误码: {}", GetLastError()));
	}

	CloseHandle(hSnapshot);
	return successPIDs;
}