#include "WinSvcMgr.h"
#include "Logger.h"  
using namespace WinUtils;
using namespace std;
WinSvcMgr::WinSvcMgr(const std::wstring& serviceName)
	: m_serviceName(serviceName)
{
}

SC_HANDLE WinSvcMgr::OpenSCM()
{
	SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
	m_lastError = GetLastError();
	if (!hSCM) WuDebug(LogLevel::Error, format(L"打开服务控制管理器失败，错误码：{}", m_lastError));
	return hSCM;
}

SC_HANDLE WinSvcMgr::OpenTargetService(DWORD desiredAccess)
{
	SC_HANDLE hSCM = OpenSCM();
	if (!hSCM) return nullptr;

	SC_HANDLE hService = OpenServiceW(hSCM, m_serviceName.c_str(), desiredAccess);
	m_lastError = GetLastError();

	if (!hService) WuDebug(LogLevel::Error, format(L"打开服务[{}]失败，错误码：{}", m_serviceName, m_lastError));
	CloseServiceHandle(hSCM); // 释放SCM句柄
	return hService;
}

bool WinSvcMgr::StartWinService()
{
	SC_HANDLE hService = OpenTargetService(SERVICE_START);
	if (!hService) return false;

	BOOL bSuccess = ::StartServiceW(hService, 0, nullptr);
	m_lastError = GetLastError();

	if (bSuccess) WuDebug(LogLevel::Info, format(L"服务[{}]启动成功", m_serviceName));
	else if (m_lastError == ERROR_SERVICE_ALREADY_RUNNING)
		WuDebug(LogLevel::Warn, format(L"服务[{}]已处于运行状态", m_serviceName));
	else WuDebug(LogLevel::Error, format(L"启动服务[{}]失败，错误码：{}", m_serviceName, m_lastError));

	CloseServiceHandle(hService);
	return bSuccess != FALSE;
}

DWORD WinSvcMgr::GetServiceStatus()
{
	SC_HANDLE hService = OpenTargetService(SERVICE_QUERY_STATUS);
	if (!hService) return 0;

	SERVICE_STATUS status{};
	BOOL bSuccess = ::QueryServiceStatus(hService, &status);
	m_lastError = GetLastError();

	CloseServiceHandle(hService);
	return bSuccess ? status.dwCurrentState : 0;
}

bool WinSvcMgr::StopService()
{
	SC_HANDLE hService = OpenTargetService(SERVICE_STOP);
	if (!hService) return false;

	SERVICE_STATUS status{};
	BOOL bSuccess = ::ControlService(hService, SERVICE_CONTROL_STOP, &status);
	m_lastError = GetLastError();

	if (bSuccess) WuDebug(LogLevel::Info, format(L"服务[{}]停止成功", m_serviceName));
	else WuDebug(LogLevel::Error, format(L"停止服务[{}]失败，错误码：{}", m_serviceName, m_lastError));

	CloseServiceHandle(hService);
	return bSuccess != FALSE;
}