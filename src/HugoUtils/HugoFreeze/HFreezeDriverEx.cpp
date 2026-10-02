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

#include <format>

#include "HugoUtils/HugoFreeze/HFreezeDriverEx.h"
#include "WinUtils/Logger.h"

using namespace WinUtils;
using namespace std;

static Logger logger(L"HFreezeDriverEx");

// Build the message describing a failed driver call. The driver rejects every
// request whose buffer is smaller than the required size with
// STATUS_UNSUCCESSFUL, so that status is called out explicitly.
static std::wstring FrzDriverErrorText(std::wstring_view what, DWORD err) {
    if (err == FRZ_STATUS_UNSUCCESSFUL) {
        return format(L"{} failed, status: STATUS_UNSUCCESSFUL (0x{:08X})", what, err);
    }
    return format(L"{} failed, err: {}", what, err);
}

// ---------------------------------------------------------------------------
// HDriverHandle
// ---------------------------------------------------------------------------
HANDLE HDriverHandle::s_handle = nullptr;
int    HDriverHandle::s_refCount = 0;
DWORD  HDriverHandle::s_lastError = ERROR_SUCCESS;

HDriverHandle::~HDriverHandle() noexcept {
    close();
}

void HDriverHandle::open() noexcept {
    if (s_refCount == 0) {
        s_handle = CreateFileW(
            FRZ_DRIVER_PATH,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            nullptr,
            OPEN_EXISTING,
            0,
            nullptr
        );
        if (s_handle == INVALID_HANDLE_VALUE) {
            s_lastError = GetLastError();
            s_handle = nullptr;
            logger.DLog(LogLevel::Error,
                format(L"Open driver fail, err: {}", s_lastError));
            return;
        }
    }
    ++s_refCount;
}

void HDriverHandle::close() noexcept {
    if (s_refCount == 0) return;
    if (--s_refCount == 0 && s_handle) {
        CloseHandle(s_handle);
        s_handle = nullptr;
    }
}

bool HDriverHandle::isOpen() const noexcept {
    return s_handle != nullptr && s_handle != INVALID_HANDLE_VALUE;
}

HANDLE HDriverHandle::handle() const noexcept {
    return isOpen() ? s_handle : nullptr;
}

DWORD HDriverHandle::lastError() noexcept {
    return s_lastError;
}

// ---------------------------------------------------------------------------
// HFreezeDriverEx
// ---------------------------------------------------------------------------
bool HFreezeDriverEx::open() noexcept {
    m_handle.open();
    if (!m_handle.isOpen()) {
        m_lastError = HDriverHandle::lastError();
        m_lastErrorMsg = L"Failed to open SWFreeze device";
        return false;
    }
    return true;
}

void HFreezeDriverEx::close() noexcept {
    m_handle.close();
}

bool HFreezeDriverEx::isOpen() const noexcept {
    return m_handle.isOpen();
}

bool HFreezeDriverEx::deviceControl(DWORD ioctl, const void* inBuf, DWORD inSize,
    void* outBuf, DWORD outSize) const noexcept {
    if (!isOpen()) {
        m_lastError = ERROR_INVALID_HANDLE;
        m_lastErrorMsg = L"SWFreeze device is not open";
        return false;
    }

    DWORD bytesReturned = 0;
    if (!DeviceIoControl(m_handle.handle(), ioctl, const_cast<void*>(inBuf), inSize,
        outBuf, outSize, &bytesReturned, nullptr)) {
        m_lastError = GetLastError();
        m_lastErrorMsg = FrzDriverErrorText(
            format(L"DeviceIoControl 0x{:08X}", ioctl), m_lastError);
        logger.DLog(LogLevel::Error, m_lastErrorMsg);
        return false;
    }
    return true;
}

bool HFreezeDriverEx::getConfig(unsigned char* data, size_t size) const noexcept {
    if (!data || size < FRZ_CONFIG_SIZE || !isOpen()) return false;

    unsigned char buf[FRZ_CONFIG_SIZE] = { 0 };
    DWORD bytesReturned = 0;
    if (!DeviceIoControl(m_handle.handle(), FRZ_IOCTL_READ_MEM_CONF, nullptr, 0,
        buf, FRZ_CONFIG_SIZE, &bytesReturned, nullptr)) {
        m_lastError = GetLastError();
        m_lastErrorMsg = FrzDriverErrorText(L"IOCTL_READ_MEM_CONF", m_lastError);
        logger.DLog(LogLevel::Error, m_lastErrorMsg);
        return false;
    }

    memcpy(data, buf, FRZ_CONFIG_SIZE);
    return true;
}

bool HFreezeDriverEx::setConfig(const unsigned char* data, size_t size) noexcept {
    if (!data || size < FRZ_CONFIG_SIZE || !isOpen()) return false;

    // Prepare the driver for a configuration write.
    unsigned char dummy[FRZ_CONFIG_SIZE] = { 0 };
    DWORD bytesReturned = 0;
    if (!DeviceIoControl(m_handle.handle(), FRZ_IOCTL_PREPARE_WRITE, nullptr, 0,
        dummy, FRZ_CONFIG_SIZE, &bytesReturned, nullptr)) {
        m_lastError = GetLastError();
        m_lastErrorMsg = FrzDriverErrorText(L"IOCTL_PREPARE_WRITE", m_lastError);
        logger.DLog(LogLevel::Error, m_lastErrorMsg);
        return false;
    }

    DWORD bytesWritten = 0;
    if (!WriteFile(m_handle.handle(), data, static_cast<DWORD>(FRZ_CONFIG_SIZE),
        &bytesWritten, nullptr)) {
        m_lastError = GetLastError();
        m_lastErrorMsg = FrzDriverErrorText(L"Write driver config", m_lastError);
        logger.DLog(LogLevel::Error, m_lastErrorMsg);
        return false;
    }
    return true;
}

bool HFreezeDriverEx::getRuntimeStatus(unsigned char* data, size_t size) const noexcept {
    if (!data || size < FRZ_CONFIG_SIZE || !isOpen()) return false;

    DWORD bytesReturned = 0;
    if (!DeviceIoControl(m_handle.handle(), FRZ_IOCTL_QUERY_RUNTIME, nullptr, 0,
        data, static_cast<DWORD>(size), &bytesReturned, nullptr)) {
        m_lastError = GetLastError();
        m_lastErrorMsg = FrzDriverErrorText(L"IOCTL_QUERY_RUNTIME", m_lastError);
        logger.DLog(LogLevel::Error, m_lastErrorMsg);
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// IOCTL wrappers
//
// Every query zeroes the destination buffer first, so a caller can never read
// stale data when the driver fails.
// ---------------------------------------------------------------------------
bool HFreezeDriverEx::queryBaseVersion(FreezeBaseVersion& out) const noexcept {
    out = FreezeBaseVersion{};
    return deviceControl(FRZ_IOCTL_QUERY_BASE_VERSION, nullptr, 0, &out,
        static_cast<DWORD>(sizeof(out)));
}

bool HFreezeDriverEx::queryBootSystem(FreezeEventBootSystem& out) const noexcept {
    out = FreezeEventBootSystem{};
    return deviceControl(FRZ_IOCTL_QUERY_BOOT_SYSTEM, nullptr, 0, &out,
        static_cast<DWORD>(sizeof(out)));
}

bool HFreezeDriverEx::queryKeyResult(FreezeKeyResult& out) const noexcept {
    out = FreezeKeyResult{};
    return deviceControl(FRZ_IOCTL_QUERY_KEY_RESULT, nullptr, 0, &out,
        static_cast<DWORD>(sizeof(out)));
}

bool HFreezeDriverEx::queryProtectionState(FreezeProtectionState& out) const noexcept {
    out = FreezeProtectionState{};

    // The driver requires a 0x400 output buffer even though only the leading
    // bytes of it carry the state.
    unsigned char buffer[FRZ_DRIVER_BUFFER_SIZE] = { 0 };
    if (!deviceControl(FRZ_IOCTL_QUERY_PROTECTION_STATE, nullptr, 0, buffer,
        static_cast<DWORD>(FRZ_DRIVER_BUFFER_SIZE))) {
        return false;
    }
    memcpy(&out, buffer, sizeof(out));
    return true;
}

bool HFreezeDriverEx::queryPassThrough(FreezeEventPassThrough& out) const noexcept {
    out = FreezeEventPassThrough{};
    return deviceControl(FRZ_IOCTL_QUERY_PASS_THROUGH, nullptr, 0, &out,
        static_cast<DWORD>(sizeof(out)));
}

bool HFreezeDriverEx::queryOldDriverQuality(FreezeEventOldDriverQuality& out) const noexcept {
    out = FreezeEventOldDriverQuality{};
    return deviceControl(FRZ_IOCTL_QUERY_OLD_DRIVER_QUALITY, nullptr, 0, &out,
        static_cast<DWORD>(sizeof(out)));
}

bool HFreezeDriverEx::queryDiskFull(FreezeEventDiskFull& out) const noexcept {
    out = FreezeEventDiskFull{};
    return deviceControl(FRZ_IOCTL_QUERY_DISK_FULL, nullptr, 0, &out,
        static_cast<DWORD>(sizeof(out)));
}

bool HFreezeDriverEx::queryBsodInfo(BsodInfo& out) const noexcept {
    out = BsodInfo{};
    return deviceControl(FRZ_IOCTL_QUERY_BSOD_INFO, nullptr, 0, &out,
        static_cast<DWORD>(sizeof(out)));
}

bool HFreezeDriverEx::queryRedirectData(FreezeRedirectData& out) const noexcept {
    out = FreezeRedirectData{};
    return deviceControl(FRZ_IOCTL_QUERY_REDIRECT_DATA, nullptr, 0, &out,
        static_cast<DWORD>(sizeof(out)));
}

bool HFreezeDriverEx::queryTidRedirect(FreezeTidRedirectBuffer& out) const noexcept {
    out = FreezeTidRedirectBuffer{};
    // This IOCTL only accepts a buffer of exactly FRZ_DRIVER_REDIRECT_TID_SIZE.
    return deviceControl(FRZ_IOCTL_QUERY_TID_REDIRECT, nullptr, 0, &out,
        static_cast<DWORD>(sizeof(out)));
}

bool HFreezeDriverEx::queryProcessImage(FreezeImageInfo& out) const noexcept {
    out = FreezeImageInfo{};
    return deviceControl(FRZ_IOCTL_QUERY_PROCESS_IMAGE, nullptr, 0, &out,
        static_cast<DWORD>(sizeof(out)));
}

bool HFreezeDriverEx::setProcessImage(const FreezeImageInfo& image) noexcept {
    unsigned char scratch[FRZ_DRIVER_BUFFER_SIZE] = { 0 };
    return deviceControl(FRZ_IOCTL_SET_PROCESS_IMAGE, &image,
        static_cast<DWORD>(sizeof(image)), scratch,
        static_cast<DWORD>(FRZ_DRIVER_BUFFER_SIZE));
}

bool HFreezeDriverEx::queryDriverImage(FreezeImageInfo& out) const noexcept {
    out = FreezeImageInfo{};
    return deviceControl(FRZ_IOCTL_QUERY_DRIVER_IMAGE, nullptr, 0, &out,
        static_cast<DWORD>(sizeof(out)));
}

bool HFreezeDriverEx::setDriverImage(const FreezeImageInfo& image) noexcept {
    unsigned char scratch[FRZ_DRIVER_BUFFER_SIZE] = { 0 };
    return deviceControl(FRZ_IOCTL_SET_DRIVER_IMAGE, &image,
        static_cast<DWORD>(sizeof(image)), scratch,
        static_cast<DWORD>(FRZ_DRIVER_BUFFER_SIZE));
}

bool HFreezeDriverEx::setNotifyHandles(const FreezeEventNotifyHandles& handles) noexcept {
    // Input-only IOCTL: the driver validates both buffer lengths, so a
    // throw-away output buffer of the same size is supplied.
    unsigned char scratch[FRZ_DRIVER_BUFFER_SIZE] = { 0 };
    return deviceControl(FRZ_IOCTL_SET_NOTIFY_HANDLES, &handles,
        static_cast<DWORD>(sizeof(handles)), scratch,
        static_cast<DWORD>(FRZ_DRIVER_BUFFER_SIZE));
}

bool HFreezeDriverEx::triggerBsod() noexcept {
    // Back door that bugchecks the system; it takes no buffer.
    return deviceControl(FRZ_IOCTL_TRIGGER_BSOD, nullptr, 0, nullptr, 0);
}

bool HFreezeDriverEx::flushWppLogs() noexcept {
    // Flushes the driver WPP trace cache; it takes no buffer.
    return deviceControl(FRZ_IOCTL_FLUSH_WPP_LOGS, nullptr, 0, nullptr, 0);
}

DWORD HFreezeDriverEx::getLastError() const noexcept {
    return m_lastError;
}

const std::wstring& HFreezeDriverEx::getLastErrorMsg() const noexcept {
    return m_lastErrorMsg;
}

#endif // !HU_DISABLE_FREEZE_DRIVER
