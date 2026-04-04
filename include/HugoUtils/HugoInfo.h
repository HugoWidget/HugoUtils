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

class HugoInfo {
public:
    std::optional<std::wstring> getHugoVersion();
    std::optional<std::wstring> getHugoFolder();
    std::optional<std::wstring> getHugoProtectDriverFolder();
    std::optional<std::wstring> getHugoProtectDriverPath();
    std::vector<std::wstring> getHugoUpdateFolder();

private:
    static inline const std::filesystem::path SEEWO_SERVICE_BASE = L"C:\\Program Files (x86)\\Seewo\\SeewoService";
    static inline const std::wstring_view SEEWO_SERVICE_PREFIX = L"SeewoService_";
    static inline const std::filesystem::path EASIUPDATE_BASE = L"C:\\ProgramData\\Seewo\\Easiupdate3";
    static inline const std::wstring_view EASIUPDATE_PREFIX = L"Easiupdate3";

    static std::optional<std::filesystem::path> FindFirstDirectory(
        const std::filesystem::path& baseDir,
        std::wstring_view prefix
    );

    static std::vector<std::filesystem::path> FindAllDirectories(
        const std::filesystem::path& baseDir,
        std::wstring_view prefix
    );
    static std::optional<std::wstring> ExtractVersionFromDirName(const std::filesystem::path& dirPath);
};
#endif // !HU_DISABLE_INFO