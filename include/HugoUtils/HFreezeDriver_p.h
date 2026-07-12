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
 *
 * HFreezeDriverPrivate ¨C All IOCTL communication with SWFreeze.sys.
 */
#pragma once
#include "HugoUtilsDef.h"
#ifndef HU_DISABLE_FREEZE_DRIVER

#include "SWFreezeTypes.h"
#include <Windows.h>
#include <vector>
#include <optional>
#include <string>

class HFreezeDriverPrivate {
public:
    HFreezeDriverPrivate() = default;
    ~HFreezeDriverPrivate() { Cleanup(); }

    bool Init();
    void Cleanup();
    bool IsInitialized() const { return m_hDriver && m_hDriver != INVALID_HANDLE_VALUE; }

    // ---- Configuration read / write ----
    std::optional<ProtectInfo> QueryBootConfig() const;        // 0x80002008
    bool WriteBootConfig(const ProtectInfo& config);           // 0x80002064 + WriteFile

    // ---- Status queries ----
    std::optional<FreezeBootSystem>        QueryBootSystem() const;        // 0x80002038
    std::optional<FreezeKeyResult>         QueryKeyResult() const;         // 0x80002028 (0x800)
    std::optional<FreezeProtectionState>   QueryProtectionState() const;   // 0x8000202C
    std::optional<FreezePassThrough>       QueryPassThrough() const;       // 0x8000203C
    std::optional<FreezeOldDriverQuality>  QueryOldDriverQuality() const;  // 0x80002040
    std::optional<FreezeDiskFull>          QueryDiskFull() const;          // 0x80002044
    std::optional<FreezeBsodInfo>          QueryBsodInfo() const;          // 0x80002058
    std::optional<FreezeRedirectData>      QueryRedirectData() const;      // 0x80002060

    // Special: thread pool IDs + redirect data (0x828)
    struct ThreadPoolResult {
        std::vector<uint32_t> threadIds;
        FreezeRedirectData redirectData;
    };
    std::optional<ThreadPoolResult> QueryThreadPoolAndRedirect() const;    // 0x8000205C

    // ---- Input / configuration uploads ----
    bool SetNotifyHandles(const FreezeNotifyHandles& handles);   // 0x80002034
    bool SetProcessImage(const FreezeImageInfo& info);           // 0x8000204C
    bool SetDriverImage(const FreezeImageInfo& info);            // 0x80002054

    // ---- Backdoors (debug only) ----
    void TriggerBSOD() const;       // 0x80002190
    void FlushWppLogs() const;      // 0x80002194

    // Error retrieval
    DWORD GetLastErrorCode() const { return m_lastError; }
    std::wstring GetLastErrorMsg() const { return m_lastErrorMsg; }

private:
    HANDLE m_hDriver = nullptr;
    mutable DWORD m_lastError = 0;
    mutable std::wstring m_lastErrorMsg;

    template<typename T>
    std::optional<T> IoControlOut(DWORD code, size_t minBufSize = 0x400) const;

    template<typename T>
    bool IoControlIn(DWORD code, const T& data, size_t size = 0x400) const;
};
#endif // !HU_DISABLE_FREEZE_DRIVER