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
#include <format>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "WinUtilsDef.h"

namespace WinUtils {

    using LoggerInitProc = void(class Logger& logger);
    inline const char_t* DftLogger = TS("Default");

    enum class LogLevel : uint8_t { Debug = 0, Info = 1, Warn = 2, Error = 3 };
    enum class LogFormat { Time = 0, Level = 1 };

    // ===========================================================================
    // LogFormatter
    // ===========================================================================
    class LogFormatter {
    public:
        explicit LogFormatter(std::vector<LogFormat> formats = {
            LogFormat::Time, LogFormat::Level })
            : m_formats(std::move(formats)) {
        }

        void AddFormat(LogFormat f) { m_formats.push_back(f); }
        void ClearFormats() { m_formats.clear(); }
        bool HasFormat(LogFormat f) const {
            return std::find(m_formats.begin(), m_formats.end(), f) != m_formats.end();
        }

        string_t Format(LogLevel level, string_view_t msg) const;

    private:
        static string_t GetFormattedTime() noexcept;
        static const char_t* LevelToString(LogLevel level) noexcept;

        std::vector<LogFormat> m_formats;
    };

    // ===========================================================================
    // LogOutputStrategy
    // ===========================================================================
    class LogOutputStrategy {
    public:
        virtual ~LogOutputStrategy() = default;

        virtual std::shared_ptr<LogFormatter> GetFormatter() const = 0;
        virtual void SetFormatter(std::shared_ptr<LogFormatter> fmt) = 0;

        virtual void Output(LogLevel level, string_view_t msg) noexcept = 0;
    };

    // ===========================================================================
    // FileLogStrategy
    // ===========================================================================
    class FileLogStrategy : public LogOutputStrategy {
    public:
        explicit FileLogStrategy(string_view_t logPath,
            std::shared_ptr<LogFormatter> fmt = nullptr);
        void Output(LogLevel level, string_view_t msg) noexcept override;

        std::shared_ptr<LogFormatter> GetFormatter() const override { return m_formatter; }
        void SetFormatter(std::shared_ptr<LogFormatter> fmt) override { m_formatter = std::move(fmt); }

    private:
        string_t m_logPath;
        std::mutex m_fileMutex;
        std::shared_ptr<LogFormatter> m_formatter;
    };

    // ===========================================================================
    // ConsoleLogStrategy
    // ===========================================================================
    class ConsoleLogStrategy : public LogOutputStrategy {
    public:
        explicit ConsoleLogStrategy(std::shared_ptr<LogFormatter> fmt = nullptr);
        void Output(LogLevel level, string_view_t msg) noexcept override;

        std::shared_ptr<LogFormatter> GetFormatter() const override { return m_formatter; }
        void SetFormatter(std::shared_ptr<LogFormatter> fmt) override { m_formatter = std::move(fmt); }

    private:
        bool m_consoleAvailable = false;
        std::shared_ptr<LogFormatter> m_formatter;
    };

    // ===========================================================================
    // DebugLogStrategy
    // ===========================================================================
    class DebugLogStrategy : public LogOutputStrategy {
    public:
        explicit DebugLogStrategy(std::shared_ptr<LogFormatter> fmt = nullptr);
        void Output(LogLevel level, string_view_t msg) noexcept override;

        std::shared_ptr<LogFormatter> GetFormatter() const override { return m_formatter; }
        void SetFormatter(std::shared_ptr<LogFormatter> fmt) override { m_formatter = std::move(fmt); }

    private:
        std::shared_ptr<LogFormatter> m_formatter;
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

        void Log(LogLevel level, string_view_t msg) noexcept;

        void SetGlobalLevel(LogLevel level) noexcept { m_globalLevel = level; }
        LogLevel GetGlobalLevel() const noexcept { return m_globalLevel; }

        template <class Strategy, class... Args>
        void AddStrategy(Args&&... args) noexcept {
            static_assert(std::is_base_of_v<LogOutputStrategy, Strategy>,
                "Strategy must inherit from LogOutputStrategy!");
            auto strategy = std::make_unique<Strategy>(std::forward<Args>(args)...);
            if (!strategy->GetFormatter())
                strategy->SetFormatter(GetDefaultFormatter());
            std::lock_guard<std::mutex> lock(m_loggerMutex);
            m_strategies.insert(std::move(strategy));
        }

        void AddFileStrategy(string_view_t path, std::shared_ptr<LogFormatter> fmt = nullptr);
        void AddConsoleStrategy(std::shared_ptr<LogFormatter> fmt = nullptr);
        void AddDebugStrategy(std::shared_ptr<LogFormatter> fmt = nullptr);
        void ClearStrategies() noexcept;
        void SetDefaultStrategies(string_view_t logPath = TS("")) noexcept;

        std::shared_ptr<LogFormatter> GetDefaultFormatter() const;
        void SetDefaultFormatter(std::shared_ptr<LogFormatter> fmt);

        void EnableAllApartments() noexcept;
        void DisableAllApartments() noexcept;
        void EnableApartment(string_view_t apartment);
        void DisableApartment(string_view_t apartment);
        bool IsApartmentEnabled(string_view_t apartment) const;

        Logger& GetDefaultLogger();
        std::optional<std::reference_wrapper<Logger>> GetLogger(string_view_t apartment);

    private:
        friend class Logger;
        void AddLogger(Logger& logger);
        void DeleteLogger(string_view_t apartment);

        LoggerCore();
        ~LoggerCore();

        std::set<std::unique_ptr<LogOutputStrategy>> m_strategies;
        std::mutex m_loggerMutex;
        mutable std::mutex m_formatterMutex;

        bool m_allEnabled = false;
        std::set<string_t> m_except;
        std::set<Logger*> m_loggers;
        LogLevel m_globalLevel = LogLevel::Debug;

        std::unique_ptr<Logger> m_defaultLogger;
        std::shared_ptr<LogFormatter> m_defaultFormatter;
    };

    // ===========================================================================
    // Logger
    // ===========================================================================
    class Logger {
    public:
        explicit Logger(string_view_t apartment, LoggerInitProc proc = [](Logger&) {});
        ~Logger();

        string_t GetApartment() const noexcept { return m_apartment; }

        template<typename... Args>
        void Debug(string_view_t fmt, Args&&... args) noexcept {
            if constexpr (sizeof...(Args) == 0) {
                Log(LogLevel::Debug, fmt);
            }
            else {
                auto tup = std::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...);
                std::apply([&](auto&... vals) {
                    Log(LogLevel::Debug, std::vformat(fmt, make_tformat_args(vals...)));
                    }, tup);
            }
        }

        template<typename... Args>
        void Info(string_view_t fmt, Args&&... args) noexcept {
            if constexpr (sizeof...(Args) == 0) {
                Log(LogLevel::Info, fmt);
            }
            else {
                auto tup = std::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...);
                std::apply([&](auto&... vals) {
                    Log(LogLevel::Info, std::vformat(fmt, make_tformat_args(vals...)));
                    }, tup);
            }
        }

        template<typename... Args>
        void Warn(string_view_t fmt, Args&&... args) noexcept {
            if constexpr (sizeof...(Args) == 0) {
                Log(LogLevel::Warn, fmt);
            }
            else {
                auto tup = std::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...);
                std::apply([&](auto&... vals) {
                    Log(LogLevel::Warn, std::vformat(fmt, make_tformat_args(vals...)));
                    }, tup);
            }
        }

        template<typename... Args>
        void Error(string_view_t fmt, Args&&... args) noexcept {
            if constexpr (sizeof...(Args) == 0) {
                Log(LogLevel::Error, fmt);
            }
            else {
                auto tup = std::tuple<std::decay_t<Args>...>(std::forward<Args>(args)...);
                std::apply([&](auto&... vals) {
                    Log(LogLevel::Error, std::vformat(fmt, make_tformat_args(vals...)));
                    }, tup);
            }
        }

        void Log(LogLevel level, string_view_t msg) const noexcept;

        void DLog(LogLevel level, string_view_t msg) const noexcept {
#ifdef WU_DEBUG
            Log(level, msg);
#endif
        }

    private:
        string_t m_apartment;
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
    }

} // namespace WinUtils