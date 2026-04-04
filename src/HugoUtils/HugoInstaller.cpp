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
#include "HugoUtils/HugoUtilsDef.h"
#ifndef HU_DISABLE_INSTALLER
#include <iostream>
#include <fstream>
#include <chrono>
#include <regex>
#include <vector>
#include <limits>
#include <sstream>
#include <algorithm>

#include <windows.h>
#include <shellapi.h>
#include "WinUtils/Console.h"
#include "WinUtils/StrConvert.h"
#include "WinUtils/WinUtils.h"

#include "HugoUtils/HugoInstaller.h"
using namespace std;
using namespace WinUtils;

HttpDownloader::HttpDownloader(int maxRedirects)
    : m_maxRedirects(maxRedirects) {
}

void HttpDownloader::setTimeout(int seconds) {
    m_timeout = seconds;
}

void HttpDownloader::setUserAgent(string ua) {
    m_userAgent = move(ua);
}

DownloadResult HttpDownloader::download(
    const string& url,
    const string& outDir,
    const string& customName,
    function<void(int64_t, int64_t)> progress,
    bool resume
) {
    auto [host, path, isHttps] = parseUrl(url);
    if (host.empty())
        return { false, 0, "Invalid URL: " + url };

    fs::path outPath = fs::path(outDir) / "SeewoServiceSetup.tmp";
    size_t existing = resume && fs::exists(outPath) ? fs::file_size(outPath) : 0;

    DownloadResult result;
    int redirects = 0;
    string currentHost = host, currentPath = path, currentUrl = url;
    bool currentHttps = isHttps;

    while (redirects <= m_maxRedirects) {
        auto headers = makeHeaders(existing);
        string location;

        auto doDownload = [&](auto& client) {
            ofstream file(outPath, existing ? (ios::binary | ios::app) : (ios::binary | ios::trunc));
            if (!file.is_open()) {
                result.errorMsg = "Unable to create file: " + outPath.string();
                return;
            }

            size_t totalSize = existing, received = existing;
            auto res = client.Get(currentPath.c_str(), headers,
                [&](const httplib::Response& r) {
                    if (r.status == 200) {
                        totalSize = stoull(r.get_header_value("Content-Length", "0"));
                        result.statusCode = 200;
                    }
                    else if (r.status == 206) {
                        auto range = r.get_header_value("Content-Range");
                        auto pos = range.find_last_of('/');
                        if (pos != string::npos)
                            totalSize = stoull(range.substr(pos + 1));
                        result.statusCode = 206;
                    }
                    else if (r.status >= 300 && r.status < 400) {
                        location = r.get_header_value("Location");
                        result.statusCode = r.status;
                        return true; // stop receiving body
                    }
                    else {
                        result.statusCode = r.status;
                        return false;
                    }
                    result.total = totalSize;
                    return true;
                },
                [&](const char* data, size_t len) {
                    file.write(data, len);
                    if (!file) return false;
                    received += len;
                    result.downloaded = received;
                    if (progress) progress(received, totalSize);
                    return true;
                });
            file.close();

            if (!res) {
                result.errorMsg = "Connection error: " + to_string(static_cast<int>(res.error()));
            }
            else if (res->status == 200 || res->status == 206) {
                result.success = (totalSize == 0 || received == totalSize);
                if (!result.success) result.errorMsg = "Incomplete download";
            }
            else if (!location.empty()) {
                // redirect will be handled by outer loop
            }
            else {
                result.errorMsg = "HTTP error: " + to_string(res->status);
            }
            };

        if (currentHttps) {
            httplib::SSLClient cli(currentHost);
            cli.set_connection_timeout(m_timeout);
            cli.set_read_timeout(m_timeout);
            doDownload(cli);
        }
        else {
            httplib::Client cli(currentHost);
            cli.set_connection_timeout(m_timeout);
            cli.set_read_timeout(m_timeout);
            doDownload(cli);
        }

        if (!result.success && !location.empty() && location != currentUrl) {
            // handle redirect
            wcout << L"[*] Redirect (" << ++redirects << L"/" << m_maxRedirects
                << L") -> " << ConvertString<wstring>(location) << L'\n';
            auto [newHost, newPath, newHttps] = parseUrl(location);
            currentHost = newHost;
            currentPath = newPath;
            currentHttps = newHttps;
            currentUrl = location;
            string newFilenameFromUrl = extractFilenameFromRedirectUrl(currentUrl);
            if (!newFilenameFromUrl.empty()) {
                result.filename = newFilenameFromUrl;
                wcout << L"[*] Filename identified from redirect URL: " << ConvertString<wstring>(newFilenameFromUrl) << L'\n';
            }
            existing = 0; // redirect causes fresh download, no resuming
            continue;
        }
        break;
    }

    return result;
}

// Parse URL
HttpDownloader::UrlParts HttpDownloader::parseUrl(const string& url) {
    string_view sv(url);
    bool https = sv.substr(0, 8) == "https://";
    if (https) sv.remove_prefix(8);
    else if (sv.substr(0, 7) == "http://") sv.remove_prefix(7);
    else return {};

    auto pos = sv.find('/');
    if (pos == string_view::npos)
        return { string(sv), "/", https };
    return { string(sv.substr(0, pos)), string(sv.substr(pos)), https };
}

// Get filename from Content-Disposition header
string HttpDownloader::getFilenameFromDisposition(const string& hdr) {
    regex re(R"(filename=["']?([^"' ;]+)["']?)");
    smatch m;
    return regex_search(hdr, m, re) ? m[1].str() : ConvertString<string>(DEFAULT_NAME);
}

// Build request headers
httplib::Headers HttpDownloader::makeHeaders(size_t rangeStart) {
    httplib::Headers h{ {"User-Agent", m_userAgent}, {"Accept", "*/*"}, {"Accept-Encoding", "identity"} };
    if (rangeStart > 0) h.insert({ "Range", "bytes=" + to_string(rangeStart) + "-" });
    return h;
}

// URL decode
string HttpDownloader::urlDecode(const string& encoded) {
    string decoded;
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            int hex;
            istringstream iss(encoded.substr(i + 1, 2));
            iss >> std::hex >> hex;
            decoded += static_cast<char>(hex);
            i += 2;
        }
        else {
            decoded += encoded[i];
        }
    }
    return decoded;
}

// Extract filename from redirect URL
string HttpDownloader::extractFilenameFromRedirectUrl(const string& url) {
    const string paramKey = "response-content-disposition=";
    size_t pos = url.find(paramKey);
    if (pos == string::npos) return {};

    pos += paramKey.length();
    size_t end = url.find('&', pos);
    string encodedValue = url.substr(pos, end - pos);
    string decodedValue = urlDecode(encodedValue);
    return getFilenameFromDisposition(decodedValue);
}

#endif // !HU_DISABLE_INSTALLER