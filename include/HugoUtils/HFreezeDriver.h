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
#pragma once
#include "HugoUtilsDef.h"
#ifndef HU_DISABLE_FREEZE_DRIVER

#include "HFreezeInterface.h"
#include "HFreezeDriver_p.h"
#include "HFreezeFile_p.h"
#include <memory>
#include <optional>

class HFreezeDriver : public IHugoFreeze {
public:
	HFreezeDriver() = default;

	// IHugoFreeze implementation
	FreezeResult Init() noexcept override;
	void Cleanup() noexcept override;
	bool IsInitialized() const noexcept override;
	FreezeResult GetFreezeState() const noexcept override;
	FreezeResult TryProtect(const std::wstring& driveLetters) const noexcept override;
	FreezeResult SetFreezeState(const std::wstring& driveLetters) noexcept override;
	std::wstring GetLastErrorMsg() const noexcept override { return L""; };
	DWORD GetLastErrorCode() const noexcept override { return 0; };
	// Comprehensive status snapshot (combines file, boot config, runtime data)
	struct FullStatus {
		std::map<wchar_t, DiskInfo> disks;
		std::optional<ProtectInfo> fileConfig;
		std::optional<ProtectInfo> bootConfig;
		std::optional<FreezeBootSystem> bootSystem;
		// Further runtime statistics can be added here as needed
	};
	FullStatus GetFullStatus() const;

	// Direct access to additional driver statistics (if driver is available)
	std::optional<FreezePassThrough>      GetPassThrough() const;
	std::optional<FreezeOldDriverQuality> GetOldDriverQuality() const;
	std::optional<FreezeDiskFull>         GetDiskFull() const;
	std::optional<FreezeBsodInfo>         GetBsodInfo() const;
	std::optional<FreezeRedirectData>     GetRedirectData() const;
	std::optional<FreezeProtectionState>  GetProtectionState() const;
	std::optional<FreezeKeyResult>        GetKeyResult() const;

private:
	mutable HFreezeDriverPrivate m_driver;   // driver communication
};
#endif // !HU_DISABLE_FREEZE_DRIVER