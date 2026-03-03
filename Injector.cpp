#pragma once
#include "Injector.h"
#include "WinUtils.h"
#include "Logger.h"
#include <algorithm>
#include <format>
#include <filesystem>
#include "StrConvert.h"
using namespace std;
using namespace WinUtils;
namespace fs = std::filesystem;
static Logger logger(TS("Injector"));

#define QUEUE_USER_APC_SPECIAL_USER_APC ((HANDLE)0x1)
#define Wow64EncodeApcRoutine(ApcRoutine) \
    ((PVOID)((0 - ((LONG_PTR)(ApcRoutine))) << 2))

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

string_t ToLower(string_view_t str) noexcept {
	string_t res(str);
#if USE_WIDE_STRING
	transform(res.begin(), res.end(), res.begin(), ::towlower);
#else
	transform(res.begin(), res.end(), res.begin(), ::tolower);
#endif
	return res;
}

void CleanupRemoteResources(HANDLE hProcess, LPVOID remoteMem) noexcept {
	if (remoteMem) VirtualFreeEx(hProcess, remoteMem, 0, MEM_RELEASE);
	if (hProcess) CloseHandle(hProcess);
}

// Get PID list of the specified process, not case-sensitive
vector<DWORD> Injector::GetProcessPIDs(const string_t& processName)
{
	vector<DWORD> pids;
	HANDLE hSnapshot = CreateProcessSnapshot();
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		logger.DLog(LogLevel::Error, TS("Failed to create process snapshot"));
		return pids;
	}

	PROCESSENTRY32W pe32{};
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	if (Process32FirstW(hSnapshot, &pe32)) {
		do {
			if (ToLower(ConvertString<string_t>((wstring)pe32.szExeFile)) == ToLower(processName)) {
				pids.push_back(pe32.th32ProcessID);
			}
		} while (Process32NextW(hSnapshot, &pe32));
	}
	else {
		logger.DLog(LogLevel::Error, format(TS("Failed to enumerate processes, error code: {}"), GetLastError()));
	}

	CloseHandle(hSnapshot);
	return pids;
}

// Check if PID is active
bool Injector::CheckPIDAlive(DWORD pid)
{
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
	if (!hProcess) return false;

	CloseHandle(hProcess);
	return true;
}

// Inject DLL into the specified PID
bool Injector::InjectDLL(DWORD pid, const string_t& dllPath)
{
	HANDLE hProcess = OpenProcess(
		PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ,
		FALSE, pid);
	if (!hProcess) {
		logger.DLog(LogLevel::Error, format(TS("OpenProcess failed (PID:{}) , error code: {}"), pid, GetLastError()));
		return false;
	}

	const size_t dllPathSize = (dllPath.length() + 1) * sizeof(wchar_t);
	LPVOID remoteMem = VirtualAllocEx(hProcess, nullptr, dllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!remoteMem) {
		logger.DLog(LogLevel::Error, format(TS("VirtualAllocEx failed (PID:{}) , error code: {}"), pid, GetLastError()));
		CleanupRemoteResources(hProcess, nullptr);
		return false;
	}

	if (!WriteProcessMemory(hProcess, remoteMem, dllPath.c_str(), dllPathSize, nullptr)) {
		logger.DLog(LogLevel::Error, format(TS("WriteProcessMemory failed (PID:{}) , error code: {}"), pid, GetLastError()));
		CleanupRemoteResources(hProcess, remoteMem);
		return false;
	}

	HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
	PVOID loadLibAddr = hKernel32 ? (PVOID)GetProcAddress(hKernel32, "LoadLibraryW") : nullptr;
	if (!loadLibAddr) {
		logger.DLog(LogLevel::Error, format(TS("Failed to get LoadLibraryW address, error code: {}"), GetLastError()));
		CleanupRemoteResources(hProcess, remoteMem);
		return false;
	}

	HANDLE hRemoteThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibAddr, remoteMem, 0, nullptr);
	if (!hRemoteThread) {
		logger.DLog(LogLevel::Error, format(TS("CreateRemoteThread failed (PID:{}) , error code: {}"), pid, GetLastError()));
		CleanupRemoteResources(hProcess, remoteMem);
		return false;
	}

	WaitForSingleObject(hRemoteThread, INFINITE);
	CloseHandle(hRemoteThread);
	CleanupRemoteResources(hProcess, remoteMem);

	logger.DLog(LogLevel::Info, format(TS("DLL injection succeeded (PID:{})"), pid));
	return true;
}

// Get all thread IDs by process ID
vector<DWORD> Injector::GetAllThreadIdByProcessId(DWORD pid)
{
	vector<DWORD> pThreadIds;
	THREADENTRY32 te32{ .dwSize = sizeof(THREADENTRY32) };
	const HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hSnapshot == INVALID_HANDLE_VALUE)
	{
		return {};
	}

	DWORD threadCount = 0;
	for (BOOL bRet = Thread32First(hSnapshot, &te32); bRet; bRet = Thread32Next(hSnapshot, &te32))
	{
		if (te32.th32OwnerProcessID == pid)
		{
			pThreadIds.push_back(te32.th32ThreadID);
		}
	}

	CloseHandle(hSnapshot);
	return pThreadIds;
}

// Inject DLL via APC (Asynchronous Procedure Call)
BOOL Injector::InjectDllViaAPC(DWORD pid, const string_t& dllPath)
{
	if (pid == 0 || dllPath.empty()) return FALSE;

	vector<DWORD> pThreadIds = GetAllThreadIdByProcessId(pid);
	HANDLE hProcess = nullptr;
	LPVOID pRemoteBuf = nullptr;
	FARPROC pLoadLib = nullptr;

	auto Cleanup = [&]() {
		if (pRemoteBuf) VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
		if (hProcess) CloseHandle(hProcess);
		};
	if (pThreadIds.empty())
	{
		Cleanup();
		return FALSE;
	}

	hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ | PROCESS_CREATE_THREAD, FALSE, pid);
	if (!hProcess)
	{
		Cleanup();
		return FALSE;
	}

	const SIZE_T bufSize = (dllPath.size() + 1) * sizeof(wchar_t);
	pRemoteBuf = VirtualAllocEx(hProcess, nullptr, bufSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pRemoteBuf)
	{
		Cleanup();
		return FALSE;
	}

	SIZE_T written = 0;
	if (!WriteProcessMemory(hProcess, pRemoteBuf, dllPath.c_str(), bufSize, &written) || written != bufSize)
	{
		Cleanup();
		return FALSE;
	}
	HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
	pLoadLib = hKernel32 ? GetProcAddress(hKernel32, "LoadLibraryW") : nullptr;
	if (!pLoadLib)
	{
		Cleanup();
		return FALSE;
	}

	for (DWORD i = 0; i < pThreadIds.size(); ++i)
	{
		HANDLE hThread = OpenThread(THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, pThreadIds[i]);
		if (hThread)
		{
			QueueUserAPC(reinterpret_cast<PAPCFUNC>(pLoadLib), hThread, reinterpret_cast<ULONG_PTR>(pRemoteBuf));
			CloseHandle(hThread);
		}
	}

	Cleanup();
	return TRUE;
}

// Advanced DLL injection via NtQueueApcThreadEx
bool Injector::InjectDLLViaAPC2(DWORD pid, const string_t& dllPath)
{
	if (!CheckPIDAlive(pid) || dllPath.empty())
	{
		logger.DLog(LogLevel::Warn, std::format(TS("PID {} is invalid or DLL path is empty"), pid));
		return false;
	}

	HANDLE hProcess = nullptr;
	LPVOID pRemoteBuf = nullptr;
	bool injectSuccess = false;

	auto Cleanup = [&]() {
		if (pRemoteBuf) VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
		if (hProcess) CloseHandle(hProcess);
		};

	hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, pid);
	if (!hProcess)
	{
		logger.DLog(LogLevel::Error, std::format(TS("OpenProcess failed (PID:{}) , error code: {}"), pid, GetLastError()));
		Cleanup();
		return false;
	}
	const size_t bufSize = (dllPath.size() + 1) * sizeof(wchar_t);
	pRemoteBuf = VirtualAllocEx(hProcess, nullptr, bufSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!pRemoteBuf || !WriteProcessMemory(hProcess, pRemoteBuf, dllPath.c_str(), bufSize, nullptr))
	{
		logger.DLog(LogLevel::Error, std::format(TS("Memory operation failed (PID:{}) , error code: {}"), pid, GetLastError()));
		Cleanup();
		return false;
	}

	HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
	HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
	PPS_APC_ROUTINE pLoadLibEx = hKernel32 ? (PPS_APC_ROUTINE)GetProcAddress(hKernel32, "LoadLibraryExW") : nullptr;
	NtQueueApcThreadExFunc pNtQueueApc = hNtdll ? (NtQueueApcThreadExFunc)GetProcAddress(hNtdll, "NtQueueApcThreadEx") : nullptr;

	if (!pLoadLibEx || !pNtQueueApc)
	{
		logger.DLog(LogLevel::Error, std::format(TS("Failed to get function addresses, error code: {}"), GetLastError()));
		Cleanup();
		return false;
	}
	const HANDLE hThreadSnap = CreateThreadSnapshot();
	if (hThreadSnap == INVALID_HANDLE_VALUE)
	{
		logger.DLog(LogLevel::Error, std::format(TS("Failed to create thread snapshot, error code: {}"), GetLastError()));
		Cleanup();
		return false;
	}

	THREADENTRY32 te32{ .dwSize = sizeof(THREADENTRY32) };
	for (BOOL bRet = Thread32First(hThreadSnap, &te32); bRet; bRet = Thread32Next(hThreadSnap, &te32))
	{
		if (te32.th32OwnerProcessID != pid) continue;

		HANDLE hThread = OpenThread(THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, te32.th32ThreadID);
		if (!hThread)
		{
			logger.DLog(LogLevel::Warn, std::format(TS("OpenThread failed (TID:{}) , error code: {}"), te32.th32ThreadID, GetLastError()));
			continue;
		}

		// Queue APC (simplified parameters)
		const NTSTATUS status = pNtQueueApc(
			hThread, QUEUE_USER_APC_SPECIAL_USER_APC,
			(PPS_APC_ROUTINE)(pLoadLibEx),
			pRemoteBuf, nullptr, (PVOID)LOAD_WITH_ALTERED_SEARCH_PATH
		);

		CloseHandle(hThread);
		if (status == 0)
		{
			logger.DLog(LogLevel::Info, std::format(TS("APC injection succeeded (TID:{})!"), te32.th32ThreadID));
			injectSuccess = true;
			break;
		}
		logger.DLog(LogLevel::Error, std::format(TS("NtQueueApcThreadEx failed (TID:{}) , status code: 0x{:X}"), te32.th32ThreadID, status));
	}

	CloseHandle(hThreadSnap);
	if (hProcess) CloseHandle(hProcess);
	return injectSuccess;
}

// Monitor process and perform automatic injection
void Injector::MonitorAndInject(const string_t& dllPath, const string_t& processName, DWORD checkInterval)
{
	if (fs::path(dllPath).filename().empty()) {
		logger.DLog(LogLevel::Error, TS("Invalid DLL path"));
		return;
	}
	vector<DWORD> injectedPIDs;
	logger.DLog(LogLevel::Info, format(TS("Started process monitoring: {}, inject DLL: {}, check interval: {}ms"), processName, dllPath, checkInterval));
	logger.DLog(LogLevel::Info, TS("Press Ctrl+C to exit..."));

	while (true) {
		vector<DWORD> currentPIDs = GetProcessPIDs(processName);
		for (DWORD pid : currentPIDs) {
			if (find(injectedPIDs.begin(), injectedPIDs.end(), pid) == injectedPIDs.end()) {
				logger.DLog(LogLevel::Info, format(TS("Detected new process (PID:{}) , attempting injection..."), pid));
				if (InjectDLL(pid, dllPath)) {
					injectedPIDs.push_back(pid);
				}
				else {
					logger.DLog(LogLevel::Error, format(TS("Injection failed (PID:{})"), pid));
				}
			}
		}
		injectedPIDs.erase(remove_if(injectedPIDs.begin(), injectedPIDs.end(),
			[](DWORD pid) { return !CheckPIDAlive(pid); }), injectedPIDs.end());

		Sleep(checkInterval);
	}
}

// Get all modules of the specified PID
vector<pair<HMODULE, string_t>> Injector::GetProcessModules(DWORD pid)
{
	vector<pair<HMODULE, string_t>> modules;
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);

	if (hSnapshot == INVALID_HANDLE_VALUE) {
		logger.DLog(LogLevel::Error, format(TS("Failed to create module snapshot (PID:{}) , error code: {}"), pid, GetLastError()));
		return modules;
	}

	MODULEENTRY32 me32{};
	me32.dwSize = sizeof(MODULEENTRY32W);

	if (Module32FirstW(hSnapshot, &me32)) {
		do {
			modules.emplace_back(me32.hModule, ConvertString((wstring)me32.szModule));
		} while (Module32NextW(hSnapshot, &me32));
	}
	else {
		logger.DLog(LogLevel::Error, format(TS("Failed to enumerate modules (PID:{}) , error code: {}"), pid, GetLastError()));
	}

	CloseHandle(hSnapshot);
	return modules;
}

// Uninject DLL from the specified PID
bool Injector::UninjectDLL(DWORD pid, const string_t& dllPath)
{
	// Normalize DLL file name
	string_t dllFileName = dllPath.substr(dllPath.find_last_of(TS("\\/")) + 1);
	dllFileName = ToLower(dllFileName);

	// Open target process
	HANDLE hProcess = OpenProcess(
		PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION,
		FALSE, pid);
	if (!hProcess) {
		logger.DLog(LogLevel::Error, format(TS("OpenProcess failed (PID:{}) , error code: {}"), pid, GetLastError()));
		return false;
	}

	// Locate target DLL module
	HMODULE targetModule = nullptr;
	auto modules = GetProcessModules(pid);
	for (const auto& [hModule, moduleName] : modules) {
		if (ToLower(moduleName) == dllFileName) {
			targetModule = hModule;
			break;
		}
	}

	if (!targetModule) {
		logger.DLog(LogLevel::Error, format(TS("Target DLL not found (PID:{}) : {}"), pid, dllFileName));
		CloseHandle(hProcess);
		return false;
	}

	// Unload DLL
	HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
	LPVOID freeLibAddr = hKernel32 ? (LPVOID)GetProcAddress(hKernel32, "FreeLibrary") : nullptr;
	if (!freeLibAddr) {
		logger.DLog(LogLevel::Error, format(TS("Failed to get FreeLibrary address, error code: {}"), GetLastError()));
		CloseHandle(hProcess);
		return false;
	}

	HANDLE hRemoteThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)freeLibAddr, targetModule, 0, nullptr);
	if (!hRemoteThread) {
		logger.DLog(LogLevel::Error, format(TS("CreateRemoteThread failed (PID:{}) , error code: {}"), pid, GetLastError()));
		CloseHandle(hProcess);
		return false;
	}

	// Wait for unload completion
	WaitForSingleObject(hRemoteThread, INFINITE);
	CloseHandle(hRemoteThread);
	CloseHandle(hProcess);

	logger.DLog(LogLevel::Info, format(TS("DLL uninjection succeeded (PID:{}) : {}"), pid, dllFileName));
	return true;
}

// Inject DLL into all matching processes
vector<DWORD> Injector::InjectToAllProcesses(
	const string_t& processName,
	const string_t& dllPath,
	const vector<DWORD>& excludeProcess)
{
	vector<DWORD> successPIDs;
	HANDLE hSnapshot = CreateProcessSnapshot();
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		logger.DLog(LogLevel::Error, format(TS("Failed to create process snapshot, error code: {}"), GetLastError()));
		return successPIDs;
	}

	const string_t targetProcess = ToLower(processName);
	const DWORD currentPID = GetCurrentProcessId();
	PROCESSENTRY32W pe32{};
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	if (Process32FirstW(hSnapshot, &pe32)) {
		do {
			if (pe32.th32ProcessID == 0 || pe32.th32ProcessID == currentPID ||
				find(excludeProcess.begin(), excludeProcess.end(), pe32.th32ProcessID) != excludeProcess.end()) {
				continue;
			}
			if (ToLower(ConvertString<string_t>((wstring)pe32.szExeFile)) != targetProcess) continue;
			logger.DLog(LogLevel::Info, ConvertString(format(L"Attempting to inject process: {} (PID:{})", pe32.szExeFile, pe32.th32ProcessID)));
			if (InjectDLL(pe32.th32ProcessID, dllPath)) {
				successPIDs.push_back(pe32.th32ProcessID);
				logger.DLog(LogLevel::Info, ConvertString(format(L"Injection succeeded: {} (PID:{})", pe32.szExeFile, pe32.th32ProcessID)));
			}
			else logger.DLog(LogLevel::Error, ConvertString(format(L"Injection failed: {} (PID:{})", pe32.szExeFile, pe32.th32ProcessID)));
		} while (Process32NextW(hSnapshot, &pe32));
	}
	else {
		logger.DLog(LogLevel::Error, format(TS("Failed to enumerate processes, error code: {}"), GetLastError()));
	}

	CloseHandle(hSnapshot);
	return successPIDs;
}

// Create process snapshot
HANDLE Injector::CreateProcessSnapshot()
{
	return CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
}

// Create thread snapshot
HANDLE Injector::CreateThreadSnapshot()
{
	return CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
}

// Uninject DLL from all matching processes
vector<DWORD> Injector::UninjectFromAllProcesses(
	const string_t& processName,
	const string_t& dllPath,
	const vector<DWORD>& excludeProcess)
{
	vector<DWORD> successPIDs;
	HANDLE hSnapshot = CreateProcessSnapshot();
	if (hSnapshot == INVALID_HANDLE_VALUE) {
		logger.DLog(LogLevel::Error, format(TS("Failed to create process snapshot, error code: {}"), GetLastError()));
		return successPIDs;
	}

	const string_t targetProcess = ToLower(processName);
	const DWORD currentPID = GetCurrentProcessId();
	PROCESSENTRY32W pe32{};
	pe32.dwSize = sizeof(PROCESSENTRY32W);

	if (Process32FirstW(hSnapshot, &pe32)) {
		do {
			if (pe32.th32ProcessID == 0 || pe32.th32ProcessID == currentPID ||
				find(excludeProcess.begin(), excludeProcess.end(), pe32.th32ProcessID) != excludeProcess.end()) {
				continue;
			}
			if (ToLower(ConvertString((wstring)pe32.szExeFile)) != targetProcess) continue;
			logger.DLog(LogLevel::Info, ConvertString(format(L"Attempting to uninject DLL from process: {} (PID:{})", pe32.szExeFile, pe32.th32ProcessID)));
			if (UninjectDLL(pe32.th32ProcessID, dllPath)) {
				successPIDs.push_back(pe32.th32ProcessID);
			}
			else  logger.DLog(LogLevel::Error, ConvertString(format(L"Uninjection failed: {} (PID:{})", pe32.szExeFile, pe32.th32ProcessID)));
		} while (Process32NextW(hSnapshot, &pe32));
	}
	else {
		logger.DLog(LogLevel::Error, ConvertString( format(L"Failed to enumerate processes, error code: {}", GetLastError())));
	}

	CloseHandle(hSnapshot);
	return successPIDs;
}