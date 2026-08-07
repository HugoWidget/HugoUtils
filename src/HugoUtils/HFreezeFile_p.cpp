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
#ifndef HU_DISABLE_FREEZE

#include "HugoUtils/HFreezeFile_p.h"
#include "hashlib/md5.h"
#include "WinUtils/StrConvert.h"
#include <fstream>
using namespace std;
using namespace WinUtils;

 // Initialize the static path with the default value
std::wstring HFreezeFilePrivate::s_configPath = HFreezeFilePrivate::DEFAULT_CONFIG_PATH;

void HFreezeFilePrivate::SetConfigPath(const std::wstring& path) {
	s_configPath = path;
}

const std::wstring& HFreezeFilePrivate::GetConfigPath() {
	return s_configPath;
}

// ---- Read without explicit path (uses s_configPath) ----
std::optional<ProtectInfo> HFreezeFilePrivate::ReadConfig() {
	return ReadConfig(s_configPath);
}

// ---- Read with explicit path ----
std::optional<ProtectInfo> HFreezeFilePrivate::ReadConfig(const std::wstring& path) {
    std::ifstream file(ConvertString<string>(path), std::ios::binary);
	if (!file) return std::nullopt;

	uint8_t raw[CONFIG_SIZE];
	file.read(reinterpret_cast<char*>(raw), CONFIG_SIZE);
	if (file.gcount() != CONFIG_SIZE) return std::nullopt;

	return ProtectInfo::FromBuffer(raw, CONFIG_SIZE);
}

// ---- Write without explicit path ----
bool HFreezeFilePrivate::WriteConfig(const ProtectInfo& config) {
	return WriteConfig(config, s_configPath);
}

// ---- Write with explicit path ----
bool HFreezeFilePrivate::WriteConfig(const ProtectInfo& config, const std::wstring& path) {
    std::ofstream file(ConvertString<string>(path), std::ios::binary);
	if (!file) return false;

	uint8_t raw[CONFIG_SIZE];
	config.ToBuffer(raw, CONFIG_SIZE);
	UpdateMD5(*reinterpret_cast<ProtectInfo*>(raw));
	file.write(reinterpret_cast<const char*>(raw), CONFIG_SIZE);
	return file.good();
}

void HFreezeFilePrivate::UpdateMD5(ProtectInfo& info) {
	MD5 md5;
	const uint8_t* start = reinterpret_cast<const uint8_t*>(&info) + 0x10;
	size_t length = CONFIG_SIZE - 0x10;
	md5.add(start, length);
	unsigned char digest[16];
	md5.getHash(digest);
	memcpy(info.md5, digest, 16);
}

ProtectInfo HFreezeFilePrivate::BuildFreezeConfig(const ProtectInfo& original,
	uint32_t targetVolMask,
	bool enableFreeze) {
	ProtectInfo cfg = original;
	cfg.readytoProtectVolume = targetVolMask;
	cfg.volMaskCopy = targetVolMask;
	cfg.bNeedFreeze = enableFreeze ? 1 : 0;
	cfg.bNeedUnFreeze = enableFreeze ? 0 : 1;
	UpdateMD5(cfg);
	return cfg;
}
#endif // !HU_DISABLE_FREEZE