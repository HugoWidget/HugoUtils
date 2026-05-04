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

#include <fstream>
#include <format>
#include <algorithm>

#include "HugoUtils/HFreezeDriver.h"
#include "WinUtils/Logger.h"
#include "hashlib/md5.h"
#include "HugoUtils/HugoUtils.h"
#include <WinUtils/StrConvert.h>
using namespace WinUtils;
using namespace HugoUtils;
using namespace std;
static Logger logger(L"HFreezeDriver");

HFreezeDriver& HFreezeDriver::Instance() noexcept {
    static HFreezeDriver instance;
    return instance;
}

FreezeResult HFreezeDriver::Init() noexcept {
    if (IsInitialized())return FreezeResult(FrzOR::Success);
    QueryResult result;
    m_hDriver = OpenDriver();
    if (m_hDriver == INVALID_HANDLE_VALUE) {
        result.lastError = GetLastError();
        result.driverOpenSuccess = false;
        logger.DLog(LogLevel::Error, format(L"Open driver fail, err: {}", result.lastError));
        m_hDriver = NULL;
    }
    result.driverOpenSuccess = true;
    return FreezeResult(result.driverOpenSuccess ? FrzOR::Success : FrzOR::InitFailed).setError(result.lastError);
}

void HFreezeDriver::Cleanup() noexcept {
    if (IsInitialized())CloseHandle(m_hDriver);
}

bool HFreezeDriver::IsInitialized() const noexcept {
    return m_hDriver != NULL && m_hDriver != INVALID_HANDLE_VALUE;
}

QueryResult HFreezeDriver::QueryDriverStatus()const noexcept {
    QueryResult result;
    result.driverOpenSuccess = false;
    if (!IsInitialized())return result;
    result.driverOpenSuccess = true;

    // Query runtime status
    unsigned char buf[CONFIG_SIZE] = { 0 };
    DWORD bytesReturned = 0;
    if (DeviceIoControl(m_hDriver, IOCTL_QUERY_RUNTIME, nullptr, 0, buf, CONFIG_SIZE, &bytesReturned, nullptr)) {
        result.runtimeStatus.querySuccess = true;
        result.runtimeStatus.activeFlag = *reinterpret_cast<uint32_t*>(buf + OFF_RT_ACTIVE);
        result.runtimeStatus.ptr1 = *reinterpret_cast<uint64_t*>(buf + OFF_RT_PTR1);

        const char* logStr = reinterpret_cast<char*>(buf + OFF_RT_LOG);
        buf[CONFIG_SIZE - 1] = 0;
        result.runtimeStatus.logStr = wstring(logStr, logStr + strlen(logStr));

        logger.DLog(LogLevel::Debug, L"Query runtime status success");
    }
    else {
        result.runtimeStatus.querySuccess = false;
        logger.DLog(LogLevel::Error, L"Query runtime status fail");
    }

    // Query boot configuration
    memset(buf, 0, CONFIG_SIZE);
    if (DeviceIoControl(m_hDriver, IOCTL_READ_MEM_CONF, nullptr, 0, buf, CONFIG_SIZE, &bytesReturned, nullptr)) {
        result.bootConfig.querySuccess = true;
        memcpy(result.bootConfig.buffer, buf, CONFIG_SIZE);
        result.bootConfig.validLen = 0x90;
        logger.DLog(LogLevel::Debug, L"Query boot config success");
        HexDump(buf, 0x90);
    }
    else {
        result.bootConfig.querySuccess = false;
        logger.DLog(LogLevel::Error, L"Query boot config fail");
    }
    return result;
}

// Core functionalities
inline FreezeResult HFreezeDriver::GetBootFreezeState() const noexcept {
    // 1. Check if driver is initialized
    if (!IsInitialized()) {
        return FreezeResult(FrzOR::NotInitialized, L"Driver not initialized");
    }

    // 2. Query driver status (runtime + boot config)
    QueryResult query = QueryDriverStatus();
    if (!query.driverOpenSuccess) {
        return FreezeResult(FrzOR::DriverError, L"Failed to communicate with driver")
            .setError(query.lastError);
    }

    const unsigned char* buf = query.bootConfig.buffer;
    const int len = query.bootConfig.validLen;
    if (!query.bootConfig.querySuccess || len < 0x90) {
        return FreezeResult(FrzOR::Failed, L"Boot config query failed or invalid length");
    }
    return ParseFreezeBuffer(query);
}

FreezeResult HFreezeDriver::GetFileFreezeState() const noexcept
{
    QueryResult  res = QueryDriverStatus();;
    string buffer = GetCurrentConfig();
    memcpy(res.bootConfig.buffer, buffer.c_str(), buffer.size());
    res.bootConfig.validLen = buffer.size();
    res.bootConfig.querySuccess = true;

    return ParseFreezeBuffer(res);
}


FreezeResult HFreezeDriver::GetFreezeState() const noexcept
{
    auto resBoot = GetBootFreezeState();
    auto resFile = GetFileFreezeState();
    FreezeResult res = resBoot;
    for (const auto disk : resBoot.diskInfos) {
        DriveFreezeState stateBoot = resBoot.diskInfos[disk.first].state;
        DriveFreezeState stateFile = resFile.diskInfos[disk.first].state;
        bool frozenBoot = (stateBoot == DriveFreezeState::PendingFreeze || stateBoot == DriveFreezeState::Frozen);
        bool frozenFile = (stateFile == DriveFreezeState::PendingFreeze || stateFile == DriveFreezeState::Frozen);
        if (frozenBoot == frozenFile) {
            auto& info = res.diskInfos[disk.first];
            info.state = stateBoot;
        }
        else {
            if (frozenBoot) {
                auto& info = res.diskInfos[disk.first];
                info.state = DriveFreezeState::PendingUnfreeze;
            }
            else {
                auto& info = res.diskInfos[disk.first];
                info.state = DriveFreezeState::PendingFreeze;
            }
        }

    }
    return res;
}



FreezeResult HFreezeDriver::TryProtect(const wstring& driveLetters) const noexcept {
    return FreezeResult(FrzOR::NotSupported);
}

HANDLE HFreezeDriver::OpenDriver()
{
    HANDLE hDevice = CreateFileW(
        DRIVER_DEVICE_PATH,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        0,
        nullptr
    );
    return hDevice;
}

bool HFreezeDriver::ModifyConfig(uint32_t newVol, bool enable) noexcept {
    // Read Config
    ifstream configFile(SWF_CONFIG_PATH, ios::binary | ios::in);
    if (!configFile.is_open()) {
        DWORD err = GetLastError();
        logger.DLog(LogLevel::Error, format(L"Open config fail, err: {}", err));
        return false;
    }

    unsigned char buffer[CONFIG_SIZE] = { 0 };
    configFile.read(reinterpret_cast<char*>(buffer), CONFIG_SIZE);
    if (!configFile || configFile.gcount() != CONFIG_SIZE) {
        logger.DLog(LogLevel::Error, L"Invalid config size");
        configFile.close();
        return false;
    }
    configFile.close();

    // Modify Config
    *reinterpret_cast<uint32_t*>(buffer + SWF_OFFSET_UINT32_NEXT_MASK) = newVol;
    buffer[SWF_OFFSET_UINT8_FLAG1] = 0x02;
    *reinterpret_cast<uint16_t*>(buffer + SWF_OFFSET_UINT16_STATUS) = enable ? 0x0000 : 0x03E4;
    buffer[SWF_OFFSET_UINT8_FLAG2] = 0x01;
    *reinterpret_cast<uint32_t*>(buffer + SWF_OFFSET_UINT32_VOL_MASK_COPY) = newVol;

    // Calc MD5
    MD5 md5;
    unsigned char digest[MD5::HashBytes] = { 0 };
    md5.add(buffer + SWF_OFFSET_UINT32_NEXT_MASK, CONFIG_SIZE - SWF_OFFSET_UINT32_NEXT_MASK);
    md5.getHash(digest);
    memcpy(buffer, digest, 16);

    logger.DLog(LogLevel::Debug, L"Config buffer ready");
    HexDump(buffer, 0x90);

    // Write to driver
    if (IsInitialized()) {
        DWORD bytesReturned = 0;
        unsigned char dummyBuffer[1024] = { 0 };
        if (DeviceIoControl(m_hDriver, IOCTL_PREPARE_WRITE, nullptr, 0, dummyBuffer, 1024, &bytesReturned, nullptr)) {
            logger.DLog(LogLevel::Debug, L"IOCTL_PREPARE_WRITE success");
        }
        else {
            DWORD err = GetLastError();
            logger.DLog(LogLevel::Error, format(L"IOCTL_PREPARE_WRITE fail, err: {}", err));
        }

        DWORD bytesWritten = 0;
        if (!WriteFile(m_hDriver, buffer, CONFIG_SIZE, &bytesWritten, nullptr)) {
            DWORD err = GetLastError();
            logger.DLog(LogLevel::Error, format(L"Write driver fail, err: {}", err));
            return false;
        }

        // Write to config file
        HANDLE hConfigFile = CreateFileW(SWF_CONFIG_PATH, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
        if (hConfigFile == INVALID_HANDLE_VALUE) {
            DWORD err = GetLastError();
            logger.DLog(LogLevel::Error, format(L"Open config file fail, err: {}", err));
            return false;
        }

        if (!WriteFile(hConfigFile, buffer, CONFIG_SIZE, &bytesWritten, nullptr)) {
            DWORD err = GetLastError();
            logger.DLog(LogLevel::Error, format(L"Write config file fail, err: {}", err));
            CloseHandle(hConfigFile);
            return false;
        }

        CloseHandle(hConfigFile);
    }
    logger.DLog(LogLevel::Info, L"Config modified, reboot to apply");
    return true;
}

std::string HFreezeDriver::GetCurrentConfig()const
{
    fstream file;
    file.open(SWF_CONFIG_PATH, ios::binary | ios::in);
    if (file.fail()) {
        wcout << L"Failed to open config file";
        //WLog()
    }
    string res;
    res.resize(1024);
    file.read((char*)res.c_str(), 1024);
    return res;
}

FreezeResult HFreezeDriver::ParseFreezeBuffer(const unsigned char* buf, int len, QueryResult query) {
    auto read_uint32 = [&](size_t offset) -> uint32_t {
        if (offset + 4 > static_cast<size_t>(len)) return 0;
        return static_cast<uint32_t>(buf[offset]) |
            (static_cast<uint32_t>(buf[offset + 1]) << 8) |
            (static_cast<uint32_t>(buf[offset + 2]) << 16) |
            (static_cast<uint32_t>(buf[offset + 3]) << 24);
        };
    auto read_uint16 = [&](size_t offset) -> uint16_t {
        if (offset + 2 > static_cast<size_t>(len)) return 0;
        return static_cast<uint16_t>(buf[offset]) |
            (static_cast<uint16_t>(buf[offset + 1]) << 8);
        };
    auto read_uint8 = [&](size_t offset) -> uint8_t {
        if (offset >= static_cast<size_t>(len)) return 0;
        return buf[offset];
        };

    // Read configuration fields
    uint32_t next_mask = read_uint32(SWF_OFFSET_UINT32_NEXT_MASK);
    uint8_t flag1 = read_uint8(SWF_OFFSET_UINT8_FLAG1);
    uint16_t status = read_uint16(SWF_OFFSET_UINT16_STATUS);
    uint8_t flag2 = read_uint8(SWF_OFFSET_UINT8_FLAG2);
    uint32_t vol_mask_copy = read_uint32(SWF_OFFSET_UINT32_VOL_MASK_COPY);

    // Device ID (ASCII, starting at 0x55 until 0x00)
    string device_id;
    for (size_t i = 0x55; i < static_cast<size_t>(len) && buf[i] != 0; ++i) {
        if (isprint(buf[i])) device_id.push_back(static_cast<char>(buf[i]));
    }
    // School code (4 bytes ASCII, at 0x68)
    string school_code;
    for (size_t i = 0x68; i < 0x68 + 4 && i < static_cast<size_t>(len); ++i) {
        if (isprint(buf[i])) school_code.push_back(static_cast<char>(buf[i]));
    }

    // MD5 (first 16 bytes)
    string md5_str;
    for (int i = 0; i < 16; ++i) {
        md5_str += format("{:02X}", buf[i]);
    }

    // 4. Determine current freeze state based on runtime activeFlag and next_mask
    bool is_active = (query.runtimeStatus.querySuccess && query.runtimeStatus.activeFlag != 0);
    map<wchar_t, DiskInfo> disk_infos;

    // Iterate over drives A: ~ Z:
    string drives = HugoUtils::GetLogicalDrives();
    for (char letter : drives) {
        int bit = letter - 'A';
        bool frozen_in_config = (next_mask & (1 << bit)) != 0;
        DriveFreezeState state;
        if (is_active && frozen_in_config) {
            state = DriveFreezeState::Frozen;         // Currently frozen
        }
        else if (!is_active && frozen_in_config) {
            state = DriveFreezeState::PendingFreeze; // Pending freeze after reboot
        }
        else if (is_active && !frozen_in_config) {
            state = DriveFreezeState::Unfrozen;     // Not frozen
        }
        else {
            state = DriveFreezeState::Unfrozen;     // Completely unfrozen
        }
        disk_infos[letter] = { state };
    }

    // 5. Build FreezeResult, fill extra info
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

FreezeResult HFreezeDriver::ParseFreezeBuffer(QueryResult query)
{
    return ParseFreezeBuffer(query.bootConfig.buffer, query.bootConfig.validLen, query);
}

wstring HFreezeDriver::HexDump(const unsigned char* data, int len)noexcept {
    wstring content;
    content += L"    Offset | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n";
    content += L"    -------+------------------------------------------------\n";

    for (int i = 0; i < len; i += 16) {
        wstring line = format(L"    0x{:04X} | ", i);
        for (int j = 0; j < 16; j++) {
            if (i + j < len) {
                line += format(L"{:02X} ", data[i + j]);
            }
            else {
                line += L"   ";
            }
        }
        content += line;
        content += L"\n";
    }
    return content;
}

wstring HFreezeDriver::FormatFreezeStateResult(const FreezeResult& result) {
    wstring output;

    if (result.result != FrzOR::Success) {
        output += format(L"Failed to get state: {}\n", result.msg);
        if (result.error != ERROR_SUCCESS) {
            output += format(L"Error code: {}\n", result.error);
        }
        return output;
    }

    // Basic information
    output += L"========== Seewo Driver Freeze Status ==========\n";
    output += format(L"Operation time: {}\n", result.operateTime.empty() ? L"Unknown" : result.operateTime);

    // Extra details
    output += L"\n--- Boot Configuration Details ---\n";
    output += format(L"MD5 checksum: {}\n", ConvertString(result.extra.md5));
    output += format(L"Next boot mask: 0x{:08X} -> ", result.extra.next_mask);
    // Decode drive letters
    bool first = true;
    for (char c = 'A'; c <= 'Z'; ++c) {
        if (result.extra.next_mask & (1 << (c - 'A'))) {
            if (!first) output += L", ";
            output += format(L"{}:", c);
            first = false;
        }
    }
    if (result.extra.next_mask == 0) output += L"None";
    output += L"\n";
    output += format(L"Backup mask: 0x{:08X}\n", result.extra.vol_mask_copy);
    output += format(L"Flag1: 0x{:02X}   Status word: 0x{:04X}   Flag2: 0x{:02X}\n",
        result.extra.flag1, result.extra.status, result.extra.flag2);
    output += format(L"Device ID: {}\n", result.extra.device_id.empty() ? L"(none)" : wstring(result.extra.device_id.begin(), result.extra.device_id.end()));
    output += format(L"School code: {}\n", result.extra.school_code.empty() ? L"(none)" : wstring(result.extra.school_code.begin(), result.extra.school_code.end()));

    // Current freeze state
    output += L"\n--- Current Freeze Status ---\n";
    if (result.diskInfos.empty()) {
        output += L"No frozen drive letters detected\n";
    }
    else {
        for (const auto& [drive, info] : result.diskInfos) {
            wstring state_str;
            switch (info.state) {
            case DriveFreezeState::Frozen:         state_str = L"Frozen"; break;
            case DriveFreezeState::Unfrozen:       state_str = L"Unfrozen"; break;
            case DriveFreezeState::PendingFreeze:  state_str = L"Pending freeze (after reboot)"; break;
            case DriveFreezeState::PendingUnfreeze:state_str = L"Pending unfreeze (after reboot)"; break;
            default:                               state_str = L"Unknown"; break;
            }
            output += format(L"Drive {}: {}\n", drive, state_str);
        }
    }

    output += L"========================================\n";
    return output;
}

wstring HFreezeDriver::FormatFreezeStateResult(
    const FreezeResult& result,
    uint32_t activeFlag,
    uint64_t ptr1,
    const wstring& logStr
) {
    wstring output;

    if (result.result != FrzOR::Success) {
        output += format(L"Failed to get state: {}\n", result.msg);
        if (result.error != ERROR_SUCCESS) {
            output += format(L"Error code: {}\n", result.error);
        }
        return output;
    }

    // ========== Title ==========
    output += L"========== Seewo Driver Freeze Status ==========\n";
    output += format(L"Operation time: {}\n", result.operateTime.empty() ? L"Unknown" : result.operateTime);

    // ========== Runtime Status ==========
    output += L"\n--- Runtime Status ---\n";
    output += format(L"Active flag: {} ({})\n", activeFlag, activeFlag ? L"Active" : L"Inactive");
    output += format(L"Pointer1: 0x{:X}\n", ptr1);
    output += format(L"Last log: {}\n", logStr.empty() ? L"(none)" : logStr);

    // ========== Boot Configuration Details ==========
    output += L"\n--- Boot Configuration Details ---\n";
    output += format(L"MD5 checksum: {}\n", ConvertString(result.extra.md5));
    output += format(L"Next boot mask: 0x{:08X} -> ", result.extra.next_mask);
    // Decode drive letters
    bool first = true;
    for (char c = 'A'; c <= 'Z'; ++c) {
        if (result.extra.next_mask & (1 << (c - 'A'))) {
            if (!first) output += L", ";
            output += format(L"{}:", c);
            first = false;
        }
    }
    if (result.extra.next_mask == 0) output += L"None";
    output += L"\n";
    output += format(L"Backup mask: 0x{:08X}\n", result.extra.vol_mask_copy);
    output += format(L"Flag1: 0x{:02X}   Status word: 0x{:04X}   Flag2: 0x{:02X}\n",
        result.extra.flag1, result.extra.status, result.extra.flag2);
    output += format(L"Device ID: {}\n", result.extra.device_id.empty() ? L"(none)" : wstring(result.extra.device_id.begin(), result.extra.device_id.end()));
    output += format(L"School code: {}\n", result.extra.school_code.empty() ? L"(none)" : wstring(result.extra.school_code.begin(), result.extra.school_code.end()));

    // ========== Current Freeze Status ==========
    output += L"\n--- Current Freeze Status ---\n";
    if (result.diskInfos.empty()) {
        output += L"No frozen drive letters detected\n";
    }
    else {
        for (const auto& [drive, info] : result.diskInfos) {
            wstring state_str;
            switch (info.state) {
            case DriveFreezeState::Frozen:         state_str = L"Frozen"; break;
            case DriveFreezeState::Unfrozen:       state_str = L"Unfrozen"; break;
            case DriveFreezeState::PendingFreeze:  state_str = L"Pending freeze (after reboot)"; break;
            case DriveFreezeState::PendingUnfreeze:state_str = L"Pending unfreeze (after reboot)"; break;
            default:                               state_str = L"Unknown"; break;
            }
            output += format(L"Drive {}: {}\n", drive, state_str);
        }
    }

    output += L"========================================\n";
    return output;
}

FreezeResult HFreezeDriver::SetFreezeState(
    const wstring& driveLetters
) noexcept {
    if (driveLetters.empty()) {
        logger.DLog(LogLevel::Info, L"Unfreeze all drives");
        bool res = ModifyConfig(0, false);
        return FreezeResult(res ? FrzOR::Success : FrzOR::Failed);
    }
    uint32_t volMask = CalculateVolumeMask(driveLetters);
    if (volMask == -1) {
        logger.DLog(LogLevel::Error, format(L"Invalid drive letters: {}", driveLetters));
        return FreezeResult(FrzOR::Failed).setErrMsg(L"Invalid drive letters");
    }
    logger.DLog(LogLevel::Info, format(L"Freeze drives: {} (mask=0x{:08X})", driveLetters, volMask));
    bool res = ModifyConfig(volMask, true);
    return FreezeResult(res ? FrzOR::Success : FrzOR::Failed);
}

wstring HFreezeDriver::GetLastErrorMsg() const noexcept {
    return L"";
}
DWORD HFreezeDriver::GetLastErrorCode() const noexcept {
    return 0;
}
#endif // !HU_DISABLE_FREEZE_DRIVER