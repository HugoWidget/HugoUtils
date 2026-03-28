/*
 * Copyright 2025-2026 howdy213, JYardX
 *
 * This file is part of HugoUtils.
 *
 * HugoUtils is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HugoUtils is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with HugoUtils. If not, see <https://www.gnu.org/licenses/>.
 */
#pragma once
#include "HugoUtils/HugoUtilsDef.h"

#if HU_INSTALLER

#include <string>
#include <functional>
#include <filesystem>
#include <utility>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include "httplib/httplib.h"
namespace fs = std::filesystem;

inline constexpr const wchar_t* DEFAULT_NAME = L"";
inline constexpr const wchar_t BASE_URL[] = L"https://e.seewo.com/download/file?code=SeewoServiceSetup";

struct PackageInfo {
    std::wstring version;
    std::wstring filePath;
};

struct DownloadResult {
    bool success = false;
    int statusCode = 0;
    std::string errorMsg;
    size_t downloaded = 0;
    size_t total = 0;
    std::string filename;
};

class HttpDownloader {
public:
    explicit HttpDownloader(int maxRedirects = 10);

    void setTimeout(int seconds);
    void setUserAgent(std::string ua);

    DownloadResult download(
        const std::string& url,
        const std::string& outDir,
        const std::string& customName = {},
        std::function<void(int64_t, int64_t)> progress = nullptr,
        bool resume = true
    );

private:
    int m_maxRedirects;
    int m_timeout = 30;
    std::string m_userAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36";

    struct UrlParts {
        std::string host, path;
        bool isHttps = false;
    };

    UrlParts parseUrl(const std::string& url);
    std::string getFilenameFromDisposition(const std::string& hdr);
    httplib::Headers makeHeaders(size_t rangeStart);
    std::string urlDecode(const std::string& encoded);
    std::string extractFilenameFromRedirectUrl(const std::string& url);
};

#endif // HU_INSTALLER