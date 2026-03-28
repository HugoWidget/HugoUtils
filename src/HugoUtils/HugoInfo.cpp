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
#include "WinUtils/WinPch.h"

#include <Windows.h>

#include <iostream>
#include <filesystem>
#include <string_view>
#include <optional>

#include "HugoUtils/HugoInfo.h"
#include "WinUtils/StrConvert.h"
#include "WinUtils/Logger.h"
using namespace WinUtils;
using namespace std;
namespace fs = std::filesystem;
static Logger logger(L"HugoInfo");

std::optional<std::wstring> HugoInfo::getHugoProtectDriverFolder()
{
	auto hugoFolder = getHugoFolder();
	if (!hugoFolder)
		return std::nullopt;
	fs::path driverFolder = *hugoFolder + L"SeewoDriverService";
	if (!fs::is_directory(driverFolder))
		return std::nullopt;
	return driverFolder.wstring() + L'\\';
}

std::optional<std::wstring> HugoInfo::getHugoProtectDriverPath()
{
	auto driverFolder = getHugoProtectDriverFolder();
	if (!driverFolder)
		return std::nullopt;
	fs::path driverPath = *driverFolder + L"DriverService.exe";
	if (!fs::is_regular_file(driverPath))
		return std::nullopt;
	return driverPath.wstring();
}

std::optional<fs::path> HugoInfo::FindSpecificDir(
	const fs::path& baseDir,
	std::wstring_view prefix
) {
	try {
		for (const auto& entry : fs::directory_iterator(baseDir)) {
			if (entry.is_directory()) {
				const auto& dirName = entry.path().filename().wstring();
				if (dirName.compare(0, prefix.size(), prefix) == 0) {
					return entry.path();
				}
			}
		}
	}
	catch (const fs::filesystem_error& e) {
		logger.Error(format(L"Failed to find directory:{}", ConvertString<wstring>(e.what())));
	}

	return std::nullopt;
}

std::optional<std::wstring> HugoInfo::getHugoVersion() {
	const fs::path baseDir = L"C:\\Program Files (x86)\\Seewo\\SeewoService";
	constexpr std::wstring_view prefix = L"SeewoService_";

	auto foundPath = FindSpecificDir(baseDir, prefix);
	if (!foundPath) {
		logger.Warn(format(L"SeewoService directory not found"));
		return std::nullopt;
	}

	std::wstring dirName = foundPath->filename().wstring();
	size_t underscorePos = dirName.find(L'_');
	if (underscorePos != std::wstring::npos && underscorePos + 1 < dirName.size()) {
		return dirName.substr(underscorePos + 1);
	}

	return std::nullopt;
}

std::optional<std::wstring> HugoInfo::getHugoFolder() {
	const fs::path baseDir = L"C:\\Program Files (x86)\\Seewo\\SeewoService";
	constexpr std::wstring_view prefix = L"SeewoService_";

	auto foundPath = FindSpecificDir(baseDir, prefix);
	if (!foundPath) {
		logger.Warn(format(L"SeewoService directory not found"));
		return std::nullopt;
	}

	return foundPath->wstring() + L'\\';
}