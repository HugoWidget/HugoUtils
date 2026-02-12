#include "WinSvcMgr.h"
#include "Logger.h"  
using namespace WinUtils;
using namespace std;
static Logger logger(L"WinSvcMgr");

WinSvcMgr::WinSvcMgr(const std::wstring& serviceName)
	: m_serviceName(serviceName)
{
}

SC_HANDLE WinSvcMgr::OpenSCM()
{
	SC_HANDLE hSCM = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
	m_lastError = GetLastError();
	if (!hSCM) logger.DLog(LogLevel::Error, format(L"Failed to open Service Control Manager (SCM), error code: {}", m_lastError));
	return hSCM;
}

SC_HANDLE WinSvcMgr::OpenTargetService(DWORD desiredAccess)
{
	SC_HANDLE hSCM = OpenSCM();
	if (!hSCM) return nullptr;

	SC_HANDLE hService = OpenServiceW(hSCM, m_serviceName.c_str(), desiredAccess);
	m_lastError = GetLastError();

	if (!hService) logger.DLog(LogLevel::Error, format(L"Failed to open service [{}], error code: {}", m_serviceName, m_lastError));
	CloseServiceHandle(hSCM); // Release SCM handle
	return hService;
}

bool WinSvcMgr::StartWinService()
{
	SC_HANDLE hService = OpenTargetService(SERVICE_START);
	if (!hService) return false;

	BOOL bSuccess = ::StartServiceW(hService, 0, nullptr);
	m_lastError = GetLastError();

	if (bSuccess) logger.DLog(LogLevel::Info, format(L"Service [{}] started successfully", m_serviceName));
	else if (m_lastError == ERROR_SERVICE_ALREADY_RUNNING)
		logger.DLog(LogLevel::Warn, format(L"Service [{}] is already running", m_serviceName));
	else logger.DLog(LogLevel::Error, format(L"Failed to start service [{}], error code: {}", m_serviceName, m_lastError));

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

	if (bSuccess) logger.DLog(LogLevel::Info, format(L"Service [{}] stopped successfully", m_serviceName));
	else logger.DLog(LogLevel::Error, format(L"Failed to stop service [{}], error code: {}", m_serviceName, m_lastError));

	CloseServiceHandle(hService);
	return bSuccess != FALSE;
}