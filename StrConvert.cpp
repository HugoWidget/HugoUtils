#include "StrConvert.h"
#include <windows.h>
#include <vector>
#include <string>
#include "WinUtils.h"
#include "Logger.h"
using namespace std;
using namespace WinUtils;
static Logger logger(L"StrConvert");

std::string WinUtils::WideStringToUtf8(const std::wstring_view wide_str) noexcept
{
    if (wide_str.empty()) return {};

    int buffer_size = WideCharToMultiByte(CP_UTF8, 0, wide_str.data(), (int)wide_str.length(),
        nullptr, 0, nullptr, nullptr);
    if (buffer_size <= 0)
    {
        logger.DLog(LogLevel::Error, std::format(L"WideStringToUtf8: Failed to calculate length - {}", GetWindowsErrorMsg(GetLastError())));
        return {};
    }

    std::string utf8_str;
    utf8_str.resize(buffer_size);
    WideCharToMultiByte(CP_UTF8, 0, wide_str.data(), (int)wide_str.length(),
        utf8_str.data(), buffer_size, nullptr, nullptr);
    return utf8_str;
}

std::wstring WinUtils::MultiByteToWide(const char* mb_str, UINT code_page) noexcept
{
    if (mb_str == nullptr || *mb_str == '\0')
    {
        return {};
    }

    // Calculate required buffer length
    const int len = MultiByteToWideChar(code_page, 0, mb_str, -1, nullptr, 0);
    if (len == 0)
    {
        const DWORD err = GetLastError();
        logger.DLog(LogLevel::Error, format(L"MultiByteToWide: Failed to calculate length [Code Page:{}] - {}", code_page, GetWindowsErrorMsg(err)));
        return {};
    }

    vector<wchar_t> wide_buf(static_cast<size_t>(len));
    const int res = MultiByteToWideChar(code_page, 0, mb_str, -1, wide_buf.data(), len);
    if (res == 0)
    {
        const DWORD err = GetLastError();
        logger.DLog(LogLevel::Error, format(L"MultiByteToWide: Conversion failed [Code Page:{}] - {}", code_page, GetWindowsErrorMsg(err)));
        return {};
    }

    return wstring(wide_buf.data());
}

// Convert ANSI (char*) to wide string (wstring/Unicode)
std::wstring WinUtils::AnsiToWideString(const char* ansi_str) noexcept
{
    return MultiByteToWide(ansi_str, CP_ACP);
}

// Convert UTF-8 (char*) to wide string (wstring/Unicode)
std::wstring WinUtils::Utf8ToWideString(const char* utf8_str) noexcept
{
    return MultiByteToWide(utf8_str, CP_UTF8);
}

// Convert wide string (wstring/Unicode) to ANSI (char*)
[[nodiscard]] char* WinUtils::WideStringToAnsi(const std::wstring_view wide_str) noexcept
{
    if (wide_str.empty())return nullptr;

    const int len = WideCharToMultiByte(CP_ACP, 0, wide_str.data(), -1, nullptr, 0, nullptr, nullptr);
    if (len == 0)
    {
        logger.DLog(LogLevel::Error, format(L"WideStringToAnsi: Failed to calculate length - {}", GetWindowsErrorMsg(GetLastError())));
        return nullptr;
    }

    char* ansi_buf = new char[static_cast<size_t>(len)];
    const int res = WideCharToMultiByte(CP_ACP, 0, wide_str.data(), -1, ansi_buf, len, nullptr, nullptr);
    if (res == 0)
    {
        logger.DLog(LogLevel::Error, format(L"WideStringToAnsi: Conversion failed - {}", GetWindowsErrorMsg(GetLastError())));
        delete[] ansi_buf;
        return nullptr;
    }

    return ansi_buf;
}

// Convert UTF-8 encoded wide string (wstring) to standard Unicode wide string (wstring)
std::wstring WinUtils::Utf8WideStringToWideString(const std::wstring_view utf8_wide_str) noexcept
{
    if (utf8_wide_str.empty())return {};

    const int ansi_len = WideCharToMultiByte(CP_ACP, 0, utf8_wide_str.data(), -1, nullptr, 0, nullptr, nullptr);
    if (ansi_len == 0)
    {
        logger.DLog(LogLevel::Error, format(L"Utf8WideStringToWideString: Failed to calculate ANSI length - {}", GetWindowsErrorMsg(GetLastError())));
        return {};
    }

    vector<char> ansi_buf(static_cast<size_t>(ansi_len));
    const int convert_ansi = WideCharToMultiByte(CP_ACP, 0, utf8_wide_str.data(), -1, ansi_buf.data(), ansi_len, nullptr, nullptr);
    if (convert_ansi == 0)
    {
        logger.DLog(LogLevel::Error, format(L"Utf8WideStringToWideString: ANSI conversion failed - {}", GetWindowsErrorMsg(GetLastError())));
        return {};
    }

    return MultiByteToWide(ansi_buf.data(), CP_UTF8);
}