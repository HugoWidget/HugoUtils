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

// Private helper: get SysWOW64 folder (32-bit system directory)
optional<fs::path> HInfo::GetSysWow64Path()
{
    wchar_t sysWow64[MAX_PATH] = { 0 };
    // GetSystemWow64DirectoryW returns the path to the 32-bit system directory
    if (GetSystemWow64DirectoryW(sysWow64, MAX_PATH) != 0) {
        return fs::path(sysWow64);
    }
    logger.Error(L"Failed to retrieve SysWOW64 directory");
    return nullopt;
}

// Private helper: find first directory matching prefix
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


// Private helper: find all directories matching prefix
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


// Private helper: extract version from directory name (e.g., "SeewoService_1.2.3.4")
optional<wstring> HInfo::ExtractVersionFromDirName(const fs::path& dirPath)
{
    wstring dirName = dirPath.filename().wstring();
    size_t underscorePos = dirName.find(L'_');
    if (underscorePos != wstring::npos && underscorePos + 1 < dirName.size()) {
        return dirName.substr(underscorePos + 1);
    }
    return nullopt;
}


// Private helper: get a known folder path as a filesystem::path
optional<fs::path> HInfo::GetKnownFolderPath(int csidl)
{
    wchar_t path[MAX_PATH] = { 0 };
    if (SUCCEEDED(SHGetFolderPathW(nullptr, csidl, nullptr, 0, path))) {
        return fs::path(path);
    }
    return nullopt;
}


// Private helper: read MachineId from registry (64-bit view preferred)
optional<string> HInfo::ReadRegistryMachineId()
{
    char buffer[128] = { 0 };
    DWORD size = sizeof(buffer);
    HKEY hKey;

    // Try 64-bit view first
    LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Microsoft\\SQMClient",
        0, KEY_READ | KEY_WOW64_64KEY, &hKey);
    if (result != ERROR_SUCCESS) {
        // Fall back to default (32-bit) view
        result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
            "SOFTWARE\\Microsoft\\SQMClient",
            0, KEY_READ, &hKey);
    }
    if (result != ERROR_SUCCESS) {
        return nullopt;
    }

    result = RegQueryValueExA(hKey, "MachineId", nullptr, nullptr,
        reinterpret_cast<LPBYTE>(buffer), &size);
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS) {
        return nullopt;
    }
    return string(buffer);
}


// Public: get SeewoService version string
optional<wstring> HInfo::getHugoVersion()
{
    auto foundDir = FindFirstDirectory(SEEWO_SERVICE_BASE, SEEWO_SERVICE_PREFIX);
    if (!foundDir) {
        logger.Warn(L"SeewoService directory not found");
        return nullopt;
    }
    return ExtractVersionFromDirName(*foundDir);
}


// Public: get SeewoService base directory
optional<fs::path> HInfo::getHugoFolder()
{
    auto foundDir = FindFirstDirectory(SEEWO_SERVICE_BASE, SEEWO_SERVICE_PREFIX);
    if (!foundDir) {
        logger.Warn(L"SeewoService directory not found");
        return nullopt;
    }
    return *foundDir;
}


// Public: get driver service folder
optional<fs::path> HInfo::getHugoProtectDriverFolder()
{
    auto hugoFolder = getHugoFolder();
    if (!hugoFolder) return nullopt;

    fs::path driverFolder = *hugoFolder / L"SeewoDriverService";
    if (!fs::is_directory(driverFolder)) return nullopt;

    return driverFolder;
}


// Public: get full path to DriverService.exe
optional<fs::path> HInfo::getHugoProtectDriverPath()
{
    auto driverFolder = getHugoProtectDriverFolder();
    if (!driverFolder) return nullopt;

    fs::path driverPath = *driverFolder / L"DriverService.exe";
    if (!fs::is_regular_file(driverPath)) return nullopt;

    return driverPath;
}


// Public: get all Easiupdate3 directories
vector<fs::path> HInfo::getHugoUpdateFolder()
{
    return FindAllDirectories(EASIUPDATE_BASE, EASIUPDATE_PREFIX);
}


// Public: get Windows MachineId
optional<string> HInfo::GetMachineId()
{
    return ReadRegistryMachineId();
}


// Public: SeewoCore.ini path (common appdata)
optional<fs::path> HInfo::GetSeewoCoreIniPath()
{
    auto base = GetKnownFolderPath(CSIDL_COMMON_APPDATA);
    if (base) {
        return *base / L"Seewo\\SeewoCore\\SeewoCore.ini";
    }
    return nullopt;
}


// Public: SeewoLockConfig.ini path (user appdata)
optional<fs::path> HInfo::GetLockConfigIniPath()
{
    auto base = GetKnownFolderPath(CSIDL_APPDATA);
    if (base) {
        return *base / L"seewo\\SeewoAbility\\SeewoLockConfig.ini";
    }
    return nullopt;
}


// Public: .lock_backup path (user appdata)
optional<fs::path> HInfo::GetLockConfigIniPath2()
{
    auto base = GetKnownFolderPath(CSIDL_APPDATA);
    if (base) {
        return *base / L"seewo\\SeewoAbility\\.lock_backup";
    }
    return nullopt;
}

// Public: C:\Windows\SysWOW64\config\systemprofile\AppData\Roaming\school.ini
optional<fs::path> HInfo::GetSeewoSchoolFilePath()
{
    auto sysWow64 = GetSysWow64Path();
    if (!sysWow64) return nullopt;

    fs::path fullPath = *sysWow64 / L"config\\systemprofile\\AppData\\Roaming\\seewo\\school.ini";
    return fullPath;
}

#endif // !HU_DISABLE_INFO