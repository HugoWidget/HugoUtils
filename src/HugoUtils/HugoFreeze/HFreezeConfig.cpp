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
#ifndef HU_DISABLE_FREEZE

#include <cstring>

#include "HugoUtils/HugoFreeze/HFreezeConfig.h"
#include "hashlib/md5.h"
#include "WinUtils/Logger.h"

using namespace WinUtils;
using namespace std;

static Logger logger(L"HFreezeConfig");

bool HFreezeConfig::open() noexcept {
    return m_driver.open();
}

void HFreezeConfig::close() noexcept {
    m_driver.close();
}

bool HFreezeConfig::isDriverOpen() const noexcept {
    return m_driver.isOpen();
}

bool HFreezeConfig::getConfig(HFreezeConfigSource source, HConfigFile& out) const noexcept {
    switch (source) {
    case HFreezeConfigSource::File:
        return m_file.getConfig(out);
    case HFreezeConfigSource::Driver:
        return m_driver.getConfig(out);
    default:
        return false;
    }
}

bool HFreezeConfig::setConfig(HFreezeConfigSource source, const HConfigFile& cfg) noexcept {
    switch (source) {
    case HFreezeConfigSource::File:
        return m_file.setConfig(cfg);
    case HFreezeConfigSource::Driver:
        return m_driver.setConfig(cfg);
    default:
        return false;
    }
}

bool HFreezeConfig::getFileConfig(HConfigFile& out) const noexcept {
    return m_file.getConfig(out);
}

bool HFreezeConfig::setFileConfig(const HConfigFile& cfg) const noexcept {
    return m_file.setConfig(cfg);
}

bool HFreezeConfig::getDriverConfig(HConfigFile& out) const noexcept {
    return m_driver.getConfig(out);
}

bool HFreezeConfig::setDriverConfig(const HConfigFile& cfg) noexcept {
    return m_driver.setConfig(cfg);
}

bool HFreezeConfig::getRuntimeStatus(DriverRuntimeStatus& out) const noexcept {
    return m_driver.getRuntimeStatus(out);
}

void HFreezeConfig::setConfigPath(const std::wstring& path) noexcept {
    m_file.setConfigPath(path);
}

const std::wstring& HFreezeConfig::getConfigPath() const noexcept {
    return m_file.getConfigPath();
}

bool HFreezeConfig::applyFreezeMask(uint32_t volMask, bool enable) noexcept {
    // 1. Read the current file config.
    HConfigFile cfg;
    if (!m_file.getConfig(cfg)) {
        return false;
    }

    // 2. Build the modified blob (field writes + fresh MD5 digest).
    HConfigFile updated = BuildFreezeConfig(cfg, volMask, enable);

    logger.DLog(LogLevel::Debug, L"Config buffer ready");
    logger.DLog(LogLevel::Debug,
        FrzHexDump(reinterpret_cast<const unsigned char*>(&updated), FRZ_CONFIG_VALID_LEN));

    // 3. Write to the driver, then write the same blob back to the config file.
    //    Only writing to the driver actually applies the freeze state change,
    //    so nothing is written unless the driver is open (mirrors the original).
    if (isDriverOpen()) {
        if (!m_driver.setConfig(updated)) {
            return false;
        }
        if (!m_file.setConfig(updated)) {
            return false;
        }
    }
    logger.DLog(LogLevel::Info, L"Config modified, reboot to apply");
    return true;
}

HConfigFile HFreezeConfig::BuildFreezeConfig(const HConfigFile& original,
    uint32_t volMask, bool enable) noexcept {
    unsigned char buffer[FRZ_CONFIG_SIZE] = { 0 };
    original.toBuffer(buffer);

    // Modify the config fields (all offsets come from HFreezeDef.h).
    *reinterpret_cast<uint32_t*>(buffer + FRZ_OFF_NEXT_MASK) = volMask;
    buffer[FRZ_OFF_FLAG1] = FRZ_FLAG1_WRITE;
    *reinterpret_cast<uint16_t*>(buffer + FRZ_OFF_STATUS) =
        enable ? FRZ_STATUS_FROZEN : FRZ_STATUS_UNFROZEN;
    buffer[FRZ_OFF_FLAG2] = FRZ_FLAG2_WRITE;
    *reinterpret_cast<uint32_t*>(buffer + FRZ_OFF_VOL_MASK_COPY) = volMask;

    // Recalculate the MD5 digest over [FRZ_OFF_NEXT_MASK, end).
    MD5 md5;
    unsigned char digest[MD5::HashBytes] = { 0 };
    md5.add(buffer + FRZ_OFF_NEXT_MASK, FRZ_CONFIG_SIZE - FRZ_OFF_NEXT_MASK);
    md5.getHash(digest);
    memcpy(buffer, digest, FRZ_CONFIG_MD5_SIZE);

    return HConfigFile(buffer);
}

#endif // !HU_DISABLE_FREEZE
