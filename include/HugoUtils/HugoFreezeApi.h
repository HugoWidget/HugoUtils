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
#pragma once
#include "HugoUtilsDef.h"
#ifndef HU_DISABLE_FREEZE_API


#include <Windows.h>
#include <string>
#include <cstdint>

#include "WinUtils/Logger.h"
#include "WinUtils/HttpConnect.h"
#include "HugoUtils/HugoFreezeInterface.h"

// Constant definitions
constexpr wchar_t DEFAULT_IP[] = L"127.0.0.1";
constexpr uint16_t DEFAULT_PORT = 6082;

// Seewo Freeze API Core Operation Class
class HugoFreezeApi: public IHugoFreeze {
public:

    virtual FreezeResult Init() noexcept;
    virtual void Cleanup() noexcept;
    virtual bool IsInitialized() const noexcept;

    // Configuration management
    virtual void SetConfig(const std::wstring& ip, uint16_t port) noexcept;
    virtual void GetConfig(std::wstring& outIp, uint16_t& outPort) const noexcept;

    // Core functionalities
    virtual FreezeResult GetFreezeState() const noexcept;
    virtual FreezeResult TryProtect(const std::wstring& driveLetters) const noexcept;
	// Set freeze state for specified drive letters (e.g., L"CDE" to freeze C:, D:, E:)
    virtual FreezeResult SetFreezeState(
        const std::wstring& driveLetters,
		DriveFreezeState state = DriveFreezeState::Frozen
    ) noexcept;

    virtual std::wstring GetLastErrorMsg() const noexcept;
    virtual DWORD GetLastErrorCode() const noexcept;

public:
    HugoFreezeApi() noexcept;
    ~HugoFreezeApi() = default;

private:
    uint16_t m_port{ DEFAULT_PORT };          // Target port
    WinUtils::HttpConnect m_httpClient;       // HTTP client instance
};
#endif // !HU_DISABLE_FREEZE_API