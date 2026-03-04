#include "HugoFreezeDriver.h"
#include "Logger.h"
#include "md5.h"
#include <fstream>
#include <format>
#include <algorithm>
using namespace WinUtils;
static Logger logger(L"HugoFreezeDriver");

HugoFreezeDriver& HugoFreezeDriver::Instance() noexcept {
	static HugoFreezeDriver instance;
	return instance;
}

FreezeResult HugoFreezeDriver::Init() noexcept {
	if (IsInitialized())return FreezeResult(FrzOR::Success);
	QueryResult result;
	m_hDriver = OpenDriver();
	if (m_hDriver == INVALID_HANDLE_VALUE) {
		result.lastError = GetLastError();
		result.driverOpenSuccess = false;
		logger.DLog(LogLevel::Error, std::format(L"[HugoFreeze] Open driver fail, err: {}", result.lastError));
		m_hDriver = NULL;
	}
	result.driverOpenSuccess = true;
	return FreezeResult(result.driverOpenSuccess ? FrzOR::Success : FrzOR::InitFailed).setError(result.lastError);
}

void HugoFreezeDriver::Cleanup() noexcept {
	if (IsInitialized())CloseHandle(m_hDriver);
}

bool HugoFreezeDriver::IsInitialized() const noexcept {
	return m_hDriver != NULL && m_hDriver != INVALID_HANDLE_VALUE;
}

QueryResult HugoFreezeDriver::QueryDriverStatus() noexcept {
	QueryResult result;
	result.driverOpenSuccess = false;
	if (!IsInitialized())return result;
	result.driverOpenSuccess = true;

	// 查询运行时状态
	unsigned char buf[CONFIG_SIZE] = { 0 };
	DWORD bytesReturned = 0;
	if (DeviceIoControl(m_hDriver, IOCTL_QUERY_RUNTIME, nullptr, 0, buf, CONFIG_SIZE, &bytesReturned, nullptr)) {
		result.runtimeStatus.querySuccess = true;
		result.runtimeStatus.activeFlag = *reinterpret_cast<uint32_t*>(buf + OFF_RT_ACTIVE);
		result.runtimeStatus.ptr1 = *reinterpret_cast<uint64_t*>(buf + OFF_RT_PTR1);

		const char* logStr = reinterpret_cast<char*>(buf + OFF_RT_LOG);
		buf[CONFIG_SIZE - 1] = 0;
		result.runtimeStatus.logStr = std::wstring(logStr, logStr + strlen(logStr));

		logger.DLog(LogLevel::Debug, L"[HugoFreeze] Query runtime status success");
	}
	else {
		result.runtimeStatus.querySuccess = false;
		logger.DLog(LogLevel::Error, L"[HugoFreeze] Query runtime status fail");
	}

	// 查询启动配置
	memset(buf, 0, CONFIG_SIZE);
	if (DeviceIoControl(m_hDriver, IOCTL_READ_MEM_CONF, nullptr, 0, buf, CONFIG_SIZE, &bytesReturned, nullptr)) {
		result.bootConfig.querySuccess = true;
		memcpy(result.bootConfig.buffer, buf, CONFIG_SIZE);
		result.bootConfig.validLen = 0x90;
		logger.DLog(LogLevel::Debug, L"[HugoFreeze] Query boot config success");
		HexDump(buf, 0x90);
	}
	else {
		result.bootConfig.querySuccess = false;
		logger.DLog(LogLevel::Error, L"[HugoFreeze] Query boot config fail");
	}
	return result;
}

// Core functionalities
FreezeResult HugoFreezeDriver::GetFreezeState() const noexcept {
	return FreezeResult(FrzOR::NotSupported);
}
FreezeResult HugoFreezeDriver::TryProtect(const std::wstring& driveLetters) const noexcept {
	return FreezeResult(FrzOR::NotSupported);
}

HANDLE HugoFreezeDriver::OpenDriver()
{
	HANDLE hDevice = CreateFileW(
		DRIVER_DEVICE_PATH,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr
	);
	return hDevice;
}

bool HugoFreezeDriver::ModifyConfig(uint32_t newVol, bool enable) noexcept {
	// Read Config
	std::ifstream configFile(SWF_CONFIG_PATH, std::ios::binary | std::ios::in);
	if (!configFile.is_open()) {
		DWORD err = GetLastError();
		logger.DLog(LogLevel::Error, std::format(L"[HugoFreeze] Open config fail, err: {}", err));
		return false;
	}

	unsigned char buffer[CONFIG_SIZE] = { 0 };
	configFile.read(reinterpret_cast<char*>(buffer), CONFIG_SIZE);
	if (!configFile || configFile.gcount() != CONFIG_SIZE) {
		logger.DLog(LogLevel::Error, L"[HugoFreeze] Invalid config size");
		configFile.close();
		return false;
	}
	configFile.close();

	// Modify Config
	*reinterpret_cast<uint32_t*>(buffer + SWF_OFFSET_UINT32_NEXT_MASK) = newVol;
	buffer[SWF_OFFSET_UINT8_FLAG1] = 0x02;
	*reinterpret_cast<uint16_t*>(buffer + SWF_OFFSET_UINT16_STATUS) = enable ? 0x0000 : 0x03E4;
	buffer[SWF_OFFSET_UINT8_FLAG2] = 0x01;
	*reinterpret_cast<uint32_t*>(buffer + SWF_OFFSET_UINT32_VOL_MASK_COPY) = newVol;

	// Calc MD5
	MD5_CTX ctx;
	unsigned char digest[MD5_DIGEST_LENGTH] = { 0 };
	MD5_Init(&ctx);
	MD5_Update(&ctx, buffer + SWF_OFFSET_UINT32_NEXT_MASK, CONFIG_SIZE - SWF_OFFSET_UINT32_NEXT_MASK);
	MD5_Final(digest, &ctx);
	memcpy(buffer, digest, 16);

	logger.DLog(LogLevel::Debug, L"[HugoFreeze] Config buffer ready");
	HexDump(buffer, 0x90);

	// Write to driver
	if (IsInitialized()) {
		DWORD bytesReturned = 0;
		unsigned char dummyBuffer[1024] = { 0 };
		if (DeviceIoControl(m_hDriver, IOCTL_PREPARE_WRITE, nullptr, 0, dummyBuffer, 1024, &bytesReturned, nullptr)) {
			logger.DLog(LogLevel::Debug, L"[HugoFreeze] IOCTL_PREPARE_WRITE success");
		}
		else {
			DWORD err = GetLastError();
			logger.DLog(LogLevel::Error, std::format(L"[HugoFreeze] IOCTL_PREPARE_WRITE fail, err: {}", err));
		}

		DWORD bytesWritten = 0;
		if (!WriteFile(m_hDriver, buffer, CONFIG_SIZE, &bytesWritten, nullptr)) {
			DWORD err = GetLastError();
			logger.DLog(LogLevel::Error, std::format(L"[HugoFreeze] Write driver fail, err: {}", err));
			return false;
		}

		// Write to config file
		HANDLE hConfigFile = CreateFileW(SWF_CONFIG_PATH, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
		if (hConfigFile == INVALID_HANDLE_VALUE) {
			DWORD err = GetLastError();
			logger.DLog(LogLevel::Error, std::format(L"[HugoFreeze] Open config file fail, err: {}", err));
			return false;
		}

		if (!WriteFile(hConfigFile, buffer, CONFIG_SIZE, &bytesWritten, nullptr)) {
			DWORD err = GetLastError();
			logger.DLog(LogLevel::Error, std::format(L"[HugoFreeze] Write config file fail, err: {}", err));
			CloseHandle(hConfigFile);
			return false;
		}

		CloseHandle(hConfigFile);
	}
	logger.DLog(LogLevel::Info, L"[HugoFreeze] Config modified, reboot to apply");
	return true;
}

std::wstring HugoFreezeDriver::HexDump(const unsigned char* data, int len)noexcept {
	std::wstring content;
	content += L"    Offset | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n";
	content += L"    -------+------------------------------------------------\n";

	for (int i = 0; i < len; i += 16) {
		std::wstring line = std::format(L"    0x{:04X} | ", i);
		for (int j = 0; j < 16; j++) {
			if (i + j < len) {
				line += std::format(L"{:02X} ", data[i + j]);
			}
			else {
				line += L"   ";
			}
		}
		content += line;
		content += L"\n";
	}
	return content;
}

FreezeResult HugoFreezeDriver::SetFreezeState(
	const std::wstring& driveLetters,
	DriveFreezeState state
) noexcept {
	if (driveLetters.empty()) {
		logger.DLog(LogLevel::Info, L"[HugoFreeze] Unfreeze all drives");
		bool res = ModifyConfig(0, false);
		return FreezeResult(res ? FrzOR::Success : FrzOR::Failed);
	}
	uint32_t volMask = CalculateVolumeMask(driveLetters);
	if (volMask == -1) {
		logger.DLog(LogLevel::Error, std::format(L"[HugoFreeze] Invalid drive letters: {}", driveLetters));
		return FreezeResult(FrzOR::Failed).setErrMsg(L"Invalid drive letters");
	}
	logger.DLog(LogLevel::Info, std::format(L"[HugoFreeze] Freeze drives: {} (mask=0x{:08X})", driveLetters, volMask));
	bool res = ModifyConfig(volMask, true);
	return FreezeResult(res ? FrzOR::Success : FrzOR::Failed);
}

std::wstring HugoFreezeDriver::GetLastErrorMsg() const noexcept {
	return L"";
}
DWORD HugoFreezeDriver::GetLastErrorCode() const noexcept {
	return 0;
}

