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
 * HFreezeDriver – High‑level freeze management.
 * Combines file configuration (HFreezeFilePrivate) and driver state (HFreezeDriverPrivate)
 * to provide an easy‑to‑use interface.
 */
#include "HugoUtils/HugoUtilsDef.h"
#ifndef HU_DISABLE_FREEZE_DRIVER

#include "HugoUtils/HFreezeDriver.h"
#include "HugoUtils/HugoUtils.h"
#include <algorithm>

// ========== IHugoFreeze interface ==========

FreezeResult HFreezeDriver::Init() noexcept {
    if (!m_driver.Init()) {
        return FreezeResult(
            FreezeOperationResult::DriverError,
            L"Failed to open SWFreeze driver",
            m_driver.GetLastErrorCode(),
            m_driver.GetLastErrorMsg()
        );
    }
    return FreezeResult(FreezeOperationResult::Success);
}

void HFreezeDriver::Cleanup() noexcept {
    m_driver.Cleanup();
}

bool HFreezeDriver::IsInitialized() const noexcept {
    return m_driver.IsInitialized();
}

FreezeResult HFreezeDriver::GetFreezeState() const noexcept {
    FreezeResult result;

    // 1. Read the configuration file
    auto fileCfg = HFreezeFilePrivate::ReadConfig();

    // 2. Read the boot configuration from the driver (if available)
    auto bootCfg = m_driver.QueryBootConfig();

    // 3. Read runtime boot system info (contains driver state flag)
    auto bootSys = m_driver.QueryBootSystem();

    // 4. Get the list of logical drives
    std::string drives = HugoUtils::GetLogicalDrives();

    // 5. Determine the freeze state for each drive
    for (char letter : drives) {
        int bit = letter - 'A';
        bool frozenInFile = fileCfg ? (fileCfg->readytoProtectVolume & (1 << bit)) != 0 : false;
        bool frozenInBoot = bootCfg ? (bootCfg->readytoProtectVolume & (1 << bit)) != 0 : false;
        bool driverActive = bootSys ? (bootSys->freezeDriverState != 0) : false;

        DriveFreezeState state = DriveFreezeState::Unknown;
        if (driverActive && frozenInBoot) {
            state = DriveFreezeState::Frozen;
        }
        else if (!driverActive && frozenInFile) {
            state = DriveFreezeState::PendingFreeze;
        }
        else {
            state = DriveFreezeState::Unfrozen;
        }

        result.diskInfos[letter] = { state };
    }

    // 6. Attach the full ProtectInfo if available
    if (fileCfg) {
        result.protectConfig = *fileCfg;
        result.hasProtectConfig = true;
    }

    result.msg = L"Freeze state queried successfully";
    return result;
}

FreezeResult HFreezeDriver::TryProtect(const std::wstring& driveLetters) const noexcept {
    auto state = GetFreezeState();
    uint32_t mask = CalculateVolumeMask(driveLetters);
    if (mask == static_cast<uint32_t>(-1)) {
        return FreezeResult(FreezeOperationResult::InvalidParam, L"Invalid drive letters");
    }

    for (const auto& [letter, info] : state.diskInfos) {
        if (mask & (1 << (letter - L'A'))) {
            if (info.state != DriveFreezeState::Frozen &&
                info.state != DriveFreezeState::PendingFreeze) {
                return FreezeResult(FreezeOperationResult::Failed,
                    L"Not all specified drives are frozen or pending freeze");
            }
        }
    }
    return FreezeResult(FreezeOperationResult::Success);
}

FreezeResult HFreezeDriver::SetFreezeState(const std::wstring& driveLetters) noexcept {
    // An empty string means unfreeze everything
    uint32_t mask = CalculateVolumeMask(driveLetters);
    if (mask == static_cast<uint32_t>(-1)) {
        return FreezeResult(FreezeOperationResult::InvalidParam, L"Invalid drive letters");
    }

    bool enableFreeze = !driveLetters.empty();

    // 1. Read the current file config
    auto currentCfg = HFreezeFilePrivate::ReadConfig();
    if (!currentCfg) {
        return FreezeResult(FreezeOperationResult::Failed, L"Cannot read current configuration file");
    }

    // 2. Build the modified config
    auto newCfg = HFreezeFilePrivate::BuildFreezeConfig(*currentCfg, mask, enableFreeze);

    // 3. Write it back to disk
    if (!HFreezeFilePrivate::WriteConfig(newCfg)) {
        return FreezeResult(FreezeOperationResult::Failed, L"Failed to write configuration file");
    }

    // 4. If the driver is loaded, synchronise the change immediately
    if (m_driver.IsInitialized()) {
        if (!m_driver.WriteBootConfig(newCfg)) {
            return FreezeResult(FreezeOperationResult::Failed,
                L"Configuration file updated, but driver synchronisation failed. A reboot is required.");
        }
    }

    return FreezeResult(FreezeOperationResult::Success,
        L"Configuration updated. Reboot to apply changes.");
}

// ========== Extended status ==========

HFreezeDriver::FullStatus HFreezeDriver::GetFullStatus() const {
    FullStatus status;
    status.fileConfig = HFreezeFilePrivate::ReadConfig();
    status.bootConfig = m_driver.QueryBootConfig();
    status.bootSystem = m_driver.QueryBootSystem();

    if (status.fileConfig || status.bootConfig) {
        std::string drives = HugoUtils::GetLogicalDrives();
        for (char letter : drives) {
            int bit = letter - 'A';
            bool frozenInFile = status.fileConfig ? (status.fileConfig->readytoProtectVolume & (1 << bit)) != 0 : false;
            bool frozenInBoot = status.bootConfig ? (status.bootConfig->readytoProtectVolume & (1 << bit)) != 0 : false;
            bool driverActive = status.bootSystem ? (status.bootSystem->freezeDriverState != 0) : false;

            bool fileFrozen = frozenInFile;
            bool bootFrozen = driverActive && frozenInBoot;

            DriveFreezeState state = DriveFreezeState::Unknown;
            if (fileFrozen == bootFrozen) {
                state = fileFrozen ? DriveFreezeState::Frozen : DriveFreezeState::Unfrozen;
            }
            else {
                state = bootFrozen ? DriveFreezeState::PendingUnfreeze : DriveFreezeState::PendingFreeze;
            }

            status.disks[letter] = { state };
        }
    }
    return status;
}

// ========== Direct access to driver statistics ==========

std::optional<FreezePassThrough> HFreezeDriver::GetPassThrough() const {
    return m_driver.QueryPassThrough();
}

std::optional<FreezeOldDriverQuality> HFreezeDriver::GetOldDriverQuality() const {
    return m_driver.QueryOldDriverQuality();
}

std::optional<FreezeDiskFull> HFreezeDriver::GetDiskFull() const {
    return m_driver.QueryDiskFull();
}

std::optional<FreezeBsodInfo> HFreezeDriver::GetBsodInfo() const {
    return m_driver.QueryBsodInfo();
}

std::optional<FreezeRedirectData> HFreezeDriver::GetRedirectData() const {
    return m_driver.QueryRedirectData();
}

std::optional<FreezeProtectionState> HFreezeDriver::GetProtectionState() const {
    return m_driver.QueryProtectionState();
}

std::optional<FreezeKeyResult> HFreezeDriver::GetKeyResult() const {
    return m_driver.QueryKeyResult();
}
#endif // !HU_DISABLE_FREEZE_DRIVER