/*
 * The MIT License (MIT)
 * Copyright (c) 2026 howdy213
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include "WinUtils/WinPch.h"

#include "WinUtils/WinSvcMgr.h"
#include "WinUtils/Logger.h"
#include <format>

namespace WinUtils {
	namespace {
		Logger logger(TS("WinSvcMgr"));
	}

	WinSvcMgr::WinSvcMgr(const string_t& serviceName)
		: m_serviceName(serviceName) {
	}

	SC_HANDLE WinSvcMgr::OpenSCM(DWORD desiredAccess) {
		SC_HANDLE hSCM = ::OpenSCManagerW(nullptr, nullptr, desiredAccess);
		m_lastError = ::GetLastError();
		if (!hSCM) {
			logger.DLog(LogLevel::Error,
				std::format(TS("OpenSCManager failed (access={:#x}), error: {}"), desiredAccess, m_lastError));
		}
		return hSCM;
	}

	SC_HANDLE WinSvcMgr::OpenSvc(DWORD desiredAccess) {
		SC_HANDLE hSCM = OpenSCM(SC_MANAGER_CONNECT);
		if (!hSCM) return nullptr;

		SC_HANDLE hSvc = ::TF(OpenService)(hSCM, m_serviceName.c_str(), desiredAccess);
		m_lastError = ::GetLastError();
		if (!hSvc) {
			logger.DLog(LogLevel::Error,
				std::format(TS("OpenService [{}] failed (access={:#x}), error: {}"),
					m_serviceName, desiredAccess, m_lastError));
		}

		::CloseServiceHandle(hSCM);
		return hSvc;
	}

	bool WinSvcMgr::Install(const string_t& binaryPath,
		const string_t& displayName,
		DWORD startType,
		const string_t& dependencies,
		const string_t& account,
		const string_t& password) {
		SC_HANDLE hSCM = OpenSCM(SC_MANAGER_CREATE_SERVICE);
		if (!hSCM) return false;

		string_t deps = dependencies;
		if (!deps.empty()) {
			if (deps.back() != TS('\0')) deps.push_back(TS('\0'));
			deps.push_back(TS('\0'));
		}

		SC_HANDLE hSvc = ::TF(CreateService)(
			hSCM,
			m_serviceName.c_str(),
			displayName.empty() ? m_serviceName.c_str() : displayName.c_str(),
			SERVICE_ALL_ACCESS,
			SERVICE_WIN32_OWN_PROCESS,
			startType,
			SERVICE_ERROR_NORMAL,
			binaryPath.c_str(),
			nullptr, nullptr,
			deps.empty() ? nullptr : deps.c_str(),
			account.empty() ? nullptr : account.c_str(),
			password.empty() ? nullptr : password.c_str()
			);

		m_lastError = ::GetLastError();
		::CloseServiceHandle(hSCM);

		if (hSvc) {
			::CloseServiceHandle(hSvc);
			logger.DLog(LogLevel::Info,
				std::format(TS("Service [{}] installed successfully"), m_serviceName));
			return true;
		}

		logger.DLog(LogLevel::Error,
			std::format(TS("CreateService [{}] failed, error: {}"), m_serviceName, m_lastError));
		return false;
	}

	bool WinSvcMgr::Uninstall(bool forceStop) {
		SC_HANDLE hSCM = OpenSCM(SC_MANAGER_ALL_ACCESS);
		if (!hSCM) return false;

		SC_HANDLE hSvc = ::TF(OpenService)(hSCM, m_serviceName.c_str(), DELETE | SERVICE_STOP);
		m_lastError = ::GetLastError();
		if (!hSvc) {
			::CloseServiceHandle(hSCM);
			logger.DLog(LogLevel::Error,
				std::format(TS("OpenService [{}] for uninstall failed, error: {}"), m_serviceName, m_lastError));
			return false;
		}

		if (forceStop) {
			SERVICE_STATUS status;
			::ControlService(hSvc, SERVICE_CONTROL_STOP, &status);
			// Ignore stop failure (service may not be running)
		}

		BOOL deleted = ::DeleteService(hSvc);
		m_lastError = ::GetLastError();

		::CloseServiceHandle(hSvc);
		::CloseServiceHandle(hSCM);

		if (deleted) {
			logger.DLog(LogLevel::Info,
				std::format(TS("Service [{}] uninstalled successfully"), m_serviceName));
			return true;
		}

		logger.DLog(LogLevel::Error,
			std::format(TS("DeleteService [{}] failed, error: {}"), m_serviceName, m_lastError));
		return false;
	}

	bool WinSvcMgr::Start() {
		SC_HANDLE hSvc = OpenSvc(SERVICE_START);
		if (!hSvc) return false;

		BOOL ok = ::StartServiceW(hSvc, 0, nullptr);
		m_lastError = ::GetLastError();

		::CloseServiceHandle(hSvc);

		if (ok) {
			logger.DLog(LogLevel::Info, std::format(TS("Service [{}] started"), m_serviceName));
			return true;
		}

		if (m_lastError == ERROR_SERVICE_ALREADY_RUNNING) {
			logger.DLog(LogLevel::Warn, std::format(TS("Service [{}] already running"), m_serviceName));
			return true; // treat as success
		}

		logger.DLog(LogLevel::Error,
			std::format(TS("StartService [{}] failed, error: {}"), m_serviceName, m_lastError));
		return false;
	}

	bool WinSvcMgr::Stop() {
		SC_HANDLE hSvc = OpenSvc(SERVICE_STOP);
		if (!hSvc) return false;

		SERVICE_STATUS status;
		BOOL ok = ::ControlService(hSvc, SERVICE_CONTROL_STOP, &status);
		m_lastError = ::GetLastError();

		::CloseServiceHandle(hSvc);

		if (ok) {
			logger.DLog(LogLevel::Info, std::format(TS("Service [{}] stopped"), m_serviceName));
			return true;
		}

		logger.DLog(LogLevel::Error,
			std::format(TS("StopService [{}] failed, error: {}"), m_serviceName, m_lastError));
		return false;
	}

	DWORD WinSvcMgr::GetStatus() {
		SC_HANDLE hSvc = OpenSvc(SERVICE_QUERY_STATUS);
		if (!hSvc) return 0;

		SERVICE_STATUS status;
		BOOL ok = ::QueryServiceStatus(hSvc, &status);
		m_lastError = ::GetLastError();

		::CloseServiceHandle(hSvc);
		return ok ? status.dwCurrentState : 0;
	}

	bool WinSvcMgr::Exists(const string_t& serviceName) {
		SC_HANDLE hSCM = ::TF(OpenSCManager)(nullptr, nullptr, SC_MANAGER_CONNECT);
		if (!hSCM) return false;

		SC_HANDLE hSvc = ::TF(OpenService)(hSCM, serviceName.c_str(), SERVICE_QUERY_CONFIG);
		DWORD err = ::GetLastError();
		::CloseServiceHandle(hSvc);
		::CloseServiceHandle(hSCM);

		if (hSvc) return true;
		return err != ERROR_SERVICE_DOES_NOT_EXIST;
	}

} // namespace WinUtils