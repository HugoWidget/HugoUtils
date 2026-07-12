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

#include "HugoUtils/HFreezeDriver_p.h"

static constexpr const wchar_t* DRIVER_PATH = L"\\\\.\\SWFreeze";

// ---- Init / Cleanup ----
bool HFreezeDriverPrivate::Init() {
	m_hDriver = CreateFileW(DRIVER_PATH, GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
		OPEN_EXISTING, 0, nullptr);
	if (m_hDriver == INVALID_HANDLE_VALUE) {
		m_lastError = GetLastError();
		m_lastErrorMsg = L"Failed to open driver device";
		return false;
	}
	return true;
}

void HFreezeDriverPrivate::Cleanup() {
	if (m_hDriver && m_hDriver != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hDriver);
		m_hDriver = nullptr;
	}
}

// ---- Template implementations ----
template<typename T>
std::optional<T> HFreezeDriverPrivate::IoControlOut(DWORD code, size_t minBufSize) const {
	size_t bufSize = (std::max)(minBufSize, sizeof(T));   // avoid macro collision
	std::vector<uint8_t> buf(bufSize, 0);
	DWORD bytesRet = 0;
	if (!DeviceIoControl(m_hDriver, code, nullptr, 0,
		buf.data(), static_cast<DWORD>(buf.size()),
		&bytesRet, nullptr)) {
		m_lastError = GetLastError();
		m_lastErrorMsg = L"DeviceIoControl output failed";
		return std::nullopt;
	}
	return T::FromBuffer(buf.data(), bytesRet);
}

template<typename T>
bool HFreezeDriverPrivate::IoControlIn(DWORD code, const T& data, size_t bufSize) const {
	std::vector<uint8_t> buf((std::max)(bufSize, sizeof(T)), 0);
	data.ToBuffer(buf.data(), buf.size());
	DWORD bytesRet = 0;
	if (!DeviceIoControl(m_hDriver, code, buf.data(),
		static_cast<DWORD>(buf.size()),
		nullptr, 0, &bytesRet, nullptr)) {
		m_lastError = GetLastError();
		m_lastErrorMsg = L"DeviceIoControl input failed";
		return false;
	}
	return true;
}

// ---- Configuration ----
std::optional<ProtectInfo> HFreezeDriverPrivate::QueryBootConfig() const {
	return IoControlOut<ProtectInfo>(0x80002008);
}

bool HFreezeDriverPrivate::WriteBootConfig(const ProtectInfo& config) {
	// 0x80002064 ¨C prepare write
	std::vector<uint8_t> dummy(0x400, 0);
	DWORD bytesRet = 0;
	if (!DeviceIoControl(m_hDriver, 0x80002064, nullptr, 0,
		dummy.data(), static_cast<DWORD>(dummy.size()),
		&bytesRet, nullptr)) {
		m_lastError = GetLastError();
		m_lastErrorMsg = L"IOCTL_PREPARE_WRITE failed";
		return false;
	}

	// Write the new configuration
	uint8_t raw[sizeof(ProtectInfo)];
	config.ToBuffer(raw, sizeof(raw));
	DWORD written = 0;
	if (!WriteFile(m_hDriver, raw, sizeof(raw), &written, nullptr)) {
		m_lastError = GetLastError();
		m_lastErrorMsg = L"WriteFile to driver failed";
		return false;
	}
	return true;
}

// ---- Status queries ----
std::optional<FreezeBootSystem> HFreezeDriverPrivate::QueryBootSystem() const {
	return IoControlOut<FreezeBootSystem>(0x80002038);
}

std::optional<FreezeKeyResult> HFreezeDriverPrivate::QueryKeyResult() const {
	return IoControlOut<FreezeKeyResult>(0x80002028, 0x800);
}

std::optional<FreezeProtectionState> HFreezeDriverPrivate::QueryProtectionState() const {
	return IoControlOut<FreezeProtectionState>(0x8000202C);
}

std::optional<FreezePassThrough> HFreezeDriverPrivate::QueryPassThrough() const {
	return IoControlOut<FreezePassThrough>(0x8000203C);
}

std::optional<FreezeOldDriverQuality> HFreezeDriverPrivate::QueryOldDriverQuality() const {
	return IoControlOut<FreezeOldDriverQuality>(0x80002040);
}

std::optional<FreezeDiskFull> HFreezeDriverPrivate::QueryDiskFull() const {
	return IoControlOut<FreezeDiskFull>(0x80002044);
}

std::optional<FreezeBsodInfo> HFreezeDriverPrivate::QueryBsodInfo() const {
	return IoControlOut<FreezeBsodInfo>(0x80002058);
}

std::optional<FreezeRedirectData> HFreezeDriverPrivate::QueryRedirectData() const {
	return IoControlOut<FreezeRedirectData>(0x80002060);
}

std::optional<HFreezeDriverPrivate::ThreadPoolResult> HFreezeDriverPrivate::QueryThreadPoolAndRedirect() const {
	std::vector<uint8_t> buf(0x828, 0);
	DWORD bytesRet = 0;
	if (!DeviceIoControl(m_hDriver, 0x8000205C, nullptr, 0,
		buf.data(), static_cast<DWORD>(buf.size()),
		&bytesRet, nullptr)) {
		return std::nullopt;
	}

	ThreadPoolResult res;
	if (bytesRet >= sizeof(FreezeRedirectData)) {
		size_t tidCount = (bytesRet - sizeof(FreezeRedirectData)) / sizeof(uint32_t);
		const uint32_t* pTid = reinterpret_cast<const uint32_t*>(buf.data());
		res.threadIds.assign(pTid, pTid + tidCount);
		res.redirectData = FreezeRedirectData::FromBuffer(
			buf.data() + tidCount * sizeof(uint32_t), sizeof(FreezeRedirectData));
	}
	return res;
}

// ---- Input / uploads ----
bool HFreezeDriverPrivate::SetNotifyHandles(const FreezeNotifyHandles& handles) {
	return IoControlIn(0x80002034, handles);
}

bool HFreezeDriverPrivate::SetProcessImage(const FreezeImageInfo& info) {
	return IoControlIn(0x8000204C, info);
}

bool HFreezeDriverPrivate::SetDriverImage(const FreezeImageInfo& info) {
	return IoControlIn(0x80002054, info);
}

// ---- Backdoors ----
void HFreezeDriverPrivate::TriggerBSOD() const {
	DeviceIoControl(m_hDriver, 0x80002190, nullptr, 0, nullptr, 0, nullptr, nullptr);
}

void HFreezeDriverPrivate::FlushWppLogs() const {
	DeviceIoControl(m_hDriver, 0x80002194, nullptr, 0, nullptr, 0, nullptr, nullptr);
}
#endif // !HU_DISABLE_FREEZE_DRIVER