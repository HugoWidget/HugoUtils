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
#include "WinPch.h"

#include <Windows.h>
#include "WinUtilsDef.h"
namespace WinUtils {
	class UIAccess {
	public:
		// Duplicate a token from winlogon.exe of the current session (for temporary impersonation)
		static DWORD DuplicateWinloginToken(DWORD dwSessionId, DWORD dwDesiredAccess, PHANDLE phToken);

		// Create a primary token with the UIAccess flag (requires SeTcbPrivilege)
		static DWORD CreateUIAccessToken(PHANDLE phToken);

		// Check whether the current process token has UIAccess enabled, and return the result in pfUIAccess (1 for true, 0 for false)
		static BOOL HasUIAccess(DWORD* pdwErr, DWORD* pfUIAccess);

		// If the current process does not have UIAccess, restart itself using a new token
		static DWORD RequireUIAccess(bool exit = true, WinUtils::string_t lpCmdLine = TF(GetCommandLine)());
		static DWORD RequireUIAccessWithParams(bool exit = true, WinUtils::string_t params = TS(""));

	private:
		struct UniqueHandle {
			HANDLE handle_ = nullptr;

			UniqueHandle() noexcept = default;
			explicit UniqueHandle(HANDLE h) noexcept;
			~UniqueHandle();

			UniqueHandle(const UniqueHandle&) = delete;
			UniqueHandle& operator=(const UniqueHandle&) = delete;

			UniqueHandle(UniqueHandle&& other) noexcept;
			UniqueHandle& operator=(UniqueHandle&& other) noexcept;

			HANDLE get() const noexcept;
			HANDLE release() noexcept;
			void reset(HANDLE h = nullptr) noexcept;
			explicit operator bool() const noexcept;
		};
	};
} // namespace WinUtils