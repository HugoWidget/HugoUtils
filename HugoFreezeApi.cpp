#include "HugoFreezeApi.h"
#include "Logger.h"
#include <format>
#include <cwctype>
#include <windows.h>
#include <algorithm>
using namespace WinUtils;
using namespace std;
static Logger logger(L"HugoFreezeApi");

wstring ToUpper(wstring_view str) noexcept {
	wstring res(str);
	transform(res.begin(), res.end(), res.begin(), ::towupper);
	return res;
}

FreezeResult HugoFreezeApi::Init() noexcept {
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

void HugoFreezeApi::Cleanup() noexcept {

}
bool HugoFreezeApi::IsInitialized() const noexcept {
	return true;
}

void HugoFreezeApi::SetConfig(const std::wstring& ip, uint16_t port) noexcept {
	m_port = port;
}

void HugoFreezeApi::GetConfig(std::wstring& outIp, uint16_t& outPort) const noexcept {
	outIp = DEFAULT_IP;
	outPort = m_port;
}

DriverStatusInfo HugoFreezeApi::GetDriverStatus() const noexcept {
	return DriverStatusInfo{};
}

FreezeResult HugoFreezeApi::GetFreezeState() const noexcept {
	const std::wstring path = L"/api/v1/get_disk_data";

	logger.DLog(LogLevel::Info, std::format(L"Sending GET request: {}:{}{}", DEFAULT_IP, m_port, path));
	std::wstring response = m_httpClient.getData(DEFAULT_IP, path.c_str(), L"");

	logger.DLog(LogLevel::Debug, std::format(L"GetDiskData response: {}", response));
	return FreezeResult(response != L"" ? FrzOR::Success : FrzOR::Failed)
		.setMsg(response);
}

FreezeResult HugoFreezeApi::TryProtect(const std::wstring& driveLetters) const noexcept {
	wstring lettersUpper;
	lettersUpper = ToUpper(driveLetters);
	if (lettersUpper != L"0" && CalculateVolumeMask(lettersUpper) == -1)
		return FreezeResult(FrzOR::Failed).setErrMsg(L"Invalid drive letters");
	if (lettersUpper == L"0")lettersUpper = L"";
	const std::wstring path = L"/api/v1/excute_protect_try";
	const std::wstring postBody = std::format(L"{{\"selectedDisks\":[\"{}\"]}}", lettersUpper);
	logger.DLog(LogLevel::Info, std::format(L"Sending POST request: {}:{}{}, Request Body: {}", DEFAULT_IP, m_port, path, postBody));
	std::wstring response = m_httpClient.postData(DEFAULT_IP, path.c_str(), postBody.c_str());

	logger.DLog(LogLevel::Debug, std::format(L"PostProtectTry response: {}", response));
	return FreezeResult(response != L"" ? FrzOR::Success : FrzOR::Failed)
		.setMsg(response);
}

FreezeResult HugoFreezeApi::SetFreezeState(
	const std::wstring& driveLetters,
	DriveFreezeState state
) noexcept {
	int vol = driveLetters == L"0" ? 0 : CalculateVolumeMask(driveLetters);
	if (vol == -1)
		return FreezeResult(FrzOR::Failed).setErrMsg(L"Invalid drive letters");
	const std::wstring path = L"/api/v1/set";
	const std::wstring params = L"vol=" + std::to_wstring(vol);

	logger.DLog(LogLevel::Info, std::format(L"Sending GET request: {}:{}{}?{}", DEFAULT_IP, m_port, path, params));
	std::wstring response = m_httpClient.getData(DEFAULT_IP, path.c_str(), params.c_str());

	logger.DLog(LogLevel::Debug, std::format(L"SetFreezeState response: {}", response));
	return FreezeResult(response != L"" ? FrzOR::Success : FrzOR::Failed)
		.setMsg(response);
}

std::wstring HugoFreezeApi::GetLastErrorMsg() const noexcept {
	return L"";
}

DWORD HugoFreezeApi::GetLastErrorCode() const noexcept {
	return 0;
}

HugoFreezeApi::HugoFreezeApi() noexcept {
	m_httpClient.setPort(m_port);
}