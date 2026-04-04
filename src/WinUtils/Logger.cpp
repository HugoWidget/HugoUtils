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
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <io.h>
#include <iostream>

#include "WinUtils/Logger.h"
#include "WinUtils/StrConvert.h"
#include "WinUtils/WinUtils.h"
using namespace WinUtils;
using namespace std;

FileLogStrategy::FileLogStrategy(string_view_t log_path)
	: m_logPath(log_path) {
}

void FileLogStrategy::Output(LogLevel /*level*/,
	const string_view_t formatted_log) noexcept {
	std::lock_guard<std::mutex> lock(m_fileMutex);
	string str = ConvertString<string>((string_t)formatted_log) + '\n';
	FILE* f = 0;
	try {
#ifndef WU_NARROW_STRING
		_wfopen_s(&f, m_logPath.c_str(), L"a");
#else
		fopen_s(&f, m_logPath.c_str(), "a");
#endif
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
	LogLevel level, const string_view_t formatted_log) noexcept {
	if (!m_isConsoleInitialized)
		return;
	try {
		if (level == LogLevel::Error) {
			tcerr << formatted_log << TS("\n");
			tcerr.flush();
		}
		else {
			tcout << formatted_log << TS("\n");
			tcout.flush();
		}
	}
	catch (...) {
	}
}

//               
void DebugLogStrategy::Output(LogLevel /*level*/,
	const string_view_t formatted_log) noexcept {
	TF(OutputDebugString)((string_t(formatted_log) + TS("\n")).c_str());
}

Logger::Logger(string_view_t apartment, LoggerInitProc proc)
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

void LoggerCore::SetDefaultStrategies(string_view_t log_path) noexcept {
	ClearStrategies();
	string_t path = log_path.empty()
		? (::GetCurrentProcessDir() + TS("log.txt"))
		: string_t(log_path);
	AddStrategy<FileLogStrategy>(path);
	AddStrategy<DebugLogStrategy>();
}

void WinUtils::LoggerCore::EnableAllApartments() noexcept
{
	m_enabledApartments.clear();
}

void WinUtils::LoggerCore::DisableAllApartments() noexcept
{
	for (auto& logger : m_loggers) {
		m_enabledApartments.insert(string_t(logger->GetApartment()));
	}
}

void LoggerCore::EnableApartment(string_view_t apartment) {
	m_enabledApartments.insert(string_t(apartment));
}

void LoggerCore::DisableApartment(string_view_t apartment) {
	m_enabledApartments.erase(string_t(apartment));
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

const std::optional<std::reference_wrapper<Logger>> WinUtils::LoggerCore::GetLogger(string_view_t apartment)
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

void LoggerCore::DeleteLogger(string_view_t apartment) {
	erase_if(m_loggers,
		[&](Logger* logger) {
			return logger->GetApartment() == apartment;
		});
}

LoggerCore::LoggerCore() {}

LoggerCore::~LoggerCore() {}

string_t Logger::GetFormattedTime() const noexcept {
	const auto now_utc = std::chrono::system_clock::now();
	const auto local_time = std::chrono::current_zone()->to_local(now_utc);
	return std::format(TS("{:%Y-%m-%d %H:%M:%S}"), local_time);
}

void Logger::Log(LogLevel level,
	string_view_t msg) const noexcept {
	string_t formatted_msg = FormatLog(level, msg);
	LoggerCore::Inst().Log(level, formatted_msg, m_apartment);
}

string_t Logger::FormatLog(LogLevel level,
	string_view_t msg) const noexcept {
	stringstream_t formattedLog;
	const char_t* level_str = TS("DEBUG");
	if (level == LogLevel::Info)
		level_str = TS("INFO");
	if (level == LogLevel::Warn)
		level_str = TS("WARN");
	if (level == LogLevel::Error)
		level_str = TS("ERROR");
	if (HasFormat(LogFormat::Time))
		formattedLog << TS("[") << GetFormattedTime() << TS("]");
	if (HasFormat(LogFormat::Level))
		formattedLog << TS("[") << level_str << TS("]");
	formattedLog << msg.data();

	return formattedLog.str();
}

LoggerCore& ::LoggerCore::Inst() noexcept {
	static LoggerCore instance;
	return instance;
}

void LoggerCore::Log(LogLevel level, string_view_t msg, string_view_t apartment) noexcept {
	if (msg.empty())
		return;
	if (apartment != TS("") && m_enabledApartments.find(string_t(apartment)) == m_enabledApartments.end())
		return;
	lock_guard<mutex> lock(m_loggerMutex);
	for (const auto& strategy : m_strategies) {
		if (strategy) {
			strategy->Output(level, msg);
		}
	}
}
