#pragma once
#include "WinUtilsDef.h"
#include <Windows.h>
#include <format>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <functional>
#include <set>

namespace WinUtils {
	using LoggerInitProc = void(Logger& logger);
	inline const wchar_t* DftLogger = L"Default";
	enum class LogLevel : uint8_t { Debug = 0, Info = 1, Warn = 2, Error = 3 };
	enum class LogFormat { Time = 0, Level = 1 };
	class LogOutputStrategy {
	public:
		virtual ~LogOutputStrategy() = default;
		virtual void Output(LogLevel level,
			std::wstring_view formatted_log) noexcept = 0;
	};

	class FileLogStrategy : public LogOutputStrategy {
	public:
		explicit FileLogStrategy(std::wstring_view log_path);
		void Output(LogLevel level,
			const std::wstring_view formatted_log) noexcept override;

	private:
		std::wstring m_logPath;
		std::mutex m_fileMutex;
	};

	class ConsoleLogStrategy : public LogOutputStrategy {
	public:
		ConsoleLogStrategy();
		void Output(LogLevel level,
			const std::wstring_view formatted_log) noexcept override;

	private:
		bool m_isConsoleInitialized = false;
	};

	class DebugLogStrategy : public LogOutputStrategy {
	public:
		void Output(LogLevel level,
			const std::wstring_view formatted_log) noexcept override;
	};

	class LoggerCore {
	public:
		static LoggerCore& Inst() noexcept;
		LoggerCore(const LoggerCore&) = delete;
		LoggerCore(LoggerCore&&) = delete;
		LoggerCore& operator=(const LoggerCore&) = delete;
		LoggerCore& operator=(LoggerCore&&) = delete;

		void Log(LogLevel level, std::wstring_view msg, std::wstring_view apartment = L"") noexcept;

		template<class Strategy, class... Args>
		void AddStrategy(Args&& ...) noexcept;
		void ClearStrategies() noexcept;
		void SetDefaultStrategies(std::wstring_view log_path = L"") noexcept;

		void EnableAllApartments() noexcept;
		void DisableAllApartments() noexcept;
		void EnableApartment(std::wstring_view apartment);
		void DisableApartment(std::wstring_view apartment);
		const Logger& GetDefaultLogger();
		const std::optional<std::reference_wrapper<Logger>> GetLogger(std::wstring_view apartment);

	private:
		friend Logger;
		void AddLogger(Logger& logger);
		void DeleteLogger(std::wstring_view apartment);

	private:
		LoggerCore();
		~LoggerCore();

		std::set<std::unique_ptr<LogOutputStrategy>> m_strategies;
		std::mutex m_loggerMutex;
		std::set<std::wstring> m_enabledApartments;
		std::set<Logger*> m_loggers;
	};
	template<class Strategy, class ...Args>
	void LoggerCore::AddStrategy(Args && ... args) noexcept
	{
		static_assert(std::is_base_of_v<LogOutputStrategy, Strategy>,
			"Strategy must inherit from LogOutputStrategy!");
		m_strategies.insert(
			std::make_unique<Strategy>(std::forward<Args>(args)...)
		);
	}
	class Logger {
	public:
		explicit Logger(
			std::wstring_view apartment, LoggerInitProc proc = [](Logger&) {});
		~Logger();

		std::wstring_view GetApartment() const noexcept { return m_apartment; }
		void AddFormat(LogFormat format);
		void ClearFormat();
		bool HasFormat(LogFormat format) const;
		void Debug(std::wstring_view msg) noexcept { Log(LogLevel::Debug, msg); }
		void Info(std::wstring_view msg) noexcept { Log(LogLevel::Info, msg); }
		void Warn(std::wstring_view msg) noexcept { Log(LogLevel::Warn, msg); }
		void Error(std::wstring_view msg) noexcept { Log(LogLevel::Error, msg); }
		void Log(LogLevel level, std::wstring_view msg)const noexcept;
		void DLog(LogLevel level, std::wstring_view msg)const noexcept {
#if WINUTILS_DEBUG
			Log(level, msg);
#endif
		};

	private:
		std::wstring FormatLog(LogLevel level, std::wstring_view msg)const noexcept;
		std::wstring GetFormattedTime() const noexcept;
		std::wstring_view m_apartment;
		std::vector<LogFormat> m_format;
	};
	inline void WLog(LogLevel level, int file, int line) noexcept {
		WinUtils::LoggerCore::Inst().GetDefaultLogger().Log(
			level, std::format(L"{}@{}", file, line));
	};
	inline void WLog(LogLevel level, std::wstring_view msg) noexcept {
		WinUtils::LoggerCore::Inst().GetDefaultLogger().Log(level, msg);
	};
	inline void WuDebug(LogLevel level, std::wstring_view msg) noexcept {
#if WINUTILS_DEBUG
		WinUtils::LoggerCore::Inst().GetDefaultLogger().Log(level, msg);
#endif
	};
} // namespace WinUtils
#define WLOG_DEBUG(FILE, LINE) WLog(WinUtils::LogLevel::Debug, FILE, LINE);
#define WLOG_INFO(FILE, LINE) WLog(WinUtils::LogLevel::Info, FILE, LINE);
#define WLOG_WARN(FILE, LINE) WLog(WinUtils::LogLevel::Warn, FILE, LINE);
#define WLOG_ERROR(FILE, LINE) WLog(WinUtils::LogLevel::Error, FILE, LINE);
#define WLOG(LEVEL) WLOG_##LEVEL(__FILE__, __LINE__)
