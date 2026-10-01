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
#ifndef HU_DISABLE_FREEZE_DRIVER

#include <Windows.h>
#include <cstddef>
#include <cstdint>
#include <string>

#include "HugoUtils/HugoFreeze/HFreezeDef.h"

 // ---------------------------------------------------------------------------
 // HDriverHandle — reference-counted owner of the SWFreeze device handle.
 // The device is opened on the first open() and closed only when the last
 // owner releases it (refCount drops to 0). There is a single process-wide
 // instance, shared by every owner.
 // ---------------------------------------------------------------------------
class HDriverHandle {
public:
    HDriverHandle() noexcept = default;
    ~HDriverHandle() noexcept;

    HDriverHandle(const HDriverHandle&) = delete;
    HDriverHandle& operator=(const HDriverHandle&) = delete;
    HDriverHandle(HDriverHandle&&) = delete;
    HDriverHandle& operator=(HDriverHandle&&) = delete;

    void open() noexcept;   // open the driver handle
    void close() noexcept;  // close it when the ref count reaches zero

    bool isOpen() const noexcept;
    HANDLE handle() const noexcept;

    static DWORD lastError() noexcept;

private:
    static HANDLE s_handle;    // the sole device handle
    static int    s_refCount;  // reference counter guarding s_handle
    static DWORD  s_lastError; // last CreateFileW error
};

// ---------------------------------------------------------------------------
// HFreezeDriverEx — low-level, C-style wrapper over the SWFreeze driver.
// Every IOCTL the driver exposes is wrapped by a strongly typed method; the
// generic deviceControl() is the shared core they all use.
// ---------------------------------------------------------------------------
class HFreezeDriverEx {
public:
    HFreezeDriverEx() noexcept = default;
    ~HFreezeDriverEx() noexcept = default;

    HFreezeDriverEx(const HFreezeDriverEx&) = delete;
    HFreezeDriverEx& operator=(const HFreezeDriverEx&) = delete;
    HFreezeDriverEx(HFreezeDriverEx&&) = delete;
    HFreezeDriverEx& operator=(HFreezeDriverEx&&) = delete;

    bool open() noexcept;
    void close() noexcept;
    bool isOpen() const noexcept;

    // Generic IOCTL core. Sends inSize bytes from inBuf and receives outSize
    // bytes into outBuf. Returns false (and records the error) when the call
    // fails or when the driver answers STATUS_UNSUCCESSFUL, which it uses to
    // reject undersized buffers.
    bool deviceControl(DWORD ioctl, const void* inBuf, DWORD inSize,
        void* outBuf, DWORD outSize) const noexcept;

    // Raw 1024-byte configuration blob (HConfigFile::toBuffer layout).
    bool getConfig(unsigned char* data, size_t size) const noexcept;
    bool setConfig(const unsigned char* data, size_t size) noexcept;

    // Raw runtime-status blob.
    bool getRuntimeStatus(unsigned char* data, size_t size) const noexcept;

    // ---- Driver status queries (all outputs are zeroed on failure) ----
    bool queryBaseVersion(FreezeBaseVersion& out) const noexcept;
    bool queryBootSystem(FreezeEventBootSystem& out) const noexcept;
    bool queryKeyResult(FreezeKeyResult& out) const noexcept;
    bool queryProtectionState(FreezeProtectionState& out) const noexcept;
    bool queryPassThrough(FreezeEventPassThrough& out) const noexcept;
    bool queryOldDriverQuality(FreezeEventOldDriverQuality& out) const noexcept;
    bool queryDiskFull(FreezeEventDiskFull& out) const noexcept;
    bool queryBsodInfo(BsodInfo& out) const noexcept;
    bool queryRedirectData(FreezeRedirectData& out) const noexcept;
    bool queryTidRedirect(FreezeTidRedirectBuffer& out) const noexcept;

    // ---- Protected image registration (process / driver) ----
    bool queryProcessImage(FreezeImageInfo& out) const noexcept;
    bool setProcessImage(const FreezeImageInfo& image) noexcept;
    bool queryDriverImage(FreezeImageInfo& out) const noexcept;
    bool setDriverImage(const FreezeImageInfo& image) noexcept;

    // ---- R3 notification event handles ----
    bool setNotifyHandles(const FreezeEventNotifyHandles& handles) noexcept;

    // ---- Driver maintenance ----
    bool triggerBsod() noexcept;
    bool flushWppLogs() noexcept;

    DWORD getLastError() const noexcept;
    const std::wstring& getLastErrorMsg() const noexcept;

private:
    HDriverHandle        m_handle;
    mutable DWORD        m_lastError = ERROR_SUCCESS;
    mutable std::wstring m_lastErrorMsg;
};

#endif // !HU_DISABLE_FREEZE_DRIVER
