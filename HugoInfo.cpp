#include "HugoInfo.h"
#include <Windows.h>
#include <iostream>
#include <filesystem>
#include <string_view>
#include <optional>
#include "StrConvert.h"
#include "Logger.h"
using namespace WinUtils;
using namespace std;
namespace fs = std::filesystem;

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
		WLog(LogLevel::Error, format(L"查找目录失败:{}",AnsiToWideString(e.what())));
	}

	return std::nullopt;
}

std::optional<std::wstring> HugoInfo::getHugoVersion() {
	const fs::path baseDir = L"C:\\Program Files (x86)\\Seewo\\SeewoService";
	constexpr std::wstring_view prefix = L"SeewoService_";

	auto foundPath = FindSpecificDir(baseDir, prefix);
	if (!foundPath) {
		WLog(LogLevel::Warn, format(L"未找到 SeewoService 目录"));
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
		WLog(LogLevel::Warn, format(L"未找到 SeewoService 目录"));
		return std::nullopt;
	}

	return foundPath->wstring() + L'\\';
}