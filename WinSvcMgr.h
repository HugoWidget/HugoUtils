#pragma once
#include <Windows.h>
#include <string>
#include "WinUtilsDef.h"

namespace WinUtils {

	class WinSvcMgr {
	public:
		explicit WinSvcMgr(const string_t& serviceName);

		// 安装服务
		bool Install(const string_t& binaryPath,
			const string_t& displayName = TS(""),
			DWORD startType = SERVICE_AUTO_START,
			const string_t& dependencies = TS(""),
			const string_t& account = TS(""),
			const string_t& password = TS(""));

		// 卸载服务
		bool Uninstall(bool forceStop = true);

		// 启动服务
		bool Start();

		// 停止服务
		bool Stop();

		// 查询服务状态（返回0表示失败）
		DWORD GetStatus();

		// 获取最后一次操作的错误码
		DWORD GetLastError() const { return m_lastError; }

		// 静态方法：检查服务是否存在
		static bool Exists(const string_t& serviceName);

	private:
		string_t m_serviceName;
		DWORD m_lastError = 0;

		SC_HANDLE OpenSCM(DWORD desiredAccess);
		SC_HANDLE OpenSvc(DWORD desiredAccess);
	};

} // namespace WinUtils