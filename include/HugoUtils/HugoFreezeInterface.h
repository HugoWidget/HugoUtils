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
#include <string>

// Common enumeration definitions
enum class FreezeOperationResult : uint8_t {
	Success = 0,        // Operation succeeded
	Failed = 1,         // Operation failed
	InvalidParam = 2,   // Invalid parameter
	DriverError = 3,    // Driver error
	NetworkError = 4,   // Network error
	InitFailed = 5,     // Initialization failed
	NotInitialized = 6, // Not initialized
	NotSupported = 7     // Not supported
};
using FrzOR = FreezeOperationResult;
enum class DriveFreezeState : uint8_t {
	Frozen = 0,         // Frozen
	Unfrozen = 1,       // Unfrozen
	Unknown = 2         // State unknown
};

// Common structure definitions
struct DiskInfo {
	std::wstring diskId;        // Drive letter (e.g., L"C:")
	DriveFreezeState state;     // Freeze state
};

struct FreezeResult {
	FreezeResult(FreezeOperationResult res, const std::wstring& message = L"",
		const DWORD err = ERROR_SUCCESS, const std::wstring& errorMessage = L"", const std::vector<DiskInfo> diskInfos = {}, const std::wstring& time = L"");
	FreezeResult& setResult(FreezeOperationResult res);
	FreezeResult& setMsg(const std::wstring& message);
	FreezeResult& setError(const DWORD error);
	FreezeResult& setErrMsg(const std::wstring& errorMessage);
	FreezeResult& setDiskInfos(const std::vector<DiskInfo>& diskInfos);
	FreezeResult& setOperateTime(const std::wstring& time);
	FreezeOperationResult result;    // Result of the protect try operation
	std::wstring msg;                // message
	DWORD error;                     // error
	std::wstring errMsg;             // error message
	std::vector<DiskInfo> diskInfos; // List of disk information
	std::wstring operateTime;        // Operation time (format: yyyy-MM-dd HH:mm:ss)
};

class IHugoFreeze {
public:
	virtual ~IHugoFreeze() = default;

	virtual FreezeResult Init() noexcept = 0;
	virtual void Cleanup() noexcept = 0;
	virtual bool IsInitialized() const noexcept = 0;

	// Core functionalities
	virtual FreezeResult GetFreezeState() const noexcept = 0;
	virtual FreezeResult TryProtect(const std::wstring& driveLetters) const noexcept = 0;
	virtual FreezeResult SetFreezeState(
		const std::wstring& driveLetters,
		DriveFreezeState state
	) noexcept = 0;

	virtual std::wstring GetLastErrorMsg() const noexcept = 0;
	virtual DWORD GetLastErrorCode() const noexcept = 0;
};
// return 0 if 'driveLetters' is empty, -1 if 'driveLetters' is invalid
uint32_t CalculateVolumeMask(const std::wstring& driveLetters)noexcept;
#endif // !HU_DISABLE_FREEZE