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

bool HugoFreezeDriver::FreezeDrives(const std::wstring& driveLetters) noexcept {
	uint32_t volMask = CalculateVolumeMask(driveLetters);
	if (volMask == 0) {
		logger.DLog(LogLevel::Error, std::format(L"[HugoFreeze] Invalid drive letters: {}", driveLetters));
		return false;
	}

	logger.DLog(LogLevel::Info, std::format(L"[HugoFreeze] Freeze drives: {} (mask=0x{:08X})", driveLetters, volMask));
	return ModifyConfig(volMask, true);
}

bool HugoFreezeDriver::UnfreezeAll() noexcept {
	logger.DLog(LogLevel::Info, L"[HugoFreeze] Unfreeze all drives");
	return ModifyConfig(0, false);
}

QueryResult HugoFreezeDriver::QueryDriverStatus() noexcept {
	QueryResult result;
	HANDLE hDevice = CreateFileW(
		DRIVER_DEVICE_PATH,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		0,
		nullptr
	);

	if (hDevice == INVALID_HANDLE_VALUE) {
		result.lastError = GetLastError();
		result.driverOpenSuccess = false;
		logger.DLog(LogLevel::Error, std::format(L"[HugoFreeze] Open driver fail, err: {}", result.lastError));
		return result;
	}
	result.driverOpenSuccess = true;

	// 查询运行时状态
	unsigned char buf[CONFIG_SIZE] = { 0 };
	DWORD bytesReturned = 0;
	if (DeviceIoControl(hDevice, IOCTL_QUERY_RUNTIME, nullptr, 0, buf, CONFIG_SIZE, &bytesReturned, nullptr)) {
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
	if (DeviceIoControl(hDevice, IOCTL_READ_MEM_CONF, nullptr, 0, buf, CONFIG_SIZE, &bytesReturned, nullptr)) {
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

	CloseHandle(hDevice);
	return result;
}


bool HugoFreezeDriver::ModifyConfig(uint32_t newVol, bool enable) noexcept {
	// 读取配置文件
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

	// 修改配置
	*reinterpret_cast<uint32_t*>(buffer + SWF_OFFSET_UINT32_NEXT_MASK) = newVol;
	buffer[SWF_OFFSET_UINT8_FLAG1] = 0x02;
	*reinterpret_cast<uint16_t*>(buffer + SWF_OFFSET_UINT16_STATUS) = enable ? 0x0000 : 0x03E4;
	buffer[SWF_OFFSET_UINT8_FLAG2] = 0x01;
	*reinterpret_cast<uint32_t*>(buffer + SWF_OFFSET_UINT32_VOL_MASK_COPY) = newVol;

	// 计算MD5
	MD5_CTX ctx;
	unsigned char digest[MD5_DIGEST_LENGTH] = { 0 };
	MD5_Init(&ctx);
	MD5_Update(&ctx, buffer + SWF_OFFSET_UINT32_NEXT_MASK, CONFIG_SIZE - SWF_OFFSET_UINT32_NEXT_MASK);
	MD5_Final(digest, &ctx);
	memcpy(buffer, digest, 16);

	logger.DLog(LogLevel::Debug, L"[HugoFreeze] Config buffer ready");
	HexDump(buffer, 0x90);

	// 写入驱动
	HANDLE hDriver = CreateFileW(DRIVER_DEVICE_PATH, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
	if (hDriver == INVALID_HANDLE_VALUE) {
		DWORD err = GetLastError();
		logger.DLog(LogLevel::Error, std::format(L"[HugoFreeze] Open driver fail, err: {}", err));
		return false;
	}

	DWORD bytesReturned = 0;
	unsigned char dummyBuffer[1024] = { 0 };
	if (DeviceIoControl(hDriver, IOCTL_PREPARE_WRITE, nullptr, 0, dummyBuffer, 1024, &bytesReturned, nullptr)) {
		logger.DLog(LogLevel::Debug, L"[HugoFreeze] IOCTL_PREPARE_WRITE success");
	}
	else {
		DWORD err = GetLastError();
		logger.DLog(LogLevel::Error, std::format(L"[HugoFreeze] IOCTL_PREPARE_WRITE fail, err: {}", err));
	}

	DWORD bytesWritten = 0;
	if (!WriteFile(hDriver, buffer, CONFIG_SIZE, &bytesWritten, nullptr)) {
		DWORD err = GetLastError();
		logger.DLog(LogLevel::Error, std::format(L"[HugoFreeze] Write driver fail, err: {}", err));
		CloseHandle(hDriver);
		return false;
	}
	CloseHandle(hDriver);

	// 写入配置文件
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
	logger.DLog(LogLevel::Info, L"[HugoFreeze] Config modified, reboot to apply");
	return true;
}

void HugoFreezeDriver::HexDump(const unsigned char* data, int len) const noexcept {
	logger.DLog(LogLevel::Debug, L"    Offset | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F");
	logger.DLog(LogLevel::Debug, L"    -------+------------------------------------------------");

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
		logger.DLog(LogLevel::Debug, line);
	}
}
