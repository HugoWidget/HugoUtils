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

#include <string>

#include "HugoUtils/HugoFreeze/HFreezeDef.h"
#include "HugoUtils/HugoFreeze/HFreezeDriverEx.h"

 // ---------------------------------------------------------------------------
 // HFreezeDriverBackend — freeze-driver operation wrapper built on top of
 // HFreezeDriverEx. Exchanges HConfigFile instead of raw buffers.
 // ---------------------------------------------------------------------------
class HFreezeDriverBackend {
public:
    HFreezeDriverBackend() noexcept = default;
    ~HFreezeDriverBackend() noexcept = default;

    HFreezeDriverBackend(const HFreezeDriverBackend&) = delete;
    HFreezeDriverBackend& operator=(const HFreezeDriverBackend&) = delete;

    bool open() noexcept;
    void close() noexcept;
    bool isOpen() const noexcept;

    bool getConfig(HConfigFile& out) const noexcept;
    bool setConfig(const HConfigFile& cfg) noexcept;

    bool getRuntimeStatus(DriverRuntimeStatus& out) const noexcept;

    DWORD getLastError() const noexcept;
    const std::wstring& getLastErrorMsg() const noexcept;

private:
    HFreezeDriverEx m_driver;
};

#endif // !HU_DISABLE_FREEZE_DRIVER
