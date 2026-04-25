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
#include "HugoUtilsDef.h"
#ifndef HU_DISABLE_FREEZE_DRIVER

#include <Windows.h>

#include <cstdint>
#include <string>

#include "HugoUtils/HFreezeInterface.h"

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
class HFreezeDriver :public IHugoFreeze {
public:
	static HFreezeDriver& Instance() noexcept;

	FreezeResult Init() noexcept;
	void Cleanup() noexcept;
	bool IsInitialized() const noexcept;

	// Configuration management
	QueryResult QueryDriverStatus() const noexcept;

	// Core functionalities
	FreezeResult GetFreezeState() const noexcept;
	FreezeResult TryProtect(const std::wstring& driveLetters) const noexcept;
	FreezeResult SetFreezeState(
		const std::wstring& driveLetters
	) noexcept;

	virtual std::wstring GetLastErrorMsg() const noexcept;
	virtual DWORD GetLastErrorCode() const noexcept;
	static FreezeResult ParseFreezeBuffer(const unsigned char* buf, int len, QueryResult query);
	static FreezeResult ParseFreezeBuffer(QueryResult query);
	static std::wstring HexDump(const unsigned char* data, int len)noexcept;
	static std::wstring FormatFreezeStateResult(const FreezeResult& result);
	static std::wstring FormatFreezeStateResult(
		const FreezeResult& result,
		uint32_t activeFlag,
		uint64_t ptr1,
		const std::wstring& logStr
	);
private:
	HANDLE m_hDriver = NULL;
	HANDLE OpenDriver();
	bool ModifyConfig(uint32_t newVol, bool enable) noexcept;
	HFreezeDriver() = default;
	~HFreezeDriver() = default;
	HFreezeDriver(const HFreezeDriver&) = delete;
	HFreezeDriver(HFreezeDriver&&) = delete;
	HFreezeDriver& operator=(const HFreezeDriver&) = delete;
	HFreezeDriver& operator=(HFreezeDriver&&) = delete;
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
#endif // !HU_DISABLE_FREEZE_DRIVER