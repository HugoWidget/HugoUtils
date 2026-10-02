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

#include <cstdint>
#include <string>

#include "HugoUtils/HugoFreeze/HFreezeDef.h"
#include "HugoUtils/HugoFreeze/HFreezeFileBackend.h"
#include "HugoUtils/HugoFreeze/HFreezeDriverBackend.h"

 // Selects which backend the unified get/set operates on.
enum class HFreezeConfigSource : uint8_t {
    File = 0,    // VolumeInfo.config on disk
    Driver = 1   // SWFreeze boot configuration
};

// ---------------------------------------------------------------------------
// HFreezeConfig - unified configuration access. Wraps HFreezeFileBackend and
// HFreezeDriverBackend and exposes a single get/set surface parameterised by
// HConfigFile. It also owns the freeze-mask build/apply logic.
// ---------------------------------------------------------------------------
class HFreezeConfig {
public:
    HFreezeConfig() noexcept = default;
    ~HFreezeConfig() noexcept = default;

    HFreezeConfig(const HFreezeConfig&) = delete;
    HFreezeConfig& operator=(const HFreezeConfig&) = delete;

    // ---- driver lifecycle ----
    bool open() noexcept;
    void close() noexcept;
    bool isDriverOpen() const noexcept;

    // ---- unified get / set ----
    bool getConfig(HFreezeConfigSource source, HConfigFile& out) const noexcept;
    bool setConfig(HFreezeConfigSource source, const HConfigFile& cfg) noexcept;

    // ---- named helpers ----
    bool getFileConfig(HConfigFile& out) const noexcept;
    bool setFileConfig(const HConfigFile& cfg) const noexcept;
    bool getDriverConfig(HConfigFile& out) const noexcept;
    bool setDriverConfig(const HConfigFile& cfg) noexcept;

    bool getRuntimeStatus(DriverRuntimeStatus& out) const noexcept;

    // ---- config file path ----
    void setConfigPath(const std::wstring& path) noexcept;
    const std::wstring& getConfigPath() const noexcept;

    // Read the current file config, apply a new target volume mask, recompute
    // the MD5 digest, then write it back to the driver (when open) and to the
    // config file.
    bool applyFreezeMask(uint32_t volMask, bool enable) noexcept;

    // Pure helper: build a new configuration blob from `original` with a new
    // target volume mask and freeze state, recomputing the MD5 digest.
    static HConfigFile BuildFreezeConfig(const HConfigFile& original,
        uint32_t volMask, bool enable) noexcept;

private:
    HFreezeFileBackend   m_file;
    HFreezeDriverBackend m_driver;
};

#endif // !HU_DISABLE_FREEZE
