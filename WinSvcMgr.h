#pragma once
#include <Windows.h>
#include <string>
#include <iostream>
#include "WinUtils.h"
class WinUtils::WinSvcMgr
{
public:
    explicit WinSvcMgr(const std::wstring& serviceName);
    bool StartWinService();
    DWORD GetServiceStatus();
    bool StopService();
    DWORD GetLastErrorCode() const { return m_lastError; }
private:
    SC_HANDLE OpenSCM();
    SC_HANDLE OpenTargetService(DWORD desiredAccess);
    std::wstring m_serviceName;
    DWORD m_lastError = 0;
};