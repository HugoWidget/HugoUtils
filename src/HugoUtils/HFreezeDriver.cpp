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
#ifndef HU_DISABLE_FREEZE_DRIVER

#include <fstream>
#include <format>
#include <algorithm>

#include "HugoUtils/HFreezeDriver.h"
#include "WinUtils/Logger.h"
#include "hashlib/md5.h"
#include "HugoUtils/HugoUtils.h"
#include <WinUtils/StrConvert.h>
using namespace WinUtils;
using namespace HugoUtils;
using namespace std;
static Logger logger(L"HFreezeDriver");

HFreezeDriver& HFreezeDriver::Instance() noexcept {
	static HFreezeDriver instance;
	return instance;
}

FreezeResult HFreezeDriver::Init() noexcept {
	if (IsInitialized())return FreezeResult(FrzOR::Success);
	QueryResult result;
	m_hDriver = OpenDriver();
	if (m_hDriver == INVALID_HANDLE_VALUE) {
		result.lastError = GetLastError();
		result.driverOpenSuccess = false;
		logger.DLog(LogLevel::Error, format(L"[HugoFreeze] Open driver fail, err: {}", result.lastError));
		m_hDriver = NULL;
	}
	result.driverOpenSuccess = true;
	return FreezeResult(result.driverOpenSuccess ? FrzOR::Success : FrzOR::InitFailed).setError(result.lastError);
}

void HFreezeDriver::Cleanup() noexcept {
	if (IsInitialized())CloseHandle(m_hDriver);
}

bool HFreezeDriver::IsInitialized() const noexcept {
	return m_hDriver != NULL && m_hDriver != INVALID_HANDLE_VALUE;
}

QueryResult HFreezeDriver::QueryDriverStatus()const noexcept {
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
		result.runtimeStatus.logStr = wstring(logStr, logStr + strlen(logStr));

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
inline FreezeResult HFreezeDriver::GetFreezeState() const noexcept {
	// 1. 检查驱动是否已初始化
	if (!IsInitialized()) {
		return FreezeResult(FrzOR::NotInitialized, L"Driver not initialized");
	}

	// 2. 查询驱动状态（运行时 + 启动配置）
	QueryResult query = QueryDriverStatus();
	if (!query.driverOpenSuccess) {
		return FreezeResult(FrzOR::DriverError, L"Failed to communicate with driver")
			.setError(query.lastError);
	}

	const unsigned char* buf = query.bootConfig.buffer;
	const int len = query.bootConfig.validLen;
	if (!query.bootConfig.querySuccess || len < 0x90) {
		return FreezeResult(FrzOR::Failed, L"Boot config query failed or invalid length");
	}
	return ParseFreezeBuffer(query);
}

FreezeResult HFreezeDriver::TryProtect(const wstring& driveLetters) const noexcept {
	return FreezeResult(FrzOR::NotSupported);
}

HANDLE HFreezeDriver::OpenDriver()
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

bool HFreezeDriver::ModifyConfig(uint32_t newVol, bool enable) noexcept {
	// Read Config
	ifstream configFile(SWF_CONFIG_PATH, ios::binary | ios::in);
	if (!configFile.is_open()) {
		DWORD err = GetLastError();
		logger.DLog(LogLevel::Error, format(L"[HugoFreeze] Open config fail, err: {}", err));
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
	MD5 md5;
	unsigned char digest[MD5::HashBytes] = { 0 };
	md5.add(buffer + SWF_OFFSET_UINT32_NEXT_MASK, CONFIG_SIZE - SWF_OFFSET_UINT32_NEXT_MASK);
	md5.getHash(digest);
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
			logger.DLog(LogLevel::Error, format(L"[HugoFreeze] IOCTL_PREPARE_WRITE fail, err: {}", err));
		}

		DWORD bytesWritten = 0;
		if (!WriteFile(m_hDriver, buffer, CONFIG_SIZE, &bytesWritten, nullptr)) {
			DWORD err = GetLastError();
			logger.DLog(LogLevel::Error, format(L"[HugoFreeze] Write driver fail, err: {}", err));
			return false;
		}

		// Write to config file
		HANDLE hConfigFile = CreateFileW(SWF_CONFIG_PATH, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
		if (hConfigFile == INVALID_HANDLE_VALUE) {
			DWORD err = GetLastError();
			logger.DLog(LogLevel::Error, format(L"[HugoFreeze] Open config file fail, err: {}", err));
			return false;
		}

		if (!WriteFile(hConfigFile, buffer, CONFIG_SIZE, &bytesWritten, nullptr)) {
			DWORD err = GetLastError();
			logger.DLog(LogLevel::Error, format(L"[HugoFreeze] Write config file fail, err: {}", err));
			CloseHandle(hConfigFile);
			return false;
		}

		CloseHandle(hConfigFile);
	}
	logger.DLog(LogLevel::Info, L"[HugoFreeze] Config modified, reboot to apply");
	return true;
}

inline FreezeResult HFreezeDriver::ParseFreezeBuffer(const unsigned char* buf, int len, QueryResult query) {
	auto read_uint32 = [&](size_t offset) -> uint32_t {
		if (offset + 4 > static_cast<size_t>(len)) return 0;
		return static_cast<uint32_t>(buf[offset]) |
			(static_cast<uint32_t>(buf[offset + 1]) << 8) |
			(static_cast<uint32_t>(buf[offset + 2]) << 16) |
			(static_cast<uint32_t>(buf[offset + 3]) << 24);
		};
	auto read_uint16 = [&](size_t offset) -> uint16_t {
		if (offset + 2 > static_cast<size_t>(len)) return 0;
		return static_cast<uint16_t>(buf[offset]) |
			(static_cast<uint16_t>(buf[offset + 1]) << 8);
		};
	auto read_uint8 = [&](size_t offset) -> uint8_t {
		if (offset >= static_cast<size_t>(len)) return 0;
		return buf[offset];
		};

	// 读取配置字段
	uint32_t next_mask = read_uint32(SWF_OFFSET_UINT32_NEXT_MASK);
	uint8_t flag1 = read_uint8(SWF_OFFSET_UINT8_FLAG1);
	uint16_t status = read_uint16(SWF_OFFSET_UINT16_STATUS);
	uint8_t flag2 = read_uint8(SWF_OFFSET_UINT8_FLAG2);
	uint32_t vol_mask_copy = read_uint32(SWF_OFFSET_UINT32_VOL_MASK_COPY);

	// 设备ID (ASCII, 从0x55开始到0x00)
	string device_id;
	for (size_t i = 0x55; i < static_cast<size_t>(len) && buf[i] != 0; ++i) {
		if (isprint(buf[i])) device_id.push_back(static_cast<char>(buf[i]));
	}
	// 学校代码 (4字节ASCII, 0x68)
	string school_code;
	for (size_t i = 0x68; i < 0x68 + 4 && i < static_cast<size_t>(len); ++i) {
		if (isprint(buf[i])) school_code.push_back(static_cast<char>(buf[i]));
	}

	// MD5 (前16字节)
	string md5_str;
	for (int i = 0; i < 16; ++i) {
		md5_str += format("{:02X}", buf[i]);
	}

	// 4. 根据运行时 activeFlag 和 next_mask 确定当前冻结状态
	bool is_active = (query.runtimeStatus.querySuccess && query.runtimeStatus.activeFlag != 0);
	map<wchar_t, DiskInfo> disk_infos;

	// 遍历 A: ~ Z:
	string drives = HugoUtils::GetLogicalDrives();
	for (char letter : drives) {
		int bit = letter - 'A';
		bool frozen_in_config = (next_mask & (1 << bit)) != 0;
		DriveFreezeState state;
		if (is_active && frozen_in_config) {
			state = DriveFreezeState::Frozen;      // 当前已冻结
		}
		else if (!is_active && frozen_in_config) {
			state = DriveFreezeState::PendingFreeze; // 等待下次重启冻结
		}
		else if (is_active && !frozen_in_config) {
			state = DriveFreezeState::Unfrozen;     // 未冻结（且不在保护列表）
		}
		else {
			state = DriveFreezeState::Unfrozen;     // 完全未冻结
		}
		disk_infos[letter] = { state };
	}

	// 5. 构建 FreezeResult，填充 extra 信息
	FreezeResult result(FrzOR::Success, L"Get freeze state completed");
	result.setDiskInfos(disk_infos);
	result.extra.md5 = md5_str;
	result.extra.next_mask = next_mask;
	result.extra.flag1 = flag1;
	result.extra.status = status;
	result.extra.flag2 = flag2;
	result.extra.vol_mask_copy = vol_mask_copy;
	result.extra.device_id = device_id;
	result.extra.school_code = school_code;

	return result;
}

FreezeResult HFreezeDriver::ParseFreezeBuffer(QueryResult query)
{
	return ParseFreezeBuffer(query.bootConfig.buffer, query.bootConfig.validLen, query);
}

wstring HFreezeDriver::HexDump(const unsigned char* data, int len)noexcept {
	wstring content;
	content += L"    Offset | 00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n";
	content += L"    -------+------------------------------------------------\n";

	for (int i = 0; i < len; i += 16) {
		wstring line = format(L"    0x{:04X} | ", i);
		for (int j = 0; j < 16; j++) {
			if (i + j < len) {
				line += format(L"{:02X} ", data[i + j]);
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

wstring HFreezeDriver::FormatFreezeStateResult(const FreezeResult& result) {
	wstring output;

	if (result.result != FrzOR::Success) {
		output += format(L"获取状态失败: {}\n", result.msg);
		if (result.error != ERROR_SUCCESS) {
			output += format(L"错误码: {}\n", result.error);
		}
		return output;
	}

	// 基础信息
	output += L"========== 希沃驱动冻结状态 ==========\n";
	output += format(L"操作时间: {}\n", result.operateTime.empty() ? L"未知" : result.operateTime);

	// Extra 详细信息
	output += L"\n--- 启动配置详情 ---\n";
	output += format(L"MD5校验和: {}\n", ConvertString(result.extra.md5));
	output += format(L"下次启动掩码: 0x{:08X} -> ", result.extra.next_mask);
	// 解码盘符
	bool first = true;
	for (char c = 'A'; c <= 'Z'; ++c) {
		if (result.extra.next_mask & (1 << (c - 'A'))) {
			if (!first) output += L", ";
			output += format(L"{}:", c);
			first = false;
		}
	}
	if (result.extra.next_mask == 0) output += L"无";
	output += L"\n";
	output += format(L"备份掩码: 0x{:08X}\n", result.extra.vol_mask_copy);
	output += format(L"标志位1: 0x{:02X}  状态字: 0x{:04X}  标志位2: 0x{:02X}\n",
		result.extra.flag1, result.extra.status, result.extra.flag2);
	output += format(L"设备ID: {}\n", result.extra.device_id.empty() ? L"(无)" : wstring(result.extra.device_id.begin(), result.extra.device_id.end()));
	output += format(L"学校代码: {}\n", result.extra.school_code.empty() ? L"(无)" : wstring(result.extra.school_code.begin(), result.extra.school_code.end()));

	// 当前冻结状态
	output += L"\n--- 当前冻结状态 ---\n";
	if (result.diskInfos.empty()) {
		output += L"未检测到任何冻结盘符\n";
	}
	else {
		for (const auto& [drive, info] : result.diskInfos) {
			wstring state_str;
			switch (info.state) {
			case DriveFreezeState::Frozen:         state_str = L"已冻结"; break;
			case DriveFreezeState::Unfrozen:       state_str = L"未冻结"; break;
			case DriveFreezeState::PendingFreeze:  state_str = L"待冻结(重启后)"; break;
			case DriveFreezeState::PendingUnfreeze:state_str = L"待解冻(重启后)"; break;
			default:                               state_str = L"未知"; break;
			}
			output += format(L"盘符 {}: {}\n", drive, state_str);
		}
	}

	output += L"========================================\n";
	return output;
}

wstring HFreezeDriver::FormatFreezeStateResult(
	const FreezeResult& result,
	uint32_t activeFlag,
	uint64_t ptr1,
	const wstring& logStr
) {
	wstring output;

	if (result.result != FrzOR::Success) {
		output += format(L"获取状态失败: {}\n", result.msg);
		if (result.error != ERROR_SUCCESS) {
			output += format(L"错误码: {}\n", result.error);
		}
		return output;
	}

	// ========== 标题 ==========
	output += L"========== 希沃驱动冻结状态 ==========\n";
	output += format(L"操作时间: {}\n", result.operateTime.empty() ? L"未知" : result.operateTime);

	// ========== 运行时状态 ==========
	output += L"\n--- 运行时状态 ---\n";
	output += format(L"激活状态: {} ({})\n", activeFlag, activeFlag ? L"已激活" : L"未激活");
	output += format(L"Pointer1: 0x{:X}\n", ptr1);
	output += format(L"最后日志: {}\n", logStr.empty() ? L"(无)" : logStr);

	// ========== 启动配置详情 ==========
	output += L"\n--- 启动配置详情 ---\n";
	output += format(L"MD5校验和: {}\n", ConvertString(result.extra.md5));
	output += format(L"下次启动掩码: 0x{:08X} -> ", result.extra.next_mask);
	// 解码盘符
	bool first = true;
	for (char c = 'A'; c <= 'Z'; ++c) {
		if (result.extra.next_mask & (1 << (c - 'A'))) {
			if (!first) output += L", ";
			output += format(L"{}:", c);
			first = false;
		}
	}
	if (result.extra.next_mask == 0) output += L"无";
	output += L"\n";
	output += format(L"备份掩码: 0x{:08X}\n", result.extra.vol_mask_copy);
	output += format(L"标志位1: 0x{:02X}  状态字: 0x{:04X}  标志位2: 0x{:02X}\n",
		result.extra.flag1, result.extra.status, result.extra.flag2);
	output += format(L"设备ID: {}\n", result.extra.device_id.empty() ? L"(无)" : wstring(result.extra.device_id.begin(), result.extra.device_id.end()));
	output += format(L"学校代码: {}\n", result.extra.school_code.empty() ? L"(无)" : wstring(result.extra.school_code.begin(), result.extra.school_code.end()));

	// ========== 当前冻结状态 ==========
	output += L"\n--- 当前冻结状态 ---\n";
	if (result.diskInfos.empty()) {
		output += L"未检测到任何冻结盘符\n";
	}
	else {
		for (const auto& [drive, info] : result.diskInfos) {
			wstring state_str;
			switch (info.state) {
			case DriveFreezeState::Frozen:         state_str = L"已冻结"; break;
			case DriveFreezeState::Unfrozen:       state_str = L"未冻结"; break;
			case DriveFreezeState::PendingFreeze:  state_str = L"待冻结(重启后)"; break;
			case DriveFreezeState::PendingUnfreeze:state_str = L"待解冻(重启后)"; break;
			default:                               state_str = L"未知"; break;
			}
			output += format(L"盘符 {}: {}\n", drive, state_str);
		}
	}

	output += L"========================================\n";
	return output;
}

FreezeResult HFreezeDriver::SetFreezeState(
	const wstring& driveLetters
) noexcept {
	if (driveLetters.empty()) {
		logger.DLog(LogLevel::Info, L"[HugoFreeze] Unfreeze all drives");
		bool res = ModifyConfig(0, false);
		return FreezeResult(res ? FrzOR::Success : FrzOR::Failed);
	}
	uint32_t volMask = CalculateVolumeMask(driveLetters);
	if (volMask == -1) {
		logger.DLog(LogLevel::Error, format(L"[HugoFreeze] Invalid drive letters: {}", driveLetters));
		return FreezeResult(FrzOR::Failed).setErrMsg(L"Invalid drive letters");
	}
	logger.DLog(LogLevel::Info, format(L"[HugoFreeze] Freeze drives: {} (mask=0x{:08X})", driveLetters, volMask));
	bool res = ModifyConfig(volMask, true);
	return FreezeResult(res ? FrzOR::Success : FrzOR::Failed);
}

wstring HFreezeDriver::GetLastErrorMsg() const noexcept {
	return L"";
}
DWORD HFreezeDriver::GetLastErrorCode() const noexcept {
	return 0;
}
#endif // !HU_DISABLE_FREEZE_DRIVER