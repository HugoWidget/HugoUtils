#pragma once
#include "WinUtilsDef.h"
#include <Windows.h>
#include <string>
#include <concepts>
namespace WinUtils {
	std::string WideStringToUtf8(const std::wstring_view wide_str) noexcept;
	std::wstring MultiByteToWide(const char* mb_str, UINT code_page) noexcept;
	std::wstring AnsiToWideString(const char* ansi_str) noexcept;
	std::wstring Utf8ToWideString(const char* utf8_str) noexcept;
	char* WideStringToAnsi(const std::wstring_view wide_str) noexcept;
	std::wstring Utf8WideStringToWideString(const std::wstring_view utf8_wide_str) noexcept;

	template<typename T>
	concept StringType = std::same_as<T, std::string> || std::same_as<T, std::wstring>;
	template<StringType To = string_t, StringType From>
	To ConvertString(const From& from)
	{
		if constexpr (std::is_same_v<From, To>)
		{
			return from;
		}
		else if constexpr (std::is_same_v<From, std::string> && std::is_same_v<To, std::wstring>)
		{
			return AnsiToWideString(from.c_str());
		}
		else if constexpr (std::is_same_v<From, std::wstring> && std::is_same_v<To, std::string>)
		{
			return WideStringToAnsi(from);
		}
		else
		{
			static_assert(!std::is_same_v<From, From>, "Unsupported string conversion");
			return To{};
		}
	}
}