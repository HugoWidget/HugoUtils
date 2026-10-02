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

#include <cstring>

#include "HugoUtils/HugoFreeze/HFreezeDriverBackend.h"

using namespace std;

bool HFreezeDriverBackend::open() noexcept {
    return m_driver.open();
}

void HFreezeDriverBackend::close() noexcept {
    m_driver.close();
}

bool HFreezeDriverBackend::isOpen() const noexcept {
    return m_driver.isOpen();
}

bool HFreezeDriverBackend::getConfig(HConfigFile& out) const noexcept {
    unsigned char buffer[FRZ_CONFIG_SIZE] = { 0 };
    if (!m_driver.getConfig(buffer, FRZ_CONFIG_SIZE)) {
        return false;
    }
    out = HConfigFile(buffer);
    return true;
}

bool HFreezeDriverBackend::setConfig(const HConfigFile& cfg) noexcept {
    unsigned char buffer[FRZ_CONFIG_SIZE] = { 0 };
    cfg.toBuffer(buffer);
    return m_driver.setConfig(buffer, FRZ_CONFIG_SIZE);
}

bool HFreezeDriverBackend::getRuntimeStatus(DriverRuntimeStatus& out) const noexcept {
    unsigned char buffer[FRZ_CONFIG_SIZE] = { 0 };
    if (!m_driver.getRuntimeStatus(buffer, FRZ_CONFIG_SIZE)) {
        out.querySuccess = false;
        return false;
    }

    out.activeFlag = *reinterpret_cast<uint32_t*>(buffer + FRZ_OFF_RT_ACTIVE);
    out.ptr1 = *reinterpret_cast<uint64_t*>(buffer + FRZ_OFF_RT_PTR1);

    buffer[FRZ_CONFIG_SIZE - 1] = 0;
    const char* logStr = reinterpret_cast<const char*>(buffer + FRZ_OFF_RT_LOG);
    out.logStr = wstring(logStr, logStr + strlen(logStr));
    out.querySuccess = true;
    return true;
}

DWORD HFreezeDriverBackend::getLastError() const noexcept {
    return m_driver.getLastError();
}

const std::wstring& HFreezeDriverBackend::getLastErrorMsg() const noexcept {
    return m_driver.getLastErrorMsg();
}

#endif // !HU_DISABLE_FREEZE_DRIVER
