#pragma once
#include <string>
#include <sstream>
#include <string_view>
#include <iostream>

// Common definitions and type aliases for WinUtils library
#ifdef _DEBUG 
#define WINUTILS_DEBUG 1
#else 
#define WINUTILS_DEBUG 0
#endif
// Set to 1 to use MD5 hashing for generating mutex names in single instance enforcement, otherwise use simple sampling method
#define USE_HASHLIB 1
// Not using wide string may cause more expenses due to the need for frequent conversions between narrow and wide strings in Windows API calls,
// especially when dealing with file paths, process names, and window titles that may contain non-ASCII characters.
// Using wide strings (wchar_t) is generally recommended for better compatibility with internationalization and Unicode support in Windows applications.
#define USE_WIDE_STRING 1

namespace WinUtils {
#if  USE_WIDE_STRING
	using string_t = std::wstring;
	using stringstream_t = std::wstringstream;
	using string_view_t = std::wstring_view;
	using char_t = wchar_t;
	using ifstream_t = std::wifstream;
	using ofstream_t = std::wofstream;
	using istringstream_t = std::wistringstream;
	inline auto& tcout = std::wcout;
	inline auto& tcerr = std::wcerr;
	inline auto& tcin = std::wcin;
	template<typename T>
	std::wstring to_tstring(T var) {
		return std::to_wstring(var);
	}
#define TF(x) x##W
#define _TS(x) L##x
#else 
	using string_t = std::string;
	using stringstream_t = std::stringstream;
	using string_view_t = std::string_view;
	using char_t = char;
	using ifstream_t = std::ifstream;
	using ofstream_t = std::ofstream;
	using istringstream_t = std::wistringstream;
	inline auto& tcout = std::cout;
	inline auto& tcerr = std::cerr;
	inline auto& tcin = std::cin;
	template<typename T>
	std::string to_tstring(T var) {
		return std::to_string(var);
	}
#define TF(x) x##A
#define _TS(x) x
#endif
#define TS(x) _TS(x)
}


namespace WinUtils {
	class CmdParser;
	class Console;
	class HttpConnect;
	class Injector;
	class WinSvcMgr;
	class Logger;
	class ConsoleMenu;
	struct CommandItem;
	class MenuNode;

	template<typename T>
	class INIMap;
	class INIReader;
	class INIGenerator;
	class INIWriter;
	class INIFile;

	class RegException;
	class RegResult;
}