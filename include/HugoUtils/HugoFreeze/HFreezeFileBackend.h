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
#ifndef HU_DISABLE_FREEZE

#include <string>

#include "HugoUtils/HugoFreeze/HFreezeDef.h"

 // ---------------------------------------------------------------------------
 // HFreezeFileBackend - pure VolumeInfo.config file I/O.
 // Reads/writes the same 1024-byte blob as the driver, using HConfigFile.
 // ---------------------------------------------------------------------------
class HFreezeFileBackend {
public:
    HFreezeFileBackend() noexcept;

    void setConfigPath(const std::wstring& path) noexcept;
    const std::wstring& getConfigPath() const noexcept;

    bool getConfig(HConfigFile& out) const noexcept;
    bool setConfig(const HConfigFile& cfg) const noexcept;

    // Raw fixed-size blob access, used to export/import a configuration file.
    // Both are thin wrappers over getConfig/setConfig and require at least
    // FRZ_CONFIG_SIZE bytes; only the first FRZ_CONFIG_SIZE bytes are used.
    bool readBlob(unsigned char* data, size_t size) const noexcept;
    bool writeBlob(const unsigned char* data, size_t size) const noexcept;

private:
    std::wstring m_configPath;
};

#endif // !HU_DISABLE_FREEZE
