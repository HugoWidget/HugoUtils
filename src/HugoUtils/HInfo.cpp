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
#ifndef HU_DISABLE_INFO

#include <iostream>
#include <string_view>
#include <optional>
#include <shlobj.h>

#include "HugoUtils/HInfo.h"
#include "WinUtils/StrConvert.h"
#include "WinUtils/Logger.h"
using namespace std;
using namespace WinUtils;
namespace fs = std::filesystem;

static Logger logger(L"HInfo");

optional<fs::path> HInfo::FindFirstDirectory(
    const fs::path& baseDir,
    wstring_view prefix)
{
    try {
        for (const auto& entry : fs::directory_iterator(baseDir)) {
            if (entry.is_directory()) {
                const auto& dirName = entry.path().filename().wstring();
                if (dirName.compare(0, prefix.size(), prefix) == 0) {
                    return entry.path();
                }
            }
        }
    }
    catch (const fs::filesystem_error& e) {
        logger.Error(format(L"Failed to enumerate directory {}: {}",
            baseDir.wstring(), ConvertString<wstring>(e.what())));
    }
    return nullopt;
}

vector<fs::path> HInfo::FindAllDirectories(
    const fs::path& baseDir,
    wstring_view prefix)
{
    vector<fs::path> result;
    try {
        for (const auto& entry : fs::directory_iterator(baseDir)) {
            if (entry.is_directory()) {
                const auto& dirName = entry.path().filename().wstring();
                if (dirName.compare(0, prefix.size(), prefix) == 0) {
                    result.push_back(entry.path());
                }
            }
        }
    }
    catch (const fs::filesystem_error& e) {
        logger.Error(format(L"Failed to enumerate directory {}: {}",
            baseDir.wstring(), ConvertString<wstring>(e.what())));
    }
    return result;
}

optional<wstring> HInfo::ExtractVersionFromDirName(const fs::path& dirPath)
{
    wstring dirName = dirPath.filename().wstring();
    size_t underscorePos = dirName.find(L'_');
    if (underscorePos != wstring::npos && underscorePos + 1 < dirName.size()) {
        return dirName.substr(underscorePos + 1);
    }
    return nullopt;
}

optional<wstring> HInfo::getHugoVersion()
{
    auto foundDir = FindFirstDirectory(SEEWO_SERVICE_BASE, SEEWO_SERVICE_PREFIX);
    if (!foundDir) {
        logger.Warn(L"SeewoService directory not found");
        return nullopt;
    }
    return ExtractVersionFromDirName(*foundDir);
}

optional<wstring> HInfo::getHugoFolder()
{
    auto foundDir = FindFirstDirectory(SEEWO_SERVICE_BASE, SEEWO_SERVICE_PREFIX);
    if (!foundDir) {
        logger.Warn(L"SeewoService directory not found");
        return nullopt;
    }
    return foundDir->wstring() + L'\\';
}

optional<wstring> HInfo::getHugoProtectDriverFolder()
{
    auto hugoFolder = getHugoFolder();
    if (!hugoFolder) return nullopt;

    fs::path driverFolder = fs::path(*hugoFolder) / L"SeewoDriverService";
    if (!fs::is_directory(driverFolder)) return nullopt;

    return driverFolder.wstring() + L'\\';
}

optional<wstring> HInfo::getHugoProtectDriverPath()
{
    auto driverFolder = getHugoProtectDriverFolder();
    if (!driverFolder) return nullopt;

    fs::path driverPath = fs::path(*driverFolder) / L"DriverService.exe";
    if (!fs::is_regular_file(driverPath)) return nullopt;

    return driverPath.wstring();
}

vector<wstring> HInfo::getHugoUpdateFolder()
{
    auto dirs = FindAllDirectories(EASIUPDATE_BASE, EASIUPDATE_PREFIX);
    vector<wstring> result;
    result.reserve(dirs.size());
    for (const auto& d : dirs) {
        result.push_back(d.wstring() + L'\\');
    }
    return result;
}

std::optional<std::string> HInfo::GetMachineId() {
    char buffer[128] = { 0 };
    DWORD buffer_size = sizeof(buffer);
    HKEY hKey;

    LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\SQMClient",
        0, KEY_READ | KEY_WOW64_64KEY, &hKey);
    if (result != ERROR_SUCCESS) {
        result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\SQMClient",
            0, KEY_READ, &hKey);
    }
    if (result != ERROR_SUCCESS) {
        return std::nullopt;
    }

    result = RegQueryValueExA(hKey, "MachineId", nullptr, nullptr,
        reinterpret_cast<LPBYTE>(buffer), &buffer_size);
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) {
        return std::nullopt;
    }
    return std::string(buffer);
}

std::optional<std::string> HInfo::GetSeewoCoreIniPath() {
    char path[MAX_PATH] = { 0 };
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_COMMON_APPDATA, nullptr, 0, path))) {
        strcat_s(path, MAX_PATH, "\\Seewo\\SeewoCore\\SeewoCore.ini");
        return std::string(path);
    }
    return std::nullopt;
}

std::optional<std::string> HInfo::GetLockConfigIniPath() {
    char path[MAX_PATH] = { 0 };
    if (SUCCEEDED(SHGetFolderPathA(nullptr, CSIDL_APPDATA, nullptr, 0, path))) {
        strcat_s(path, MAX_PATH, "\\seewo\\SeewoAbility\\SeewoLockConfig.ini");
        return std::string(path);
    }
    return std::nullopt;
}
#endif // !HU_DISABLE_INFO