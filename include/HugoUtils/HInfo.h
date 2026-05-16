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
#include "HugoUtilsDef.h"
#ifndef HU_DISABLE_INFO
#include <Windows.h>
#include <string>
#include <optional>
#include <vector>
#include <filesystem>

class HInfo {
public:
    // Returns the extracted version string (e.g., "1.2.3.4") from the SeewoService directory name
    std::optional<std::wstring> getHugoVersion();

    // Base directory of the SeewoService installation
    std::optional<std::filesystem::path> getHugoFolder();

    // Folder containing the driver service
    std::optional<std::filesystem::path> getHugoProtectDriverFolder();

    // Full path to DriverService.exe
    std::optional<std::filesystem::path> getHugoProtectDriverPath();

    // All update directories under Easiupdate3
    std::vector<std::filesystem::path> getHugoUpdateFolder();

    // Retrieves the Windows MachineId from the registry
    static std::optional<std::string> GetMachineId();

    // Path to SeewoCore.ini (common appdata)
    static std::optional<std::filesystem::path> GetSeewoCoreIniPath();

    // Path to SeewoLockConfig.ini (user appdata)
    static std::optional<std::filesystem::path> GetLockConfigIniPath();

    // Path to .lock_backup (user appdata)
    static std::optional<std::filesystem::path> GetLockConfigIniPath2();

private:
    static inline const std::filesystem::path SEEWO_SERVICE_BASE = L"C:\\Program Files (x86)\\Seewo\\SeewoService";
    static inline const std::wstring_view SEEWO_SERVICE_PREFIX = L"SeewoService_";
    static inline const std::filesystem::path EASIUPDATE_BASE = L"C:\\ProgramData\\Seewo\\Easiupdate3";
    static inline const std::wstring_view EASIUPDATE_PREFIX = L"Easiupdate3";

    // Find the first directory whose name starts with the given prefix
    static std::optional<std::filesystem::path> FindFirstDirectory(
        const std::filesystem::path& baseDir,
        std::wstring_view prefix
    );

    // Find all directories whose name starts with the given prefix
    static std::vector<std::filesystem::path> FindAllDirectories(
        const std::filesystem::path& baseDir,
        std::wstring_view prefix
    );

    // Extract the version part (after the first underscore) from a directory name
    static std::optional<std::wstring> ExtractVersionFromDirName(const std::filesystem::path& dirPath);

    // Helper to obtain known folder paths via SHGetFolderPathW
    static std::optional<std::filesystem::path> GetKnownFolderPath(int csidl);

    // Helper to read the MachineId from registry (tries 64-bit view first)
    static std::optional<std::string> ReadRegistryMachineId();
};
#endif // !HU_DISABLE_INFO