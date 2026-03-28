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

#include <Windows.h>
#include <vector>
#include <string>
#include <system_error>

#include "WinUtils/StrConvert.h"
#include "WinUtils/WinUtils.h"
#include "WinUtils/Logger.h"
using namespace std;

namespace WinUtils {
	namespace {
		static Logger logger(TS("StrConvert"));
	} // unnamed namespace

	// ----------------------------------------------------------------------------
	// WideToUtf8
	// ----------------------------------------------------------------------------
	std::string WideToUtf8(std::wstring_view wide_str) noexcept {
		if (wide_str.empty()) return {};

		try {
			// Calculate required UTF-8 buffer size (in bytes)
			int needed = WideCharToMultiByte(CP_UTF8, 0,
				wide_str.data(), -1,
				nullptr, 0, nullptr, nullptr);
			if (needed <= 0) {
				logger.DLog(LogLevel::Error,
					format(TS("WideToUtf8 failed - {}"), GetWindowsErrorMsg(GetLastError())));
				return {};
			}

			std::string utf8_str;
			utf8_str.resize(needed);
			int written = WideCharToMultiByte(CP_UTF8, 0,
				wide_str.data(), -1,
				utf8_str.data(), needed, nullptr, nullptr);
			if (written <= 0) {
				logger.DLog(LogLevel::Error,
					format(TS("WideToUtf8 failed - {}"), GetWindowsErrorMsg(GetLastError())));
				return {};
			}
			utf8_str.pop_back();
			return utf8_str;
		}
		catch (...) {
			return {};
		}
	}

	// ----------------------------------------------------------------------------
	// MultiByteToWide
	// ----------------------------------------------------------------------------
	std::wstring MultiByteToWide(std::string_view mb_str, UINT code_page) noexcept {
		if (mb_str.empty()) return {};

		try {
			// Calculate required wide character buffer size (in characters)
			int needed = MultiByteToWideChar(code_page, 0,
				mb_str.data(), -1,
				nullptr, 0);
			if (needed == 0) {
				logger.DLog(LogLevel::Error,
					format(TS("MultiByteToWide failed - {}"), GetWindowsErrorMsg(GetLastError())));
				return {};
			}

			std::vector<wchar_t> buffer(static_cast<size_t>(needed));
			int written = MultiByteToWideChar(code_page, 0,
				mb_str.data(), -1,
				buffer.data(), needed);
			if (written == 0) {
				logger.DLog(LogLevel::Error,
					format(TS("MultiByteToWide failed - {}"), GetWindowsErrorMsg(GetLastError())));
				return {};
			}
			return buffer.data();   // construct wstring from null-terminated buffer
		}
		catch (...) {
			return {};
		}
	}

	// ----------------------------------------------------------------------------
	// AnsiToWide
	// ----------------------------------------------------------------------------
	std::wstring AnsiToWide(std::string_view ansi_str) noexcept {
		return MultiByteToWide(ansi_str, CP_ACP);
	}

	// ----------------------------------------------------------------------------
	// Utf8ToWide
	// ----------------------------------------------------------------------------
	std::wstring Utf8ToWide(std::string_view utf8_str) noexcept {
		return MultiByteToWide(utf8_str, CP_UTF8);
	}

	// ----------------------------------------------------------------------------
	// WideToAnsi
	// ----------------------------------------------------------------------------
	std::string WideToAnsi(std::wstring_view wide_str) noexcept {
		if (wide_str.empty()) return {};

		try {
			int needed = WideCharToMultiByte(CP_ACP, 0,
				wide_str.data(), -1,
				nullptr, 0, nullptr, nullptr);
			if (needed <= 0) {
				logger.DLog(LogLevel::Error,
					format(TS("WideToAnsi failed - {}"), GetWindowsErrorMsg(GetLastError())));
				return {};
			}

			std::string ansi_str;
			ansi_str.resize(needed);
			int written = WideCharToMultiByte(CP_ACP, 0,
				wide_str.data(), -1,
				ansi_str.data(), needed, nullptr, nullptr);
			if (written <= 0) {
				logger.DLog(LogLevel::Error,
					format(TS("WideToAnsi failed - {}"), GetWindowsErrorMsg(GetLastError())));
				return {};
			}
			ansi_str.pop_back();
			return ansi_str;
		}
		catch (...) {
			return {};
		}
	}

} // namespace WinUtils