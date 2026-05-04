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
#ifndef HU_DISABLE_FREEZE_API

#include <Windows.h>

#include <format>
#include <cwctype>
#include <algorithm>

#include "HugoUtils/HFreezeApi.h"
#include "WinUtils/Logger.h"
#include <WinUtils/StrConvert.h>
#include "HugoUtils/HugoString.h"
using namespace HugoUtils::FreezeApiInvalidStrings;
using namespace WinUtils;
using namespace std;
static Logger logger(L"HFreezeApi");

wstring ToUpper(wstring_view str) noexcept {
	wstring res(str);
	transform(res.begin(), res.end(), res.begin(), ::towupper);
	return res;
}

bool IsValidGBKChar(wchar_t ch) {
	if (ch <= 0x1F || ch == 0x7F) {
		if (ch != 0x09 && ch != 0x0A && ch != 0x0D) return false;
	}
	if (ch == L'\uFFFD') {
		return false;
	}
	const bool isBasicLatin = (ch >= 0x20 && ch <= 0x7E);
	const bool isChineseSymbol = (ch >= 0x3000 && ch <= 0x303F);
	const bool isChineseChar = (ch >= 0x4E00 && ch <= 0x9FA5);
	const bool isFullWidthChar = (ch >= 0xFF00 && ch <= 0xFFEF);
	return isBasicLatin || isChineseSymbol || isChineseChar || isFullWidthChar;
}

std::wstring CleanInvalidGBKChars(const std::wstring& gbk_str) {
	std::wstring result = gbk_str;
	auto valid_end = std::remove_if(
		result.begin(), result.end(),
		[](wchar_t ch) { return !IsValidGBKChar(ch); }
	);
	result.erase(valid_end, result.end());
	return result;
}

FreezeResult HFreezeApi::Init() noexcept {
	HKEY hKey = NULL;
	LONG lResult = RegOpenKeyExW(
		HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\WOW6432Node\\Zeus\\Rpc\\freeze",
		0,
		KEY_READ | KEY_WOW64_64KEY,
		&hKey
	);

	if (lResult != ERROR_SUCCESS) {
		logger.DLog(LogLevel::Warn, std::format(L"Failed to access registry key (Error Code: {}), using default port: {}", lResult, DEFAULT_PORT));
		return FreezeResult(FrzOR::InitFailed);
	}

	DWORD portDword = 0;
	DWORD portBufferSize = sizeof(portDword);
	lResult = RegQueryValueExW(
		hKey,
		L"port",
		NULL,
		NULL,
		(LPBYTE)&portDword,
		&portBufferSize
	);

	if (lResult != ERROR_SUCCESS) {
		logger.DLog(LogLevel::Warn, std::format(L"Failed to read registry Port value (Error Code: {}), using default port: {}", lResult, DEFAULT_PORT));
	}
	else {
		m_port = static_cast<uint16_t>(portDword);
		logger.DLog(LogLevel::Info, std::format(L"Read port from registry: {}", m_port));
	}

	RegCloseKey(hKey);
	logger.DLog(LogLevel::Info, std::format(L"Final configuration: IP={}, Port={}", DEFAULT_IP, m_port));
	return FreezeResult(FrzOR::Success);
}

void HFreezeApi::Cleanup() noexcept {

}
bool HFreezeApi::IsInitialized() const noexcept {
	return true;
}

void HFreezeApi::SetConfig(const std::wstring& ip, uint16_t port) noexcept {
	m_port = port;
}

void HFreezeApi::GetConfig(std::wstring& outIp, uint16_t& outPort) const noexcept {
	outIp = DEFAULT_IP;
	outPort = m_port;
}

FreezeResult HFreezeApi::GetFreezeState() const noexcept {
	const std::wstring path = L"/api/v1/get_disk_data";

	logger.DLog(LogLevel::Info, std::format(L"Sending GET request: {}:{}{}", DEFAULT_IP, m_port, path));
	std::wstring response = m_httpClient.getData(DEFAULT_IP, path.c_str(), L"");

	logger.DLog(LogLevel::Debug, std::format(L"GetDiskData response: {}", response));
	return FreezeResult(response != L"" ? FrzOR::Success : FrzOR::Failed)
		.setMsg(response);
}

FreezeResult HFreezeApi::TryProtect(const std::wstring& driveLetters) const noexcept {
	wstring lettersUpper;
	lettersUpper = ToUpper(driveLetters);
	if (CalculateVolumeMask(lettersUpper) == -1)
		return FreezeResult(FrzOR::Failed).setErrMsg(L"Invalid drive letters");
	const std::wstring path = L"/api/v1/excute_protect_try";
	const std::wstring postBody = std::format(L"{{\"selectedDisks\":[\"{}\"]}}", lettersUpper);
	logger.DLog(LogLevel::Info, std::format(L"Sending POST request: {}:{}{}, Request Body: {}", DEFAULT_IP, m_port, path, postBody));
	std::wstring response = m_httpClient.postData(DEFAULT_IP, path.c_str(), postBody.c_str());

	logger.DLog(LogLevel::Debug, std::format(L"PostProtectTry response: {}", response));
	return FreezeResult(response != L"" ? FrzOR::Success : FrzOR::Failed)
		.setMsg(response);
}

FreezeResult HFreezeApi::SetFreezeState(
	const std::wstring& driveLetters
) noexcept {
	int vol = CalculateVolumeMask(driveLetters);
	if (vol == -1)
		return FreezeResult(FrzOR::Failed).setErrMsg(L"Invalid drive letters");
	const std::wstring path = L"/api/v1/set";
	const std::wstring params = L"vol=" + std::to_wstring(vol);

	logger.DLog(LogLevel::Info, std::format(L"Sending GET request: {}:{}{}?{}", DEFAULT_IP, m_port, path, params));

	std::wstring response = Utf8ToWide(WideToAnsi(m_httpClient.getData(DEFAULT_IP, path.c_str(), params.c_str())));
	response = CleanInvalidGBKChars(response);
	size_t pos = response.find(Str1);
	if (pos != wstring::npos)response = response.substr(0, pos + 3) + Str3 + response.substr(pos + 4);
	size_t pos2 = response.find(Str2);
	if (pos2 != wstring::npos)response = response.substr(0, pos2 + 3) + Str3 + response.substr(pos2 + 4);
	logger.DLog(LogLevel::Debug, std::format(L"SetFreezeState response: {}", response));
	return FreezeResult(response != L"" ? FrzOR::Success : FrzOR::Failed)
		.setMsg(response);
}

std::wstring HFreezeApi::GetLastErrorMsg() const noexcept {
	return L"";
}

DWORD HFreezeApi::GetLastErrorCode() const noexcept {
	return 0;
}

HFreezeApi::HFreezeApi() noexcept {
	m_httpClient.setPort(m_port);
}
#endif // !HU_DISABLE_FREEZE_API