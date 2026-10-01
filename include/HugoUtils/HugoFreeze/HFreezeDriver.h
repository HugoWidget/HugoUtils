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
 * HFreezeDriver — high-level freeze management. Adapts HFreezeConfig to the
 * IHugoFreeze interface. Both the default queries and the default state setter
 * go through the layered wrappers (HFreezeConfig -> backends -> HFreezeDriverEx).
 */
#pragma once
#include "HugoUtils/HugoUtilsDef.h"
#ifndef HU_DISABLE_FREEZE_DRIVER

#include <cstddef>
#include <string>

#include "HugoUtils/HugoFreeze/HFreezeInterface.h"
#include "HugoUtils/HugoFreeze/HFreezeDef.h"
#include "HugoUtils/HugoFreeze/HFreezeConfig.h"

class HFreezeDriver : public IHugoFreeze {
public:
    HFreezeDriver() noexcept = default;
    ~HFreezeDriver() noexcept override = default;

    HFreezeDriver(const HFreezeDriver&) = delete;
    HFreezeDriver& operator=(const HFreezeDriver&) = delete;

    // IHugoFreeze implementation
    FreezeResult Init() noexcept override;
    void Cleanup() noexcept override;
    bool IsInitialized() const noexcept override;

    FreezeResult GetFreezeState() const noexcept override;
    FreezeResult TryProtect(const std::wstring& driveLetters) const noexcept override;
    FreezeResult SetFreezeState(const std::wstring& driveLetters) noexcept override;

    std::wstring GetLastErrorMsg() const noexcept override;
    DWORD GetLastErrorCode() const noexcept override;

    // Extended queries — also routed through HFreezeConfig.
    FreezeResult GetBootFreezeState() const noexcept;
    FreezeResult GetFileFreezeState() const noexcept;
    bool QueryDriverStatus(DriverRuntimeStatus& runtimeOut) const noexcept;

private:
    FreezeResult ParseConfig(const HConfigFile& cfg,
        const DriverRuntimeStatus& runtime) const noexcept;

    HFreezeConfig m_config;
};

#endif // !HU_DISABLE_FREEZE_DRIVER
