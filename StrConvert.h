#pragma once
#include "WinUtilsDef.h"
#include <Windows.h>
#include <string>
namespace WinUtils {
	std::wstring MultiByteToWide(const char* mb_str, UINT code_page) noexcept;
	std::wstring AnsiToWideString(const char* ansi_str) noexcept;
	std::wstring Utf8ToWideString(const char* utf8_str) noexcept;
	char* WideStringToAnsi(const std::wstring& wide_str) noexcept;
	std::wstring Utf8WideStringToWideString(const std::wstring& utf8_wide_str) noexcept;
}