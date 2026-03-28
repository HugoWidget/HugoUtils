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
#include <string_view>
#include <concepts>
#include <type_traits>

#include "WinUtilsDef.h"
namespace WinUtils {

    // ============================================================================
    // Basic conversion functions
    // ============================================================================

    /**
     * @brief Convert UTF-16 wide string view to UTF-8 narrow string.
     * @param wide_str Source wide string (any view/string).
     * @return UTF-8 std::string. Empty if conversion fails.
     */
    std::string WideToUtf8(std::wstring_view wide_str) noexcept;

    /**
     * @brief Convert multibyte string (given code page) to wide string.
     * @param mb_str     Source multibyte string view (bytes, not necessarily null-terminated).
     * @param code_page  Code page to interpret the bytes (e.g., CP_ACP, CP_UTF8).
     * @return Wide std::wstring. Empty if conversion fails.
     */
    WUAPI std::wstring MultiByteToWide(std::string_view mb_str, UINT code_page) noexcept;

    /**
     * @brief Convert ANSI (system default code page) string to wide string.
     * @param ansi_str ANSI string view.
     * @return Wide std::wstring.
     */
    WUAPI std::wstring AnsiToWide(std::string_view ansi_str) noexcept;

    /**
     * @brief Convert UTF-8 string to wide string.
     * @param utf8_str UTF-8 string view.
     * @return Wide std::wstring.
     */
    WUAPI std::wstring Utf8ToWide(std::string_view utf8_str) noexcept;

    /**
     * @brief Convert wide string to ANSI (system default code page) string.
     * @param wide_str Wide string view.
     * @return ANSI std::string. Empty if conversion fails.
     */
    WUAPI std::string WideToAnsi(std::wstring_view wide_str) noexcept;

    // ============================================================================
    // Concepts for generic string conversion (supports any string-like type)
    // ============================================================================

    // Types that can be viewed as std::string_view (narrow strings)
    template<typename T>
    concept NarrowStringLike = std::is_convertible_v<const T&, std::string_view>
        && !std::is_convertible_v<const T&, std::wstring_view>;

    // Types that can be viewed as std::wstring_view (wide strings)
    template<typename T>
    concept WideStringLike = std::is_convertible_v<const T&, std::wstring_view>
        && !std::is_convertible_v<const T&, std::string_view>;

    // Any source string type (narrow or wide)
    template<typename T>
    concept SourceStringType = NarrowStringLike<T> || WideStringLike<T>;

    // Target type must own its memory (prevent dangling pointers)
    template<typename T>
    concept OwnedStringType = std::same_as<T, std::string> || std::same_as<T, std::wstring>;

    // ============================================================================
    // Generic conversion template (now accepts all string-like types)
    // ============================================================================

    /**
     * @brief Universal string converter.
     * @tparam To   Target owned string type (std::string or std::wstring). Defaults to string_t.
     * @tparam From Source string type (must satisfy SourceStringType).
     * @param from  Source string (any narrow/wide string-like object).
     * @return      Converted string of type To. Empty if conversion fails or unsupported combination.
     *
     * Supported conversions:
     *   - Narrow source (const char*, char*, char[], std::string, std::string_view, etc.)
     *     -> std::string   (copy)
     *     -> std::wstring  (via AnsiToWide)
     *   - Wide source (const wchar_t*, wchar_t*, wchar_t[], std::wstring, std::wstring_view, etc.)
     *     -> std::wstring  (copy)
     *     -> std::string   (via WideToAnsi)
     */
    template<OwnedStringType To = string_t, SourceStringType From>
    To ConvertString(const From& from) noexcept {
        // Determine the character width of the source
        if constexpr (NarrowStringLike<From>) {
            // Source is narrow: obtain a string_view
            std::string_view sv(from);  // implicit conversion works for all narrow-like types

            if constexpr (std::is_same_v<To, std::string>) {
                // Narrow -> narrow: just copy
                return To(sv);
            }
            else if constexpr (std::is_same_v<To, std::wstring>) {
                // Narrow -> wide: convert using AnsiToWide
                return AnsiToWide(sv);
            }
            else {
                static_assert(!std::is_same_v<From, From>, "Unsupported target type for narrow source");
                return To{};
            }
        }
        else if constexpr (WideStringLike<From>) {
            // Source is wide: obtain a wstring_view
            std::wstring_view wsv(from);

            if constexpr (std::is_same_v<To, std::wstring>) {
                // Wide -> wide: just copy
                return To(wsv);
            }
            else if constexpr (std::is_same_v<To, std::string>) {
                // Wide -> narrow: convert using WideToAnsi
                return WideToAnsi(wsv);
            }
            else {
                static_assert(!std::is_same_v<From, From>, "Unsupported target type for wide source");
                return To{};
            }
        }
        else {
            // Should never reach here due to SourceStringType constraint
            static_assert(!std::is_same_v<From, From>, "Source type is neither narrow nor wide string-like");
            return To{};
        }
    }

} // namespace WinUtils