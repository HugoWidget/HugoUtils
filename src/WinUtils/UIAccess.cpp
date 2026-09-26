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
#include "WinUtils/UIAccess.h"
#include <Tlhelp32.h>
#include <string_view>
#include <cstdlib>
namespace {
	WinUtils::string_t GetCurrentProcessPath2() {
		WinUtils::char_t path[MAX_PATH] = {};
		DWORD dwLen = TF(GetModuleFileName)(nullptr, path, _countof(path));
		if (dwLen == 0)return TS("");
		if (dwLen >= _countof(path)) return TS("");
		return path;
	}
}

namespace WinUtils {
	UIAccess::UniqueHandle::UniqueHandle(HANDLE h) noexcept : handle_(h) {}

	UIAccess::UniqueHandle::~UniqueHandle() {
		reset();
	}

	UIAccess::UniqueHandle::UniqueHandle(UniqueHandle&& other) noexcept
		: handle_(other.release()) {
	}

	UIAccess::UniqueHandle& UIAccess::UniqueHandle::operator=(UniqueHandle&& other) noexcept {
		if (this != &other) {
			reset(other.release());
		}
		return *this;
	}

	HANDLE UIAccess::UniqueHandle::get() const noexcept {
		return handle_;
	}

	HANDLE UIAccess::UniqueHandle::release() noexcept {
		HANDLE h = handle_;
		handle_ = nullptr;
		return h;
	}

	void UIAccess::UniqueHandle::reset(HANDLE h) noexcept {
		if (handle_ && handle_ != INVALID_HANDLE_VALUE) {
			::CloseHandle(handle_);
		}
		handle_ = h;
	}

	UIAccess::UniqueHandle::operator bool() const noexcept {
		return handle_ && handle_ != INVALID_HANDLE_VALUE;
	}

	DWORD UIAccess::DuplicateWinloginToken(DWORD dwSessionId, DWORD dwDesiredAccess, PHANDLE phToken) {
		if (!phToken) {
			return ERROR_INVALID_PARAMETER;
		}
		*phToken = nullptr;

		PRIVILEGE_SET ps;
		ps.PrivilegeCount = 1;
		ps.Control = PRIVILEGE_SET_ALL_NECESSARY;
		if (!::LookupPrivilegeValueW(nullptr, SE_TCB_NAME, &ps.Privilege[0].Luid)) {
			return ::GetLastError();
		}

		UniqueHandle hSnapshot(::CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
		if (!hSnapshot) {
			return ::GetLastError();
		}

		PROCESSENTRY32W pe{};
		pe.dwSize = sizeof(pe);

		BOOL bFound = FALSE;
		DWORD dwErr = ERROR_NOT_FOUND;

		for (BOOL bCont = ::Process32FirstW(hSnapshot.get(), &pe); bCont;
			bCont = ::Process32NextW(hSnapshot.get(), &pe)) {

			if (_wcsicmp(pe.szExeFile, L"winlogon.exe") != 0) {
				continue;
			}

			UniqueHandle hProcess(::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pe.th32ProcessID));
			if (!hProcess) {
				continue;
			}

			UniqueHandle hToken;
			if (!::OpenProcessToken(hProcess.get(), TOKEN_QUERY | TOKEN_DUPLICATE, &hToken.handle_)) {
				continue;
			}

			BOOL fTcb = FALSE;
			if (!::PrivilegeCheck(hToken.get(), &ps, &fTcb) || !fTcb) {
				continue;
			}

			DWORD sid = 0;
			DWORD dwRetLen = 0;
			if (!::GetTokenInformation(hToken.get(), TokenSessionId, &sid, sizeof(sid), &dwRetLen) || sid != dwSessionId) {
				continue;
			}

			bFound = TRUE;
			HANDLE hDup = nullptr;
			if (::DuplicateTokenEx(hToken.get(), dwDesiredAccess, nullptr,
				SecurityImpersonation, TokenImpersonation, &hDup)) {
				*phToken = hDup;
				dwErr = ERROR_SUCCESS;
			}
			else {
				dwErr = ::GetLastError();
			}
			break;
		}

		return dwErr;
	}

	DWORD UIAccess::CreateUIAccessToken(PHANDLE phToken) {
		if (!phToken) {
			return ERROR_INVALID_PARAMETER;
		}
		*phToken = nullptr;

		UniqueHandle hTokenSelf;
		if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE, &hTokenSelf.handle_)) {
			return ::GetLastError();
		}

		DWORD dwSessionId = 0;
		DWORD dwRetLen = 0;
		if (!::GetTokenInformation(hTokenSelf.get(), TokenSessionId, &dwSessionId, sizeof(dwSessionId), &dwRetLen)) {
			return ::GetLastError();
		}

		UniqueHandle hTokenSystem;
		DWORD dwErr = DuplicateWinloginToken(dwSessionId, TOKEN_IMPERSONATE, &hTokenSystem.handle_);
		if (dwErr != ERROR_SUCCESS) {
			return dwErr;
		}

		if (!::SetThreadToken(nullptr, hTokenSystem.get())) {
			return ::GetLastError();
		}

		HANDLE hNewToken = nullptr;
		if (!::DuplicateTokenEx(hTokenSelf.get(),
			TOKEN_QUERY | TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_ADJUST_DEFAULT,
			nullptr, SecurityAnonymous, TokenPrimary, &hNewToken)) {
			dwErr = ::GetLastError();
			::RevertToSelf();
			return dwErr;
		}

		BOOL bUIAccess = TRUE;
		if (!::SetTokenInformation(hNewToken, TokenUIAccess, &bUIAccess, sizeof(bUIAccess))) {
			dwErr = ::GetLastError();
			::CloseHandle(hNewToken);
			::RevertToSelf();
			return dwErr;
		}

		*phToken = hNewToken;
		dwErr = ERROR_SUCCESS;

		::RevertToSelf();
		return dwErr;
	}

	BOOL WinUtils::UIAccess::HasUIAccess(DWORD* pdwErr, DWORD* pfUIAccess) {
		if (!pdwErr || !pfUIAccess) {
			return FALSE;
		}
		*pdwErr = ERROR_SUCCESS;

		UniqueHandle hToken;
		if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, &hToken.handle_)) {
			*pdwErr = ::GetLastError();
			return FALSE;
		}

		DWORD dwRetLen = 0;
		if (!::GetTokenInformation(hToken.get(), TokenUIAccess, pfUIAccess, sizeof(*pfUIAccess), &dwRetLen)) {
			*pdwErr = ::GetLastError();
			return FALSE;
		}

		return TRUE;
	}

	DWORD WinUtils::UIAccess::RequireUIAccess(bool exit, string_t cmdLine) {
		if (cmdLine.empty())return ERROR_INVALID_PARAMETER;
		DWORD dwErr = ERROR_SUCCESS;
		DWORD fUIAccess = FALSE;

		if (!HasUIAccess(&dwErr, &fUIAccess)) {
			return dwErr;
		}

		if (fUIAccess != 0) {
			return ERROR_SUCCESS;
		}

		UniqueHandle hTokenUIAccess;
		dwErr = CreateUIAccessToken(&hTokenUIAccess.handle_);
		if (dwErr != ERROR_SUCCESS) {
			return dwErr;
		}

		STARTUPINFOW si{};
		si.cb = sizeof(si);
		::GetStartupInfoW(&si);

		PROCESS_INFORMATION pi{};
		BOOL bCreated = TF(CreateProcessAsUser)(
			hTokenUIAccess.get(),
			nullptr,
			cmdLine.data(),
			nullptr, nullptr,
			FALSE,
			0,
			nullptr,
			nullptr,
			&si,
			&pi
			);

		if (bCreated) {
			::CloseHandle(pi.hProcess);
			::CloseHandle(pi.hThread);
			if (exit)::ExitProcess(0);
		}
		else {
			dwErr = ::GetLastError();
		}

		return dwErr;
	}

	DWORD WinUtils::UIAccess::RequireUIAccessWithParams(bool exit, string_t params)
	{
		const string_t path = GetCurrentProcessPath2();
		if (path.empty()) {
			return ERROR_FILE_NOT_FOUND;
		}

		string_t cmdLine;
		cmdLine.reserve(path.size() + params.size() + 8);
		cmdLine += L"\"";
		cmdLine += path;
		cmdLine += L"\"";

		if (!params.empty()) {
			cmdLine += L" ";
			cmdLine += params;
		}

		return RequireUIAccess(exit, cmdLine);
	}

} // namespace WinUtils