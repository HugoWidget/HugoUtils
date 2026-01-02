#pragma once
#include <windows.h>

#include <tlhelp32.h>
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <utility>  
#include "WinUtilsDef.h"

class WinUtils::Injector
{
public:
    // 获取所有指定名称进程的PID列表
    static std::vector<DWORD> GetProcessPIDs(const std::wstring& processName);

    // 检查指定PID的进程是否存活
    static bool CheckPIDAlive(DWORD pid);

    // 传统远程线程注入DLL
    static bool InjectDLL(DWORD pid, const std::wstring& dllPath);

    // APC注入DLL
    static bool InjectDLLViaAPC(DWORD pid, const std::wstring& dllPath);

    // 监控指定进程并自动注入DLL
    static void MonitorAndInject(const std::wstring& dllPath, const std::wstring& processName, DWORD checkInterval = 2000);

    // 获取指定PID的所有模块
    static std::vector<std::pair<HMODULE, std::wstring>> GetProcessModules(DWORD pid);

    // 从指定PID的进程中卸载指定DLL
    static bool UninjectDLL(DWORD pid, const std::wstring& dllPath);

    // 从所有匹配进程名的进程中卸载指定DLL，返回卸载成功的PID列表
    static std::vector<DWORD> UninjectFromAllProcesses(const std::wstring& processName, const std::wstring& dllPath, const std::vector<DWORD>& excludeProcess = {});

    // 注入DLL到所有进程，返回注入成功的PID列表
    static std::vector<DWORD> InjectToAllProcesses(const std::wstring& processName, const std::wstring& dllPath, const std::vector<DWORD>& excludeProcess = {});

private:

    static HANDLE CreateProcessSnapshot();
    static HANDLE CreateThreadSnapshot();
};