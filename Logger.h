#pragma once
#include <Windows.h>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <string_view>
#include <chrono>
#include "WinUtilsDef.h"

namespace WinUtils {
	enum class LogLevel : uint8_t {
		Debug = 0,
		Info = 1,
		Warn = 2,
		Error = 3
	};
	enum class LogFormat {
		Time = 0,
		Level =1
	};
	class LogOutputStrategy {
	public:
		virtual ~LogOutputStrategy() = default;
		virtual void Output(LogLevel level, const std::wstring& formatted_log) noexcept = 0;
	};

	class FileLogStrategy : public LogOutputStrategy {
	public:
		explicit FileLogStrategy(std::wstring_view log_path);
		void Output(LogLevel level, const std::wstring& formatted_log) noexcept override;
	private:
		std::wstring log_path_;
		std::mutex file_mutex_; // 文件写入独立锁
	};

	class ConsoleLogStrategy : public LogOutputStrategy {
	public:
		ConsoleLogStrategy();
		void Output(LogLevel level, const std::wstring& formatted_log) noexcept override;
	private:
		bool console_inited_ = false;
	};

	class DebugLogStrategy : public LogOutputStrategy {
	public:
		void Output(LogLevel level, const std::wstring& formatted_log) noexcept override;
	};

	class Logger {
	public:
		static Logger& Inst() noexcept {
			static Logger instance;
			return instance;
		}

		Logger(const Logger&) = delete;
		Logger(Logger&&) = delete;
		Logger& operator=(const Logger&) = delete;
		Logger& operator=(Logger&&) = delete;
		
		void AddFormat(LogFormat format);
		void ClearFormat();
		bool HasFormat(LogFormat format);
		void AddStrategy(std::unique_ptr<LogOutputStrategy> strategy) noexcept;
		void ClearStrategies() noexcept;

		void Log(LogLevel level, std::wstring_view msg) noexcept;
		void Debug(std::wstring_view msg) noexcept { Log(LogLevel::Debug, msg); }
		void Info(std::wstring_view msg) noexcept { Log(LogLevel::Info, msg); }
		void Warn(std::wstring_view msg) noexcept { Log(LogLevel::Warn, msg); }
		void Error(std::wstring_view msg) noexcept { Log(LogLevel::Error, msg); }

		void SetDefaultStrategies(std::wstring_view log_path = L"") noexcept;

	private:
		Logger() = default;
		~Logger() = default;

		std::wstring FormatLog(LogLevel level, std::wstring_view msg) noexcept;
		std::wstring GetFormattedTime() noexcept;

		std::vector<std::unique_ptr<LogOutputStrategy>> m_strategies;
		std::mutex m_loggerMutex;
		std::vector<LogFormat> m_format;
	};
	inline void WLog(LogLevel level, int file, int line) noexcept { WinUtils::Logger::Inst().Log(level, std::format(L"{}@{}", file, line)); };
	inline void WLog(LogLevel level, std::wstring_view msg) noexcept { WinUtils::Logger::Inst().Log(level, msg); };
	inline void WuDebug(LogLevel level, std::wstring_view msg) noexcept {
#if WINUTILS_DEBUG
		WinUtils::Logger::Inst().Log(level, msg);
#endif
	};
}
#define WLOG_DEBUG(FILE,LINE) WLog(WinUtils::LogLevel::Debug,FILE,LINE);
#define WLOG_INFO(FILE,LINE) WLog(WinUtils::LogLevel::Info,FILE,LINE);
#define WLOG_WARN(FILE,LINE) WLog(WinUtils::LogLevel::Warn,FILE,LINE);
#define WLOG_ERROR(FILE,LINE) WLog(WinUtils::LogLevel::Error,FILE,LINE);
#define WLOG(LEVEL) WLOG_##LEVEL(__FILE__,__LINE__)