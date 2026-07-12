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
#ifndef HU_DISABLE_FREEZE

#include <Windows.h>
#include <cstdint>
#include <vector>
#include <map>
#include <string>

#include "SWFreezeTypes.h"

enum class FreezeOperationResult : uint8_t {
	Success = 0,        // Operation succeeded
	Failed = 1,         // Operation failed
	InvalidParam = 2,   // Invalid parameter
	DriverError = 3,    // Driver error
	NetworkError = 4,   // Network error
	InitFailed = 5,     // Initialization failed
	NotInitialized = 6, // Not initialized
	NotSupported = 7    // Operation not supported
};
using FrzOR = FreezeOperationResult;

enum class DriveFreezeState : uint8_t {
	Unfrozen = 0,
	Frozen = 1,
	PendingFreeze = 2,
	PendingUnfreeze = 3,
	Unknown = 4
};

struct DiskInfo {
	DriveFreezeState state;
	size_t bytesFree = 0;
	size_t bytesTotal = 0;
};

struct ExtraInfo {
	std::string md5;
	uint32_t next_mask;
	uint8_t flag1;
	uint16_t status;
	uint8_t flag2;
	uint32_t vol_mask_copy;
	std::string device_id;
	std::string school_code;
};

struct FreezeResult {
	FreezeResult(FreezeOperationResult res = FreezeOperationResult::Success,
		const std::wstring& message = L"",
		const DWORD err = ERROR_SUCCESS,
		const std::wstring& errorMessage = L"",
		const std::map<wchar_t, DiskInfo>& diskInfos = {},
		const std::wstring& time = L"",
		const ProtectInfo& protectConfig = ProtectInfo());

	FreezeResult& setResult(FreezeOperationResult res);
	FreezeResult& setMsg(const std::wstring& message);
	FreezeResult& setError(const DWORD error);
	FreezeResult& setErrMsg(const std::wstring& errorMessage);
	FreezeResult& setDiskInfos(const std::map<wchar_t, DiskInfo>& diskInfos);
	FreezeResult& setOperateTime(const std::wstring& time);
	FreezeResult& setProtectConfig(const ProtectInfo& config);

	FreezeOperationResult result;         // Result code
	std::wstring msg;                     // Message
	DWORD error;                          // Error code
	std::wstring errMsg;                  // Error message
	std::map<wchar_t, DiskInfo> diskInfos;// Drive letter status
	std::wstring operateTime;             // Operation time
	ExtraInfo extra;                      // Legacy extra info (retained)
	ProtectInfo protectConfig;            // New full configuration structure
	bool hasProtectConfig = false;        // Flag indicating whether protectConfig is valid
};

class IHugoFreeze {
public:
	virtual ~IHugoFreeze() = default;
	virtual FreezeResult Init() noexcept = 0;
	virtual void Cleanup() noexcept = 0;
	virtual bool IsInitialized() const noexcept = 0;
	virtual FreezeResult GetFreezeState() const noexcept = 0;
	virtual FreezeResult TryProtect(const std::wstring& driveLetters) const noexcept = 0;
	virtual FreezeResult SetFreezeState(const std::wstring& driveLetters) noexcept = 0;
	virtual std::wstring GetLastErrorMsg() const noexcept = 0;
	virtual DWORD GetLastErrorCode() const noexcept = 0;
};
uint32_t CalculateVolumeMask(const std::wstring& driveLetters) noexcept;

#endif // !HU_DISABLE_FREEZE