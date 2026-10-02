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
#include <iostream>
#include <algorithm>
#include <utility>  

#include "WinUtilsDef.h"

class WinUtils::Injector
{
public:
	// Get the PID list of all processes with the specified name
	static std::vector<DWORD> GetProcessPIDs(const string_t& processName);

	// Check whether the process with the specified PID is alive
	static bool CheckPIDAlive(DWORD pid);

	// Traditional remote thread DLL injection
	static bool InjectDLL(DWORD pid, const string_t& dllPath);

	// APC injection
	BOOL InjectDllViaAPC(DWORD pid, const string_t& dllPath);

	// Special APC injection
	static bool InjectDLLViaAPC2(DWORD pid, const string_t& dllPath);

	// Monitor the specified process and automatically inject the DLL
	static void MonitorAndInject(const string_t& dllPath, const string_t& processName, DWORD checkInterval = 2000);

	// Get all modules of the specified PID
	static std::vector<std::pair<HMODULE, string_t>> GetProcessModules(DWORD pid);

	// Unload the specified DLL from the process with the specified PID
	static bool UninjectDLL(DWORD pid, const string_t& dllPath);

	// Unload the specified DLL from all processes matching the process name,
	// returning the list of PIDs from which unloading succeeded
	static std::vector<DWORD> UninjectFromAllProcesses(const string_t& processName, const string_t& dllPath, const std::vector<DWORD>& excludeProcess = {});

	// Inject the DLL into all processes,
	// returning the list of PIDs where injection succeeded
	static std::vector<DWORD> InjectToAllProcesses(const string_t& processName, const string_t& dllPath, const std::vector<DWORD>& excludeProcess = {});

private:
	static std::vector<DWORD> GetAllThreadIdByProcessId(DWORD pid);
	static HANDLE CreateProcessSnapshot();
	static HANDLE CreateThreadSnapshot();
};