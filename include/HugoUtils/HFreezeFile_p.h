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
 *
 * HFreezeFilePrivate – Pure file I/O for VolumeInfo.config.
 * Supports custom file path via SetConfigPath().
 */
#pragma once
#include "HugoUtilsDef.h"
#ifndef HU_DISABLE_FREEZE

#include "SWFreezeTypes.h"
#include <optional>
#include <vector>
#include <string>

class HFreezeFilePrivate {
public:
	// Default configuration path (used when no custom path is set)
	static constexpr const wchar_t* DEFAULT_CONFIG_PATH =
		L"C:\\ProgramData\\SeewoFreezeKernelConfig\\VolumeInfo.config";

	static constexpr size_t CONFIG_SIZE = 1024;

	// Set / get the current configuration file path
	static void SetConfigPath(const std::wstring& path);
	static const std::wstring& GetConfigPath();

	// Read the current configuration using the active path
	static std::optional<ProtectInfo> ReadConfig();
	// Read from an explicit path (ignores the active path)
	static std::optional<ProtectInfo> ReadConfig(const std::wstring& path);

	// Write using the active path
	static bool WriteConfig(const ProtectInfo& config);
	// Write to an explicit path
	static bool WriteConfig(const ProtectInfo& config, const std::wstring& path);

	// Build a modified configuration (does not involve file I/O)
	static ProtectInfo BuildFreezeConfig(const ProtectInfo& original,
		uint32_t targetVolMask,
		bool enableFreeze);

private:
	// Recalculate MD5 digest from offset 0x10 to the end of the struct.
	static void UpdateMD5(ProtectInfo& info);

	// Active configuration file path (thread‑safe initialization guaranteed)
	static std::wstring s_configPath;
};
#endif // !HU_DISABLE_FREEZE