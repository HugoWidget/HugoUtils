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
#include "HugoUtils/HFreezeInterface.h"
#ifndef HU_DISABLE_FREEZE

FreezeResult::FreezeResult(FreezeOperationResult res,
	const std::wstring& message,
	const DWORD err,
	const std::wstring& errorMessage,
	const std::map<wchar_t, DiskInfo>& diskInfos,
	const std::wstring& time,
	const ProtectInfo& protectConfig)
	: result(res), msg(message), error(err), errMsg(errorMessage),
	diskInfos(diskInfos), operateTime(time), protectConfig(protectConfig), hasProtectConfig(true) {
}

FreezeResult& FreezeResult::setResult(FreezeOperationResult res) {
	result = res;
	return *this;
}
FreezeResult& FreezeResult::setMsg(const std::wstring& message) {
	msg = message;
	return *this;
}
FreezeResult& FreezeResult::setError(const DWORD err) {
	error = err;
	return *this;
}
FreezeResult& FreezeResult::setErrMsg(const std::wstring& errorMessage) {
	errMsg = errorMessage;
	return *this;
}
FreezeResult& FreezeResult::setDiskInfos(const std::map<wchar_t, DiskInfo>& infos) {
	diskInfos = infos;
	return *this;
}
FreezeResult& FreezeResult::setOperateTime(const std::wstring& time) {
	operateTime = time;
	return *this;
}

FreezeResult& FreezeResult::setProtectConfig(const ProtectInfo& config)
{
	protectConfig = config;
	hasProtectConfig = true;
	return *this;
}

uint32_t CalculateVolumeMask(const std::wstring& driveLetters) noexcept {
	if (driveLetters.empty()) return 0;
	uint32_t mask = 0;
	for (wchar_t ch : driveLetters) {
		if (ch >= L'A' && ch <= L'Z') {
			mask |= (1u << (ch - L'A'));
		}
		else if (ch >= L'a' && ch <= L'z') {
			mask |= (1u << (ch - L'a'));
		}
		else {
			return static_cast<uint32_t>(-1);
		}
	}
	return mask;
}

#endif // !HU_DISABLE_FREEZE