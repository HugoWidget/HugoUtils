#include "GPL3.h"
#include <fstream>
#include <Windows.h>
#include <string>

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
		std::wcerr << L"错误：无法打开许可证文件 [" << licensePath << L"]\n";
		std::wcerr << L"请确认文件路径正确，且程序有读取权限。\n";
		return;
	}

	const int screenRows = GetConsoleScreenRows() - 1;
	std::wstring line;
	int currentRow = 0;

	while (std::getline(licenseFile, line)) {
		std::wcout << line << L'\n';
		currentRow++;

		if (currentRow >= screenRows) {
			std::wcout << L"\n-- 按回车继续 (输入 q 退出) -- ";
			std::wstring input;
			std::getline(std::wcin, input);

			if (input == L"q" || input == L"Q") {
				std::wcout << L"\n已退出许可证文本显示\n";
				licenseFile.close();
				return;
			}

			currentRow = 0;
			std::wcout << L'\n';
		}
	}

	licenseFile.close();
}
