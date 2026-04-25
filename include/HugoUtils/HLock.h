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
#include <windows.h>
#include <atomic>
#include <string>

class SharedFlag {
public:
	explicit SharedFlag(const std::wstring& name)
		: name_(name) {
		hMap_ = CreateFileMappingW(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
			0, sizeof(BOOL), name_.c_str());
		if (hMap_) {
			mapped_ = MapViewOfFile(hMap_, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(BOOL));
			if (mapped_) {
				value_ = static_cast<volatile BOOL*>(mapped_);
				*value_ = FALSE;
			}
		}
	}

	~SharedFlag() {
		if (mapped_) UnmapViewOfFile(mapped_);
		if (hMap_) CloseHandle(hMap_);
	}

	void Set(BOOL val) {
		if (value_) InterlockedExchange((LONG*)value_, val);
	}

	BOOL Get() const {
		return value_ ? *value_ : FALSE;
	}

	bool Valid() const { return value_ != nullptr; }

	SharedFlag(const SharedFlag&) = delete;
	SharedFlag& operator=(const SharedFlag&) = delete;

private:
	std::wstring name_;
	void* mapped_ = nullptr;
	HANDLE hMap_ = nullptr;
	volatile BOOL* value_ = nullptr;
};
