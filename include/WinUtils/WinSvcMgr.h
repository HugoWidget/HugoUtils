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

#include <string>

#include "WinUtilsDef.h"
namespace WinUtils {

	class WUAPI WinSvcMgr {
	public:
		explicit WinSvcMgr(const string_t& serviceName);

		// Install service
		bool Install(const string_t& binaryPath,
			const string_t& displayName = TS(""),
			DWORD startType = SERVICE_AUTO_START,
			const string_t& dependencies = TS(""),
			const string_t& account = TS(""),
			const string_t& password = TS(""));

		// Uninstall service
		bool Uninstall(bool forceStop = true);

		// Start service
		bool Start();

		// Stop service
		bool Stop();

		// Query service status (returns 0 on failure)
		DWORD GetStatus();

		// Get the last operation error code
		DWORD GetLastError() const { return m_lastError; }

		// Static method: check if service exists
		static bool Exists(const string_t& serviceName);

	private:
		string_t m_serviceName;
		DWORD m_lastError = 0;

		SC_HANDLE OpenSCM(DWORD desiredAccess);
		SC_HANDLE OpenSvc(DWORD desiredAccess);
	};

} // namespace WinUtils