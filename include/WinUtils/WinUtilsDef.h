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
#include <string>
#include <sstream>
#include <string_view>
#include <iostream>

// Common definitions and type aliases for WinUtils library
#ifdef _DEBUG 
#define WU_DEBUG
#endif
// Manually define the macros to enable different features of the library.
// You can define these in your project settings or before including this header in your source files.
//
// Dynamic library linkage or Static library linkage (recommended for ABI compatibility)
// #define WU_DYNAMIC_LINK
//
// Wide string usage
// Using wide strings (wchar_t) is recommended for better compatibility with
// internationalization and Unicode support in Windows applications. Not using
// wide strings may incur performance penalties due to frequent conversions
// between narrow and wide strings in Windows API calls.
// #define WU_NARROW_STRING
//
// INI parser case sensitivity (used in ini.h only)
// #define WU_NO_INI_CASE_SENSITIVE

#ifdef WU_DYNAMIC_LINK   
#ifdef WU_EXPORTS   
#define WUAPI __declspec(dllexport)
#else
#define WUAPI __declspec(dllimport) 
#endif // WU_EXPORTS
#else             
#define WUAPI 
#endif // WU_DYNAMIC_LINK

namespace WinUtils {
#ifdef  WU_NARROW_STRING
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
#else 
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