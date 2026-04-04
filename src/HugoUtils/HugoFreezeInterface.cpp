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
#ifndef HU_DISABLE_FREEZE
#include "HugoUtils/HugoFreezeInterface.h"
using namespace std;
uint32_t CalculateVolumeMask(const wstring& driveLetters)noexcept {
	uint32_t mask = 0;
	for (wchar_t c : driveLetters) {
		wchar_t upperC = static_cast<wchar_t>(toupper(static_cast<wint_t>(c)));
		if (upperC < L'A' || upperC > L'Z')  return -1;
		int offset = upperC - L'A';
		if (offset >= 32)return -1;
		mask |= (1 << offset);
	}
	return mask;
}

FreezeResult::FreezeResult(FreezeOperationResult res, const wstring& message, const DWORD err, const wstring& errorMessage, const vector<DiskInfo> diskInfos, const wstring& time)
	: result(res), msg(message), error(err), errMsg(errorMessage), diskInfos(diskInfos), operateTime(time) {
}

FreezeResult& FreezeResult::setResult(FreezeOperationResult res) {
	this->result = res;
	return *this;
}
FreezeResult& FreezeResult::setMsg(const wstring& message) {
	this->msg = message;
	return *this;
}
FreezeResult& FreezeResult::setError(const DWORD error){
	this->error = error;
	return *this;
}
FreezeResult& FreezeResult::setErrMsg(const wstring& errorMessage) {
	this->errMsg = errorMessage;
	return *this;
}
FreezeResult& FreezeResult::setDiskInfos(const vector<DiskInfo>& diskInfos) {
	this->diskInfos = diskInfos;
	return *this;
}
FreezeResult& FreezeResult::setOperateTime(const wstring& time) {
	this->operateTime = time;
	return *this;
}
#endif // !HU_DISABLE_FREEZE