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
#pragma once

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdio>
#include <format>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "WinUtilsDef.h"

namespace WinUtils {

	class Logger;
	using LoggerInitProc = void(class Logger& logger);
	inline const char_t* DftLogger = TS("Default");

	enum class LogLevel : uint8_t { Debug = 0, Info = 1, Warn = 2, Error = 3 };
	enum class LogFormat { Time = 0, Level = 1 };

	// Threshold sentinel: value 4 means "logging disabled for this apartment".
	inline constexpr uint8_t kLogDisabledThreshold = 4;

	// ===========================================================================
	// LogFormatter
	// ===========================================================================
	class LogFormatter {
	public:
		using FormatVec = std::vector<LogFormat>;

		explicit LogFormatter(FormatVec formats = {
			LogFormat::Time, LogFormat::Level })
			: m_formats(std::make_shared<const FormatVec>(std::move(formats))) {
		}

		// Copy-on-write: readers keep lock-free access to a stable snapshot.
		void AddFormat(LogFormat f) {
			auto next = std::make_shared<FormatVec>();
			if (auto old = m_formats.load(std::memory_order_acquire))
				*next = *old;
			next->push_back(f);
			m_formats.store(std::move(next), std::memory_order_release);
		}
		void ClearFormats() noexcept {
			m_formats.store(std::make_shared<const FormatVec>(), std::memory_order_release);
		}
		bool HasFormat(LogFormat f) const noexcept {
			auto formats = m_formats.load(std::memory_order_acquire);
			return formats &&
				std::find(formats->begin(), formats->end(), f) != formats->end();
		}

		string_t Format(LogLevel level, string_view_t msg) const;
		// Append-only variant: reuses caller-provided buffer to avoid extra allocation.
		void FormatTo(string_t& out, LogLevel level, string_view_t msg) const;

	private:
		// Thread-local second-level cache: at most one strftime per second per thread.
		static const string_t& GetFormattedTime() noexcept;
		static const char_t* LevelToString(LogLevel level) noexcept;

		std::atomic<std::shared_ptr<const FormatVec>> m_formats;
	};

	// ===========================================================================
	// LogOutputStrategy
	// ===========================================================================
	class LogOutputStrategy {
	public:
		explicit LogOutputStrategy(std::shared_ptr<LogFormatter> fmt = nullptr);
		virtual ~LogOutputStrategy() = default;

		LogOutputStrategy(const LogOutputStrategy&) = delete;
		LogOutputStrategy& operator=(const LogOutputStrategy&) = delete;

		// Hot path: single atomic acquire load, no lock.
		std::shared_ptr<LogFormatter> GetFormatter() const noexcept {
			return m_formatter.load(std::memory_order_acquire);
		}
		void SetFormatter(std::shared_ptr<LogFormatter> fmt) noexcept {
			m_formatter.store(std::move(fmt), std::memory_order_release);
		}

		virtual void Output(LogLevel level, string_view_t msg) noexcept = 0;

	protected:
		std::atomic<std::shared_ptr<LogFormatter>> m_formatter;
	};

	// ===========================================================================
	// FileLogStrategy
	// ===========================================================================
	class FileLogStrategy : public LogOutputStrategy {
	public:
		explicit FileLogStrategy(string_view_t logPath,
			std::shared_ptr<LogFormatter> fmt = nullptr);
		~FileLogStrategy() override;

		void Output(LogLevel level, string_view_t msg) noexcept override;

	private:
		// Caller must hold m_fileMutex.
		void OpenLocked() noexcept;

		string_t m_logPath;
		std::mutex m_fileMutex;
		FILE* m_file = nullptr;
	};

	// ===========================================================================
	// ConsoleLogStrategy
	// ===========================================================================
	class ConsoleLogStrategy : public LogOutputStrategy {
	public:
		explicit ConsoleLogStrategy(std::shared_ptr<LogFormatter> fmt = nullptr);
		void Output(LogLevel level, string_view_t msg) noexcept override;

	private:
		// Re-checked on demand so a console attached after construction still works.
		bool EnsureConsole() noexcept;

		std::atomic<bool> m_consoleAvailable{ false };
		std::mutex m_consoleMutex;
	};

	// ===========================================================================
	// DebugLogStrategy
	// ===========================================================================
	class DebugLogStrategy : public LogOutputStrategy {
	public:
		explicit DebugLogStrategy(std::shared_ptr<LogFormatter> fmt = nullptr);
		void Output(LogLevel level, string_view_t msg) noexcept override;
	};

	// ===========================================================================
	// LoggerCore
	// ===========================================================================
	class LoggerCore {
	public:
		static LoggerCore& Inst() noexcept;

		LoggerCore(const LoggerCore&) = delete;
		LoggerCore(LoggerCore&&) = delete;
		LoggerCore& operator=(const LoggerCore&) = delete;
		LoggerCore& operator=(LoggerCore&&) = delete;

		// ---- Global level / apartment configuration (low-frequency, locked) ----
		void SetGlobalLevel(LogLevel level) noexcept;
		LogLevel GetGlobalLevel() const noexcept;

		void EnableAllApartments() noexcept;
		void DisableAllApartments() noexcept;
		void EnableApartment(string_view_t apartment);
		void DisableApartment(string_view_t apartment);
		bool IsApartmentEnabled(string_view_t apartment) const;

		// ---- Strategies (written rarely; dispatch path is lock-free) ----
		template <class Strategy, class... Args>
		void AddStrategy(Args&&... args) {
			static_assert(std::is_base_of_v<LogOutputStrategy, Strategy>,
				"Strategy must inherit from LogOutputStrategy!");
			auto strategy = std::make_shared<Strategy>(std::forward<Args>(args)...);
			if (!strategy->GetFormatter())
				strategy->SetFormatter(GetDefaultFormatter());

			std::lock_guard<std::mutex> lock(m_strategiesWriteMutex);
			auto old = m_strategies.load(std::memory_order_acquire);
			auto next = old ? std::make_shared<StrategyVec>(*old)
				: std::make_shared<StrategyVec>();
			next->push_back(std::move(strategy));
			m_strategies.store(std::move(next), std::memory_order_release);
		}

		void AddFileStrategy(string_view_t path, std::shared_ptr<LogFormatter> fmt = nullptr);
		void AddConsoleStrategy(std::shared_ptr<LogFormatter> fmt = nullptr);
		void AddDebugStrategy(std::shared_ptr<LogFormatter> fmt = nullptr);
		void ClearStrategies() noexcept;
		void SetDefaultStrategies(string_view_t logPath = TS("")) noexcept;

		// ---- Formatter ----
		std::shared_ptr<LogFormatter> GetDefaultFormatter() const;
		// A null formatter is ignored so log output never silently disappears.
		void SetDefaultFormatter(std::shared_ptr<LogFormatter> fmt);

		// ---- Logger lookup ----
		Logger& GetDefaultLogger();
		// WARNING: the returned logger is owned by the caller. The reference
		// becomes dangling if that Logger is destroyed concurrently.
		std::optional<std::reference_wrapper<Logger>> GetLogger(string_view_t apartment);

		// ---- Hot-path dispatch (lock-free) ----
		void Dispatch(LogLevel level, string_view_t msg) noexcept {
			auto strategies = m_strategies.load(std::memory_order_acquire);
			if (!strategies) return;
			for (const auto& s : *strategies) {
				if (s) s->Output(level, msg);
			}
		}

	private:
		friend class Logger;
		using StrategyVec = std::vector<std::shared_ptr<LogOutputStrategy>>;

		LoggerCore();
		~LoggerCore();

		void AddLogger(Logger& logger);
		void DeleteLogger(Logger& logger);
		// Caller must hold m_configMutex.
		void RefreshAllLoggersUnlocked() noexcept;
		uint8_t CalcThresholdUnlocked(string_view_t apartment) const noexcept;

		std::atomic<std::shared_ptr<const StrategyVec>> m_strategies;
		std::mutex m_strategiesWriteMutex;

		mutable std::mutex m_configMutex;
		LogLevel m_globalLevel = LogLevel::Debug;
		bool m_allEnabled = false;
		std::set<string_t, std::less<>> m_except; // heterogeneous lookup: find accepts string_view_t
		std::set<Logger*> m_loggers;

		mutable std::mutex m_formatterMutex;
		std::shared_ptr<LogFormatter> m_defaultFormatter;

		// Declared last so it is destroyed first; see ~LoggerCore.
		std::unique_ptr<Logger> m_defaultLogger;
		std::once_flag m_defaultLoggerOnce;
	};

	// ===========================================================================
	// Logger
	// ===========================================================================
	class Logger {
	public:
		explicit Logger(string_view_t apartment, LoggerInitProc proc = [](Logger&) {});
		~Logger();

		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;

		const string_t& GetApartment() const noexcept { return m_apartment; }

		void Log(LogLevel level, string_view_t msg) const noexcept {
			if (msg.empty() || !ShouldLog(level)) return;
			LoggerCore::Inst().Dispatch(level, msg);
		}

		template<typename... Args>
		void Log(LogLevel level, string_view_t fmt, Args&&... args) const noexcept {
			FormatAndLog(level, fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		void Debug(string_view_t fmt, Args&&... args) const noexcept {
			FormatAndLog(LogLevel::Debug, fmt, std::forward<Args>(args)...);
		}
		template<typename... Args>
		void Info(string_view_t fmt, Args&&... args) const noexcept {
			FormatAndLog(LogLevel::Info, fmt, std::forward<Args>(args)...);
		}
		template<typename... Args>
		void Warn(string_view_t fmt, Args&&... args) const noexcept {
			FormatAndLog(LogLevel::Warn, fmt, std::forward<Args>(args)...);
		}
		template<typename... Args>
		void Error(string_view_t fmt, Args&&... args) const noexcept {
			FormatAndLog(LogLevel::Error, fmt, std::forward<Args>(args)...);
		}

		void DLog(LogLevel level, string_view_t msg) const noexcept {
#ifdef WU_DEBUG
			Log(level, msg);
#endif
		}

	private:
		friend class LoggerCore;

		void UpdateThreshold(uint8_t t) noexcept {
			m_threshold.store(t, std::memory_order_relaxed);
		}

		// Single relaxed atomic load: the only check on the hot path.
		bool ShouldLog(LogLevel level) const noexcept {
			return static_cast<uint8_t>(level) >=
				m_threshold.load(std::memory_order_relaxed);
		}

		template<typename... Args>
		void FormatAndLog(LogLevel level, string_view_t fmt, Args&&... args) const noexcept {
			// Return before tuple construction / vformat when disabled.
			if (fmt.empty() || !ShouldLog(level)) return;

			if constexpr (sizeof...(Args) == 0) {
				LoggerCore::Inst().Dispatch(level, fmt);
			}
			else {
				// noexcept contract: a bad format string or OOM must not terminate.
				try {
					auto tup = std::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...);
					std::apply([level, fmt](auto&... vals) {
						auto s = std::vformat(fmt, make_tformat_args(vals...));
						LoggerCore::Inst().Dispatch(level, s);
						}, tup);
				}
				catch (...) {
				}
			}
		}

		string_t m_apartment;
		std::atomic<uint8_t> m_threshold{ kLogDisabledThreshold };
	};

	// ---------- WuLog global convenience functions ----------
	namespace WuLog {
		inline WinUtils::Logger& Default() {
			return WinUtils::LoggerCore::Inst().GetDefaultLogger();
		}

		template<typename... Args>
		void Debug(string_view_t fmt, Args&&... args) {
			Default().Debug(fmt, std::forward<Args>(args)...);
		}
		template<typename... Args>
		void Info(string_view_t fmt, Args&&... args) {
			Default().Info(fmt, std::forward<Args>(args)...);
		}
		template<typename... Args>
		void Warn(string_view_t fmt, Args&&... args) {
			Default().Warn(fmt, std::forward<Args>(args)...);
		}
		template<typename... Args>
		void Error(string_view_t fmt, Args&&... args) {
			Default().Error(fmt, std::forward<Args>(args)...);
		}
		template<typename... Args>
		void Log(LogLevel level, string_view_t fmt, Args&&... args) {
			Default().Log(level, fmt, std::forward<Args>(args)...);
		}
	}

} // namespace WinUtils
