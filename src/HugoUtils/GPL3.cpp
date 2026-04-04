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
#ifndef HU_DISABLE_GPL3

#include <Windows.h>

#include <fstream>
#include <string>

#include "HugoUtils/GPL3.h"
void ShowWarranty() {
	const wchar_t* warranty =
		L"THERE IS NO WARRANTY FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.\n"
		L"EXCEPT WHEN OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n"
		L"PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,\n"
		L"INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS\n"
		L"FOR A PARTICULAR PURPOSE. THE ENTIRE RISK AS TO THE QUALITY AND PERFORMANCE OF THE\n"
		L"PROGRAM IS WITH YOU. SHOULD THE PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL\n"
		L"NECESSARY SERVICING, REPAIR OR CORRECTION.\n";
	std::wcout << warranty;
}

int GetConsoleScreenRows() {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
		return csbi.srWindow.Bottom - csbi.srWindow.Top;
	}
	return 24;
}

void ShowLicense(const wchar_t* licensePath) {
	std::wifstream licenseFile(licensePath);
	if (!licenseFile.is_open()) {
		std::wcerr << L"Error: Failed to open license file [" << licensePath << L"]\n";
		std::wcerr << L"Please verify the file path is correct and the program has read permissions.\n";
		return;
	}

	const int screenRows = GetConsoleScreenRows() - 1;
	std::wstring line;
	int currentRow = 0;

	while (std::getline(licenseFile, line)) {
		std::wcout << line << L'\n';
		currentRow++;

		if (currentRow >= screenRows) {
			std::wcout << L"\n-- Press Enter to continue (enter q to quit) -- ";
			std::wstring input;
			std::getline(std::wcin, input);

			if (input == L"q" || input == L"Q") {
				std::wcout << L"\nExited license text display\n";
				licenseFile.close();
				return;
			}

			currentRow = 0;
			std::wcout << L'\n';
		}
	}

	licenseFile.close();
}
#endif // !HU_DISABLE_GPL3