#pragma once
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

#include "WinUtilsDef.h"
namespace WinUtils {
	using LoggerInitProc = void(Logger& logger);
	inline const char_t* DftLogger = TS("Default");
	enum class LogLevel : uint8_t { Debug = 0, Info = 1, Warn = 2, Error = 3 };
	enum class LogFormat { Time = 0, Level = 1 };
	class LogOutputStrategy {
	public:
		virtual ~LogOutputStrategy() = default;
		virtual void Output(LogLevel level,
			string_view_t formatted_log) noexcept = 0;
	};

	class FileLogStrategy : public LogOutputStrategy {
	public:
		explicit FileLogStrategy(string_view_t log_path);
		void Output(LogLevel level,
			const string_view_t formatted_log) noexcept override;

	private:
		string_t m_logPath;
		std::mutex m_fileMutex;
	};

	class ConsoleLogStrategy : public LogOutputStrategy {
	public:
		ConsoleLogStrategy();
		void Output(LogLevel level,
			const string_view_t formatted_log) noexcept override;

	private:
		bool m_isConsoleInitialized = false;
	};

	class DebugLogStrategy : public LogOutputStrategy {
	public:
		void Output(LogLevel level,
			const string_view_t formatted_log) noexcept override;
	};

	class LoggerCore {
	public:
		static LoggerCore& Inst() noexcept;
		LoggerCore(const LoggerCore&) = delete;
		LoggerCore(LoggerCore&&) = delete;
		LoggerCore& operator=(const LoggerCore&) = delete;
		LoggerCore& operator=(LoggerCore&&) = delete;

		void Log(LogLevel level, string_view_t msg, string_view_t apartment = TS("")) noexcept;

		template<class Strategy, class... Args>
		void AddStrategy(Args&& ...) noexcept;
		void ClearStrategies() noexcept;
		void SetDefaultStrategies(string_view_t log_path = TS("")) noexcept;

		void EnableAllApartments() noexcept;
		void DisableAllApartments() noexcept;
		void EnableApartment(string_view_t apartment);
		void DisableApartment(string_view_t apartment);
		const Logger& GetDefaultLogger();
		const std::optional<std::reference_wrapper<Logger>> GetLogger(string_view_t apartment);

	private:
		friend Logger;
		void AddLogger(Logger& logger);
		void DeleteLogger(string_view_t apartment);

	private:
		LoggerCore();
		~LoggerCore();

		std::set<std::unique_ptr<LogOutputStrategy>> m_strategies;
		std::mutex m_loggerMutex;
		std::set<string_t> m_enabledApartments;
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
			string_view_t apartment, LoggerInitProc proc = [](Logger&) {});
		~Logger();

		string_view_t GetApartment() const noexcept { return m_apartment; }
		void AddFormat(LogFormat format);
		void ClearFormat();
		bool HasFormat(LogFormat format) const;
		void Debug(string_view_t msg) noexcept { Log(LogLevel::Debug, msg); }
		void Info(string_view_t msg) noexcept { Log(LogLevel::Info, msg); }
		void Warn(string_view_t msg) noexcept { Log(LogLevel::Warn, msg); }
		void Error(string_view_t msg) noexcept { Log(LogLevel::Error, msg); }
		void Log(LogLevel level, string_view_t msg)const noexcept;
		void DLog(LogLevel level, string_view_t msg)const noexcept {
#if WINUTILS_DEBUG
			Log(level, msg);
#endif
		};

	private:
		string_t FormatLog(LogLevel level, string_view_t msg)const noexcept;
		string_t GetFormattedTime() const noexcept;
		string_view_t m_apartment;
		std::vector<LogFormat> m_format;
	};
	inline void WLog(LogLevel level, int file, int line) noexcept {
		WinUtils::LoggerCore::Inst().GetDefaultLogger().Log(
			level, std::format(TS("{}@{}"), file, line));
	};
	inline void WLog(LogLevel level, string_view_t msg) noexcept {
		WinUtils::LoggerCore::Inst().GetDefaultLogger().Log(level, msg);
	};
	inline void WuDebug(LogLevel level, string_view_t msg) noexcept {
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
