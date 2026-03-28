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
#include "WinUtils/WinPch.h"

#include <Windows.h>

#include <string>
#include <optional>
#include <filesystem>

#include "WinUtils/WinUtils.h"
class HugoInfo {
public:
    std::optional<std::wstring> getHugoVersion();
    std::optional<std::wstring> getHugoFolder();
    std::optional<std::wstring> getHugoProtectDriverFolder();
    std::optional<std::wstring> getHugoProtectDriverPath();
private:
    static std::optional<std::filesystem::path> FindSpecificDir(
        const std::filesystem::path& baseDir,
        std::wstring_view prefix
    );
};