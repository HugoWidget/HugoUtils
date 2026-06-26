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

#include <Windows.h>
#include <chrono>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <io.h>
#include <iostream>
#include <sstream>

#include "WinUtils/Logger.h"
#include "WinUtils/StrConvert.h"

using namespace WinUtils;
using namespace std;

static string_t GetCurrentProcessDir2() {
    char_t path[MAX_PATH] = { 0 };
    TF(GetModuleFileName)(nullptr, path, _countof(path));
    string_t str(path);
    size_t pos = str.find_last_of(TS("\\/"));
    return (pos == string_t::npos) ? TS("") : string_t(str.substr(0, pos + 1));
}

// --- LogFormatter ---
string_t LogFormatter::GetFormattedTime() noexcept {
    const auto now = std::time(nullptr);
    std::tm local_tm;
    localtime_s(&local_tm, &now);
    char_t buf[64];
#ifdef WU_NARROW_STRING
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &local_tm);
#else
    wcsftime(buf, sizeof(buf) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &local_tm);
#endif
    return string_t(buf);
}

const char_t* LogFormatter::LevelToString(LogLevel level) noexcept {
    switch (level) {
    case LogLevel::Info:  return TS("INFO");
    case LogLevel::Warn:  return TS("WARN");
    case LogLevel::Error: return TS("ERROR");
    default:              return TS("DEBUG");
    }
}

string_t LogFormatter::Format(LogLevel level, string_view_t msg) const {
    stringstream_t ss;
    if (HasFormat(LogFormat::Time))   ss << TS("[") << GetFormattedTime() << TS("]");
    if (HasFormat(LogFormat::Level))  ss << TS("[") << LevelToString(level) << TS("]");
    ss << msg.data();
    return ss.str();
}

// --- FileLogStrategy ---
FileLogStrategy::FileLogStrategy(string_view_t logPath, std::shared_ptr<LogFormatter> fmt)
    : m_logPath(logPath),
    m_formatter(fmt ? std::move(fmt) : std::make_shared<LogFormatter>()) {
}

void FileLogStrategy::Output(LogLevel level, const string_view_t msg) noexcept {
    std::lock_guard<std::mutex> lock(m_fileMutex);
    string_t formatted = m_formatter->Format(level, msg) + TS('\n');
    string str = ConvertString<string>(formatted);
    FILE* f = nullptr;
    try {
#ifdef WU_NARROW_STRING
        fopen_s(&f, m_logPath.c_str(), "a");
#else
        _wfopen_s(&f, m_logPath.c_str(), L"a");
#endif
        if (!f) return;
        fwrite(str.c_str(), sizeof(char), str.length(), f);
        fflush(f);
        fclose(f);
    }
    catch (...) {
    }
}

// --- ConsoleLogStrategy ---
ConsoleLogStrategy::ConsoleLogStrategy(std::shared_ptr<LogFormatter> fmt)
    : m_formatter(fmt ? std::move(fmt) : std::make_shared<LogFormatter>()) {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != nullptr && hOut != INVALID_HANDLE_VALUE) {
        DWORD mode;
        if (GetConsoleMode(hOut, &mode))
            m_consoleAvailable = true;
    }
}

void ConsoleLogStrategy::Output(LogLevel level, const string_view_t msg) noexcept {
    if (!m_consoleAvailable) return;
    try {
        string_t formatted = m_formatter->Format(level, msg);
        if (level == LogLevel::Error) {
            tcerr << formatted << TS("\n");
            tcerr.flush();
        }
        else {
            tcout << formatted << TS("\n");
            tcout.flush();
        }
    }
    catch (...) {
    }
}

// --- DebugLogStrategy ---
DebugLogStrategy::DebugLogStrategy(std::shared_ptr<LogFormatter> fmt)
    : m_formatter(fmt ? std::move(fmt) : std::make_shared<LogFormatter>()) {
}

void DebugLogStrategy::Output(LogLevel level, const string_view_t msg) noexcept {
    TF(OutputDebugString)((m_formatter->Format(level, msg) + TS("\n")).c_str());
}

// --- Logger ---
Logger::Logger(string_view_t apartment, LoggerInitProc proc)
    : m_apartment(apartment) {
    LoggerCore::Inst().AddLogger(*this);
    proc(*this);
}

Logger::~Logger() {
    LoggerCore::Inst().DeleteLogger(m_apartment);
}

void Logger::Log(LogLevel level, string_view_t msg) const noexcept {
    if (msg.empty()) return;
    if (!LoggerCore::Inst().IsApartmentEnabled(m_apartment))
        return;

    LoggerCore::Inst().Log(level, msg);
}

// --- LoggerCore ---
LoggerCore::LoggerCore()
    : m_defaultFormatter(std::make_shared<LogFormatter>()),
    m_allEnabled(false) {
}

LoggerCore::~LoggerCore() = default;

LoggerCore& LoggerCore::Inst() noexcept {
    static LoggerCore instance;
    return instance;
}

std::shared_ptr<LogFormatter> LoggerCore::GetDefaultFormatter() const {
    std::lock_guard<std::mutex> lock(m_formatterMutex);
    return m_defaultFormatter;
}

void LoggerCore::SetDefaultFormatter(std::shared_ptr<LogFormatter> fmt) {
    std::lock_guard<std::mutex> lock(m_formatterMutex);
    m_defaultFormatter = std::move(fmt);
}

void LoggerCore::AddFileStrategy(string_view_t path, std::shared_ptr<LogFormatter> fmt) {
    AddStrategy<FileLogStrategy>(path, fmt);
}

void LoggerCore::AddConsoleStrategy(std::shared_ptr<LogFormatter> fmt) {
    AddStrategy<ConsoleLogStrategy>(fmt);
}

void LoggerCore::AddDebugStrategy(std::shared_ptr<LogFormatter> fmt) {
    AddStrategy<DebugLogStrategy>(fmt);
}

void LoggerCore::ClearStrategies() noexcept {
    std::lock_guard<std::mutex> lock(m_loggerMutex);
    m_strategies.clear();
}

void LoggerCore::SetDefaultStrategies(string_view_t logPath) noexcept {
    ClearStrategies();
    string_t path = logPath.empty()
        ? (GetCurrentProcessDir2() + TS("log.txt"))
        : string_t(logPath);
    AddFileStrategy(path);
    AddDebugStrategy();
}

void LoggerCore::EnableAllApartments() noexcept {
    std::lock_guard<std::mutex> lock(m_loggerMutex);
    m_allEnabled = true;
    m_except.clear();
}

void LoggerCore::DisableAllApartments() noexcept {
    std::lock_guard<std::mutex> lock(m_loggerMutex);
    m_allEnabled = false;
    m_except.clear();
}

void LoggerCore::EnableApartment(string_view_t apartment) {
    std::lock_guard<std::mutex> lock(m_loggerMutex);
    if (m_allEnabled)
        m_except.erase(string_t(apartment));
    else
        m_except.insert(string_t(apartment));
}

void LoggerCore::DisableApartment(string_view_t apartment) {
    std::lock_guard<std::mutex> lock(m_loggerMutex);
    if (m_allEnabled)
        m_except.insert(string_t(apartment));
    else
        m_except.erase(string_t(apartment));
}

bool LoggerCore::IsApartmentEnabled(string_view_t apartment) const {
    if (m_allEnabled)
        return m_except.find(string_t(apartment)) == m_except.end();
    else
        return m_except.find(string_t(apartment)) != m_except.end();
}

Logger& LoggerCore::GetDefaultLogger() {
    if (!m_defaultLogger)
        m_defaultLogger = std::make_unique<Logger>(DftLogger);
    return *m_defaultLogger;
}

std::optional<std::reference_wrapper<Logger>> LoggerCore::GetLogger(string_view_t apartment) {
    auto it = std::find_if(m_loggers.begin(), m_loggers.end(),
        [&](Logger* logger) {
            return logger->GetApartment() == apartment;
        });
    if (it != m_loggers.end())
        return **it;
    return std::nullopt;
}

void LoggerCore::AddLogger(Logger& logger) {
    m_loggers.insert(&logger);
}

void LoggerCore::DeleteLogger(string_view_t apartment) {
    std::erase_if(m_loggers,
        [&](Logger* logger) {
            return logger->GetApartment() == apartment;
        });
}

void LoggerCore::Log(LogLevel level, string_view_t msg) noexcept {
    if (msg.empty() || level < m_globalLevel) return;

    std::lock_guard<std::mutex> lock(m_loggerMutex);
    for (const auto& strategy : m_strategies) {
        if (strategy)
            strategy->Output(level, msg);
    }
}