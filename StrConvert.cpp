#include "StrConvert.h"
#include <windows.h>
#include <vector>
#include <string>
#include "WinUtils.h"
#include "Logger.h"
using namespace std;
using namespace WinUtils;

std::wstring WinUtils::MultiByteToWide(const char* mb_str, UINT code_page) noexcept
{
    if (mb_str == nullptr || *mb_str == '\0')
    {
        return {};
    }

    // ¼ÆËãËùÐè³¤¶È
    const int len = MultiByteToWideChar(code_page, 0, mb_str, -1, nullptr, 0);
    if (len == 0)
    {
        const DWORD err = GetLastError();
        WuDebug(LogLevel::Error, format(L"MultiByteToWide¼ÆËã³¤¶ÈÊ§°Ü[±àÂëÒ³:{}] - {}", code_page, GetWindowsErrorMsg(err)));
        return {};
    }

    vector<wchar_t> wide_buf(static_cast<size_t>(len));
    const int res = MultiByteToWideChar(code_page, 0, mb_str, -1, wide_buf.data(), len);
    if (res == 0)
    {
        const DWORD err = GetLastError();
        WuDebug(LogLevel::Error, format(L"MultiByteToWide×ª»»Ê§°Ü[±àÂëÒ³:{}] - {}", code_page, GetWindowsErrorMsg(err)));
        return {};
    }

    return wstring(wide_buf.data());
}

// ANSI(char*) ×ª ¿í×Ö·û´®(wstring/Unicode)
std::wstring WinUtils::AnsiToWideString(const char* ansi_str) noexcept
{
    return MultiByteToWide(ansi_str, CP_ACP);
}

// UTF8(char*) ×ª ¿í×Ö·û´®(wstring/Unicode)
std::wstring WinUtils::Utf8ToWideString(const char* utf8_str) noexcept
{
    return MultiByteToWide(utf8_str, CP_UTF8);
}

// ¿í×Ö·û´®(wstring/Unicode) ×ª ANSI(char*)
[[nodiscard]] char* WinUtils::WideStringToAnsi(const std::wstring& wide_str) noexcept
{
    if (wide_str.empty())return nullptr;

    const int len = WideCharToMultiByte(CP_ACP, 0, wide_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (len == 0)
    {
        WuDebug(LogLevel::Error, format(L"WideStringToAnsi¼ÆËã³¤¶ÈÊ§°Ü - {}", GetWindowsErrorMsg(GetLastError())));
        return nullptr;
    }

    char* ansi_buf = new char[static_cast<size_t>(len)];
    const int res = WideCharToMultiByte(CP_ACP, 0, wide_str.c_str(), -1, ansi_buf, len, nullptr, nullptr);
    if (res == 0)
    {
        WuDebug(LogLevel::Error, format(L"WideStringToAnsi×ª»»Ê§°Ü - {}", GetWindowsErrorMsg(GetLastError())));
        delete[] ansi_buf;
        return nullptr;
    }

    return ansi_buf;
}

// UTF8±àÂëµÄ¿í×Ö·û´®(wstring) ×ª ±ê×¼Unicode¿í×Ö·û´®(wstring)
std::wstring WinUtils::Utf8WideStringToWideString(const std::wstring& utf8_wide_str) noexcept
{
    if (utf8_wide_str.empty())return {};
    

    const int ansi_len = WideCharToMultiByte(CP_ACP, 0, utf8_wide_str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (ansi_len == 0)
    {
        WuDebug(LogLevel::Error, format(L"Utf8WideStringToWideString: ¼ÆËãANSI³¤¶ÈÊ§°Ü - {}", GetWindowsErrorMsg(GetLastError())));
        return {};
    }

    vector<char> ansi_buf(static_cast<size_t>(ansi_len));
    const int convert_ansi = WideCharToMultiByte(CP_ACP, 0, utf8_wide_str.c_str(), -1, ansi_buf.data(), ansi_len, nullptr, nullptr);
    if (convert_ansi == 0)
    {
        WuDebug(LogLevel::Error, format(L"Utf8WideStringToWideString: ×ª»»ANSIÊ§°Ü - {}", GetWindowsErrorMsg(GetLastError())));
        return {};
    }

    return MultiByteToWide(ansi_buf.data(), CP_UTF8);
}