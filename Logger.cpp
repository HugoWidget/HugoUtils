#include "Logger.h"
#include "WinUtils.h"
#include <fstream>
#include <iostream>
#include <ctime>
#include <io.h>
#include <fcntl.h>
using namespace WinUtils;
using namespace std;
// 文件输出策略
FileLogStrategy::FileLogStrategy(std::wstring_view log_path) : log_path_(log_path) {}

void FileLogStrategy::Output(LogLevel /*level*/, const std::wstring& formatted_log) noexcept {
    lock_guard<mutex> lock(file_mutex_);
    try {
        wofstream file(log_path_, ios::app | ios::out);
        if (file.is_open()) {
            file << formatted_log << L"\r\n";
            file.close();
        }
    }
    catch (...) {
    }
}

// 控制台输出策略
ConsoleLogStrategy::ConsoleLogStrategy() {
    try {
        console_inited_ = true;
    }
    catch (...) {
        console_inited_ = false;
    }
}

void ConsoleLogStrategy::Output(LogLevel level, const std::wstring& formatted_log) noexcept {
    if (!console_inited_) return;
    try {
        if (level == LogLevel::Error) {
            wcerr << formatted_log << L"\n";
            wcerr.flush();
        }
        else {
            wcout << formatted_log << L"\n";
            wcout.flush();
        }
    }
    catch (...) {}
}

// 调试器输出策略
void DebugLogStrategy::Output(LogLevel /*level*/, const std::wstring& formatted_log) noexcept {
    OutputDebugStringW((formatted_log + L"\n").c_str());
}

void WinUtils::Logger::AddFormat(LogFormat format)
{
    m_format.push_back(format);
}

void WinUtils::Logger::ClearFormat()
{
    m_format.clear();
}

bool WinUtils::Logger::HasFormat(LogFormat format)
{
    return find(m_format.begin(),m_format.end(),format)!=m_format.end();
}

void Logger::AddStrategy(std::unique_ptr<LogOutputStrategy> strategy) noexcept {
    if (!strategy) return;
    lock_guard<mutex> lock(m_loggerMutex);
    m_strategies.emplace_back(move(strategy));
}

void Logger::ClearStrategies() noexcept {
    lock_guard<mutex> lock(m_loggerMutex);
    m_strategies.clear();
}

void Logger::SetDefaultStrategies(std::wstring_view log_path) noexcept {
    ClearStrategies();
    wstring path = log_path.empty() ? (WinUtils::GetCurrentProcessDir() + L"log.txt") : wstring(log_path);
    AddStrategy(make_unique<FileLogStrategy>(path));
    AddStrategy(make_unique<DebugLogStrategy>());
}

std::wstring Logger::GetFormattedTime() noexcept {
    const auto now = chrono::system_clock::now();
    const auto time_t_now = chrono::system_clock::to_time_t(now);
    tm local_tm{};
    localtime_s(&local_tm, &time_t_now);

    wchar_t time_buf[32] = { 0 };
    swprintf_s(time_buf, L"%04d-%02d-%02d %02d:%02d:%02d",
        local_tm.tm_year + 1900, local_tm.tm_mon + 1, local_tm.tm_mday,
        local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec);
    return time_buf;
}

std::wstring Logger::FormatLog(LogLevel level, std::wstring_view msg) noexcept {
    wstringstream formattedLog;
    const wchar_t* level_str = L"DEBUG";
    if (level == LogLevel::Info) level_str = L"INFO";
    if (level == LogLevel::Warn) level_str = L"WARN";
    if (level == LogLevel::Error) level_str = L"ERROR";
    if (HasFormat(LogFormat::Time)) 
        formattedLog << L"[" << GetFormattedTime() << "]";
    if (HasFormat(LogFormat::Level))
        formattedLog << L"[" << level_str << L"]";
    formattedLog << msg.data();

    return formattedLog.str();
}

void Logger::Log(LogLevel level, std::wstring_view msg) noexcept {
    if (msg.empty()) return;
    const auto formatted_log = FormatLog(level, msg);
    lock_guard<mutex> lock(m_loggerMutex);
    // 遍历所有策略执行输出
    for (const auto& strategy : m_strategies) {
        if (strategy) {
            strategy->Output(level, formatted_log);
        }
    }
}