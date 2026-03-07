#pragma once
#include <Windows.h>
#include <string>
#include <cstdint>

#include "Logger.h"
#include "HttpConnect.h"
#include "HugoFreezeInterface.h"

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