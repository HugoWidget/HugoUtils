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
#include "WinUtils/WinPch.h"

#include <Windows.h>

#include <cstdint>
#include <string>

#include "HugoUtils/HugoFreezeInterface.h"

// Driver runtime status structure
struct DriverRuntimeStatus {
	bool querySuccess = false;
	uint32_t activeFlag = 0;
	uint64_t ptr1 = 0;
	std::wstring logStr;
};

// Driver boot configuration structure
struct DriverBootConfig {
	bool querySuccess = false;
	unsigned char buffer[1024] = { 0 };
	int validLen = 0;
};

// Driver query result structure
struct QueryResult {
	DriverRuntimeStatus runtimeStatus;
	DriverBootConfig bootConfig;
	bool driverOpenSuccess = false;
	DWORD lastError = 0;
};

// SWFreeze Driver Management Class
class HugoFreezeDriver :public IHugoFreeze {
public:
	static HugoFreezeDriver& Instance() noexcept;

	FreezeResult Init() noexcept;
	void Cleanup() noexcept;
	bool IsInitialized() const noexcept;

	// Configuration management
	QueryResult QueryDriverStatus() noexcept;

	// Core functionalities
	FreezeResult GetFreezeState() const noexcept;
	FreezeResult TryProtect(const std::wstring& driveLetters) const noexcept;
	FreezeResult SetFreezeState(
		const std::wstring& driveLetters,
		DriveFreezeState state
	) noexcept;

	virtual std::wstring GetLastErrorMsg() const noexcept;
	virtual DWORD GetLastErrorCode() const noexcept;

	static std::wstring HexDump(const unsigned char* data, int len)noexcept;
private:
	HANDLE m_hDriver = NULL;
	HANDLE OpenDriver();
	bool ModifyConfig(uint32_t newVol, bool enable) noexcept;
	HugoFreezeDriver() = default;
	~HugoFreezeDriver() = default;
	HugoFreezeDriver(const HugoFreezeDriver&) = delete;
	HugoFreezeDriver(HugoFreezeDriver&&) = delete;
	HugoFreezeDriver& operator=(const HugoFreezeDriver&) = delete;
	HugoFreezeDriver& operator=(HugoFreezeDriver&&) = delete;
private:
	static constexpr uint32_t SWF_OFFSET_UINT32_NEXT_MASK = 0x10;
	static constexpr uint8_t  SWF_OFFSET_UINT8_FLAG1 = 0x2D;
	static constexpr uint16_t SWF_OFFSET_UINT16_STATUS = 0x31;
	static constexpr uint8_t  SWF_OFFSET_UINT8_FLAG2 = 0x6D;
	static constexpr uint32_t SWF_OFFSET_UINT32_VOL_MASK_COPY = 0x8C;

	static constexpr DWORD IOCTL_QUERY_RUNTIME = 0x80002038;
	static constexpr DWORD IOCTL_READ_MEM_CONF = 0x80002008;
	static constexpr DWORD IOCTL_PREPARE_WRITE = 0x80002064;

	static constexpr const wchar_t* DRIVER_DEVICE_PATH = L"\\\\.\\SWFreeze";
	static constexpr const wchar_t* SWF_CONFIG_PATH = L"C:\\ProgramData\\SeewoFreezeKernelConfig\\VolumeInfo.config";
	static constexpr size_t        CONFIG_SIZE = 1024;

	static constexpr uint32_t OFF_RT_PTR1 = 0x04;
	static constexpr uint32_t OFF_RT_LOG = 0x0C;
	static constexpr uint32_t OFF_RT_ACTIVE = 0x114;
	static constexpr uint32_t OFF_RT_STATS = 0x118;

};