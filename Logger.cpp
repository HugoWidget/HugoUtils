#include "Logger.h"
#include "StrConvert.h"
#include "WinUtils.h"
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <io.h>
#include <iostream>
using namespace WinUtils;
using namespace std;

FileLogStrategy::FileLogStrategy(std::wstring_view log_path)
	: m_logPath(log_path) {
}

void FileLogStrategy::Output(LogLevel /*level*/,
	const std::wstring_view formatted_log) noexcept {
	std::lock_guard<std::mutex> lock(m_fileMutex);
	string str = WideStringToUtf8(formatted_log) + "\n";
	FILE* f = 0;
	try {
		_wfopen_s(&f, m_logPath.c_str(), L"a");
		if (!f)
			return;
		fwrite(str.c_str(), sizeof(char), str.length(), f);
		fflush(f);
		fclose(f);
	}
	catch (...) {
		return;
	}
}

ConsoleLogStrategy::ConsoleLogStrategy() {
	try {
		m_isConsoleInitialized = true;
	}
	catch (...) {
		m_isConsoleInitialized = false;
	}
}

void ConsoleLogStrategy::Output(
	LogLevel level, const std::wstring_view formatted_log) noexcept {
	if (!m_isConsoleInitialized)
		return;
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
	catch (...) {
	}
}

//               
void DebugLogStrategy::Output(LogLevel /*level*/,
	const std::wstring_view formatted_log) noexcept {
	OutputDebugStringW((std::wstring(formatted_log) + L"\n").c_str());
}

Logger::Logger(std::wstring_view apartment, LoggerInitProc proc)
	: m_apartment(apartment) {
	LoggerCore::Inst().AddLogger(*this);
	proc(*this);
}

Logger::~Logger() {
	LoggerCore::Inst().DeleteLogger(m_apartment);
}

void Logger::AddFormat(LogFormat format) {
	m_format.push_back(format);
}

void Logger::ClearFormat() { m_format.clear(); }

bool Logger::HasFormat(LogFormat format) const {
	return find(m_format.begin(), m_format.end(), format) != m_format.end();
}

void LoggerCore::ClearStrategies() noexcept {
	lock_guard<mutex> lock(m_loggerMutex);
	m_strategies.clear();
}

void LoggerCore::SetDefaultStrategies(std::wstring_view log_path) noexcept {
	ClearStrategies();
	wstring path = log_path.empty()
		? (::GetCurrentProcessDir() + L"log.txt")
		: wstring(log_path);
	AddStrategy<FileLogStrategy>(path);
	AddStrategy<DebugLogStrategy>();
}

void WinUtils::LoggerCore::EnableAllApartments() noexcept
{
	m_enabledApartments.clear();
}

void WinUtils::LoggerCore::DisableAllApartments() noexcept
{
	for(auto & logger : m_loggers) {
		m_enabledApartments.insert(wstring(logger->GetApartment()));
	}
}

void LoggerCore::EnableApartment(std::wstring_view apartment) {
	m_enabledApartments.insert(std::wstring(apartment));
}

void LoggerCore::DisableApartment(std::wstring_view apartment) {
	m_enabledApartments.erase(std::wstring(apartment));
}

const Logger& LoggerCore::GetDefaultLogger() {
	auto defaultLogger = GetLogger(DftLogger);
	if (defaultLogger) {
		return defaultLogger->get();
	}
	else {
		Logger* defaultLogger = new Logger(DftLogger);
		m_loggers.insert(defaultLogger);
		return *defaultLogger;
	}
}

const std::optional<std::reference_wrapper<Logger>> WinUtils::LoggerCore::GetLogger(std::wstring_view apartment)
{
	auto it = find_if(m_loggers.begin(), m_loggers.end(),
		[&](const Logger* logger) {
			return logger->GetApartment() == apartment;
		});
	if (it != m_loggers.end()) {
		return **it;
	}
	return std::nullopt;
}

void LoggerCore::AddLogger(Logger& logger) {
	m_loggers.insert(&logger);
}

void LoggerCore::DeleteLogger(std::wstring_view apartment) {
	erase_if(m_loggers,
		[&](Logger* logger) {
			return logger->GetApartment() == apartment;
		});
}

LoggerCore::LoggerCore() {}

LoggerCore::~LoggerCore() {}

std::wstring Logger::GetFormattedTime() const noexcept {
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

void Logger::Log(LogLevel level,
	std::wstring_view msg) const noexcept {
	wstring formatted_msg = FormatLog(level, msg);
	LoggerCore::Inst().Log(level, formatted_msg, m_apartment);
}

std::wstring Logger::FormatLog(LogLevel level,
	std::wstring_view msg) const noexcept {
	wstringstream formattedLog;
	const wchar_t* level_str = L"DEBUG";
	if (level == LogLevel::Info)
		level_str = L"INFO";
	if (level == LogLevel::Warn)
		level_str = L"WARN";
	if (level == LogLevel::Error)
		level_str = L"ERROR";
	if (HasFormat(LogFormat::Time))
		formattedLog << L"[" << GetFormattedTime() << "]";
	if (HasFormat(LogFormat::Level))
		formattedLog << L"[" << level_str << L"]";
	formattedLog << msg.data();

	return formattedLog.str();
}

LoggerCore& ::LoggerCore::Inst() noexcept {
	static LoggerCore instance;
	return instance;
}

void LoggerCore::Log(LogLevel level, std::wstring_view msg, std::wstring_view apartment) noexcept {
	if (msg.empty())
		return;
	if (apartment != L"" && m_enabledApartments.find(wstring(apartment)) == m_enabledApartments.end())
		return;
	lock_guard<mutex> lock(m_loggerMutex);
	for (const auto& strategy : m_strategies) {
		if (strategy) {
			strategy->Output(level, msg);
		}
	}
}
