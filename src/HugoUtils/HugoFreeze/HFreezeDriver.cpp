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
#ifndef HU_DISABLE_FREEZE_DRIVER

#include <cctype>
#include <format>

#include "HugoUtils/HugoFreeze/HFreezeDriver.h"
#include "HugoUtils/HugoUtils.h"
#include "WinUtils/Logger.h"
#include "WinUtils/WinUtils.h"

using namespace WinUtils;
using namespace std;

static Logger logger(L"HFreezeDriver");

FreezeResult HFreezeDriver::Init() noexcept {
    if (IsInitialized()) return FreezeResult(FrzOR::Success);

    if (!m_config.open()) {
        DWORD err = GetLastError();
        logger.DLog(LogLevel::Error, format(L"Open driver fail, err: {}", err));
        return FreezeResult(FrzOR::InitFailed).setError(err).setErrMsg(GetWindowsErrorMsg());
    }
    return FreezeResult(FrzOR::Success);
}

void HFreezeDriver::Cleanup() noexcept {
    m_config.close();
}

bool HFreezeDriver::IsInitialized() const noexcept {
    return m_config.isDriverOpen();
}

bool HFreezeDriver::QueryDriverStatus(DriverRuntimeStatus& runtimeOut) const noexcept {
    if (!IsInitialized()) return false;

    if (!m_config.getRuntimeStatus(runtimeOut)) {
        runtimeOut.querySuccess = false;
        logger.DLog(LogLevel::Error, L"Query runtime status fail");
        return false;
    }
    logger.DLog(LogLevel::Debug, L"Query runtime status success");
    return true;
}

FreezeResult HFreezeDriver::GetBootFreezeState() const noexcept {
    if (!IsInitialized()) {
        return FreezeResult(FrzOR::NotInitialized, L"Driver not initialized");
    }

    DriverRuntimeStatus runtime;
    QueryDriverStatus(runtime);

    HConfigFile cfg;
    if (!m_config.getDriverConfig(cfg)) {
        return FreezeResult(FrzOR::DriverError, L"Failed to query driver boot config");
    }
    return ParseConfig(cfg, runtime);
}

FreezeResult HFreezeDriver::GetFileFreezeState() const noexcept {
    DriverRuntimeStatus runtime;
    QueryDriverStatus(runtime);

    HConfigFile cfg;
    if (!m_config.getFileConfig(cfg)) {
        return FreezeResult(FrzOR::Failed, L"Failed to read config file");
    }
    return ParseConfig(cfg, runtime);
}

FreezeResult HFreezeDriver::GetFreezeState() const noexcept {
    FreezeResult resBoot = GetBootFreezeState();
    FreezeResult resFile = GetFileFreezeState();

    FreezeResult res = resBoot;
    for (const auto& disk : resBoot.diskInfos) {
        const auto fileIt = resFile.diskInfos.find(disk.first);
        DriveFreezeState stateBoot = resBoot.diskInfos[disk.first].state;
        DriveFreezeState stateFile = (fileIt != resFile.diskInfos.end())
            ? fileIt->second.state
            : DriveFreezeState::Unknown;

        bool frozenBoot = (stateBoot == DriveFreezeState::PendingFreeze ||
            stateBoot == DriveFreezeState::Frozen);
        bool frozenFile = (stateFile == DriveFreezeState::PendingFreeze ||
            stateFile == DriveFreezeState::Frozen);

        auto& info = res.diskInfos[disk.first];
        if (frozenBoot == frozenFile) {
            info.state = stateBoot;
        }
        else if (frozenBoot) {
            info.state = DriveFreezeState::PendingUnfreeze;
        }
        else {
            info.state = DriveFreezeState::PendingFreeze;
        }
    }
    return res;
}

FreezeResult HFreezeDriver::TryProtect(const std::wstring& driveLetters) const noexcept {
    return FreezeResult(FrzOR::NotSupported);
}

FreezeResult HFreezeDriver::SetFreezeState(const std::wstring& driveLetters) noexcept {
    if (driveLetters.empty()) {
        logger.DLog(LogLevel::Info, L"Unfreeze all drives");
        bool res = m_config.applyFreezeMask(0, false);
        return FreezeResult(res ? FrzOR::Success : FrzOR::Failed);
    }

    uint32_t volMask = CalculateVolumeMask(driveLetters);
    if (volMask == FRZ_VOLUME_MASK_INVALID) {
        logger.DLog(LogLevel::Error,
            format(L"Invalid drive letters: {}", driveLetters));
        return FreezeResult(FrzOR::Failed).setErrMsg(L"Invalid drive letters");
    }

    logger.DLog(LogLevel::Info,
        format(L"Freeze drives: {} (mask=0x{:08X})", driveLetters, volMask));
    bool res = m_config.applyFreezeMask(volMask, true);
    return FreezeResult(res ? FrzOR::Success : FrzOR::Failed);
}

std::wstring HFreezeDriver::GetLastErrorMsg() const noexcept {
    return L"";
}

DWORD HFreezeDriver::GetLastErrorCode() const noexcept {
    return 0;
}

FreezeResult HFreezeDriver::ParseConfig(const HConfigFile& cfg,
    const DriverRuntimeStatus& runtime) const noexcept {
    unsigned char buf[FRZ_CONFIG_SIZE] = { 0 };
    cfg.toBuffer(buf);
    const size_t len = FRZ_CONFIG_SIZE;

    auto readU32 = [&](size_t offset) -> uint32_t {
        if (offset + 4 > len) return 0;
        return static_cast<uint32_t>(buf[offset]) |
            (static_cast<uint32_t>(buf[offset + 1]) << 8) |
            (static_cast<uint32_t>(buf[offset + 2]) << 16) |
            (static_cast<uint32_t>(buf[offset + 3]) << 24);
        };
    auto readU16 = [&](size_t offset) -> uint16_t {
        if (offset + 2 > len) return 0;
        return static_cast<uint16_t>(buf[offset]) |
            (static_cast<uint16_t>(buf[offset + 1]) << 8);
        };
    auto readU8 = [&](size_t offset) -> uint8_t {
        if (offset >= len) return 0;
        return buf[offset];
        };

    // Read configuration fields.
    uint32_t next_mask = readU32(FRZ_OFF_NEXT_MASK);
    uint8_t flag1 = readU8(FRZ_OFF_FLAG1);
    uint16_t status = readU16(FRZ_OFF_STATUS);
    uint8_t flag2 = readU8(FRZ_OFF_FLAG2);
    uint32_t vol_mask_copy = readU32(FRZ_OFF_VOL_MASK_COPY);

    // Device ID (ASCII, starting at FRZ_OFF_DEVICE_ID until NUL).
    string device_id;
    for (size_t i = FRZ_OFF_DEVICE_ID; i < len && buf[i] != 0; ++i) {
        if (isprint(buf[i])) device_id.push_back(static_cast<char>(buf[i]));
    }
    // School code (4 bytes ASCII).
    string school_code;
    for (size_t i = FRZ_OFF_SCHOOL_CODE;
        i < FRZ_OFF_SCHOOL_CODE + FRZ_SCHOOL_CODE_LEN && i < len; ++i) {
        if (isprint(buf[i])) school_code.push_back(static_cast<char>(buf[i]));
    }

    // MD5 (first 16 bytes).
    string md5_str;
    for (size_t i = 0; i < FRZ_CONFIG_MD5_SIZE; ++i) {
        md5_str += format("{:02X}", buf[i]);
    }

    // Determine the current freeze state from the runtime active flag and the
    // configured volume mask.
    bool is_active = (runtime.querySuccess && runtime.activeFlag != 0);
    map<wchar_t, DiskInfo> disk_infos;

    string drives = HugoUtils::GetLogicalDrives();
    for (char letter : drives) {
        int bit = letter - 'A';
        if (bit < 0 || bit >= FRZ_MAX_DRIVE_LETTERS) continue;
        bool frozen_in_config = (next_mask & (1u << bit)) != 0;
        DriveFreezeState state = DriveFreezeState::Unfrozen;
        if (is_active && frozen_in_config) {
            state = DriveFreezeState::Frozen;
        }
        else if (!is_active && frozen_in_config) {
            state = DriveFreezeState::PendingFreeze;
        }
        disk_infos[letter] = { state };
    }

    FreezeResult result(FrzOR::Success, L"Get freeze state completed");
    result.setDiskInfos(disk_infos);
    result.extra.md5 = md5_str;
    result.extra.next_mask = next_mask;
    result.extra.flag1 = flag1;
    result.extra.status = status;
    result.extra.flag2 = flag2;
    result.extra.vol_mask_copy = vol_mask_copy;
    result.extra.device_id = device_id;
    result.extra.school_code = school_code;

    return result;
}

#endif // !HU_DISABLE_FREEZE_DRIVER
