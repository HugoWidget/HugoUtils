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

#include "HugoUtils/HugoUtils.h"
#include <Windows.h>
using namespace std;
namespace HugoUtils {
	// Returns a string containing all currently used drive letters (lowercase).
	std::string GetDrivesInUse()
	{
		auto IsDriveLetterInUse = [](char driveLetter) ->bool {
			char dosDevice[4] = { static_cast<char>(toupper(driveLetter)), ':', '\0' };
			char buf[MAX_PATH] = {};
			return QueryDosDeviceA(dosDevice, buf, MAX_PATH) != 0;
			};
		string res;
		char c = 'a';
		while (c <= 'z') {
			if (IsDriveLetterInUse(c)) res += c;
			c++;
		}
		return res;
	}
	std::string GetLogicalDrives() {
		DWORD drives = ::GetLogicalDrives();
		string str;
		char letter = 'A';
		while (letter <= 'Z') {
			if (drives & 1)str += letter;
			drives >>= 1;
			letter++;
		}
		return str;
	}
}