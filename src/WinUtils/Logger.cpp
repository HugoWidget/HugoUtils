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
#include <cstdio>
#include <ctime>
#include <fcntl.h>
#include <io.h>
#include <iostream>

#include "WinUtils/Logger.h"
#include "WinUtils/StrConvert.h"

using namespace WinUtils;
using namespace std;

namespace {

	string_t GetCurrentProcessDir2() {
		char_t path[MAX_PATH] = { 0 };
		TF(GetModuleFileName)(nullptr, path, _countof(path));
		string_t str(path);
		size_t pos = str.find_last_of(TS("\\/"));
		return (pos == string_t::npos) ? TS("") : string_t(str.substr(0, pos + 1));
	}

	// Per-thread scratch buffer: avoids one allocation per log line.
	thread_local string_t g_scratch;

	// Guards late access from statics destroyed after LoggerCore.
	std::atomic<bool> g_coreAlive{ true };

} // namespace

// --- LogFormatter ---
const string_t& LogFormatter::GetFormattedTime() noexcept {
	// Thread-local second-level cache: at most one strftime per second per thread.
	thread_local std::time_t last = static_cast<std::time_t>(-1);
	thread_local string_t cached;

	try {
		const auto now = std::time(nullptr);
		if (now != last) {
			last = now;
			std::tm tm{};
			localtime_s(&tm, &now);

			char_t buf[64] = {};
#ifdef WU_NARROW_STRING
			std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
#else
			std::wcsftime(buf, sizeof(buf) / sizeof(wchar_t), L"%Y-%m-%d %H:%M:%S", &tm);
#endif
			cached = buf;
		}
	}
	catch (...) {
		// noexcept contract: fall back to the last successfully cached value.
	}
	return cached;
}

const char_t* LogFormatter::LevelToString(LogLevel level) noexcept {
	switch (level) {
	case LogLevel::Info:  return TS("INFO");
	case LogLevel::Warn:  return TS("WARN");
	case LogLevel::Error: return TS("ERROR");
	default:              return TS("DEBUG");
	}
}

void LogFormatter::FormatTo(string_t& out, LogLevel level, string_view_t msg) const {
	// Single snapshot load: no lock, no torn state while another thread edits formats.
	auto formats = m_formats.load(std::memory_order_acquire);
	if (formats) {
		for (auto f : *formats) {
			switch (f) {
			case LogFormat::Time:
				out += TS("[");
				out += GetFormattedTime();
				out += TS("]");
				break;
			case LogFormat::Level:
				out += TS("[");
				out += LevelToString(level);
				out += TS("]");
				break;
			}
		}
	}
	out.append(msg.data(), msg.size());
}

string_t LogFormatter::Format(LogLevel level, string_view_t msg) const {
	auto formats = m_formats.load(std::memory_order_acquire);
	const size_t prefixCount = formats ? formats->size() : 0;

	string_t s;
	s.reserve(msg.size() + 24 * prefixCount);
	FormatTo(s, level, msg);
	return s;
}

// --- LogOutputStrategy ---
LogOutputStrategy::LogOutputStrategy(std::shared_ptr<LogFormatter> fmt)
	: m_formatter(fmt ? std::move(fmt) : std::make_shared<LogFormatter>()) {
}

// --- FileLogStrategy ---
FileLogStrategy::FileLogStrategy(string_view_t logPath, std::shared_ptr<LogFormatter> fmt)
	: LogOutputStrategy(std::move(fmt)), m_logPath(logPath) {
	std::lock_guard<std::mutex> lock(m_fileMutex);
	OpenLocked();
}

FileLogStrategy::~FileLogStrategy() {
	std::lock_guard<std::mutex> lock(m_fileMutex);
	if (m_file) {
		fclose(m_file);
		m_file = nullptr;
	}
}

void FileLogStrategy::OpenLocked() noexcept {
	if (m_file) return;

	// Share delete so cleanup tools (e.g. HugoLogs) can remove the log while a
	// sibling process is still running and holding the handle open.
#ifdef WU_NARROW_STRING
	HANDLE h = CreateFileA(m_logPath.c_str(), FILE_APPEND_DATA,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
#else
	HANDLE h = CreateFileW(m_logPath.c_str(), FILE_APPEND_DATA,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
#endif
	if (h == INVALID_HANDLE_VALUE) return;

	int fd = _open_osfhandle(reinterpret_cast<intptr_t>(h), _O_APPEND | _O_TEXT);
	if (fd == -1) {
		CloseHandle(h);
		return;
	}

	FILE* f = _fdopen(fd, "a");
	if (!f) {
		_close(fd);
		return;
	}
	m_file = f;
}

void FileLogStrategy::Output(LogLevel level, const string_view_t msg) noexcept {
	try {
		auto fmt = GetFormatter();
		if (!fmt) return;

		// Format outside the lock; reuse thread-local scratch buffer.
		g_scratch.clear();
		fmt->FormatTo(g_scratch, level, msg);

#ifdef WU_NARROW_STRING
		const string& narrow = g_scratch;
#else
		string narrow = ConvertString<string>(g_scratch);
#endif

		std::lock_guard<std::mutex> lock(m_fileMutex);
		// Retry lazily: the initial open may fail if the directory is not ready yet.
		if (!m_file) OpenLocked();
		if (!m_file) return;

		fwrite(narrow.data(), 1, narrow.size(), m_file);
		fputc('\n', m_file);
		fflush(m_file);
	}
	catch (...) {
		// Output failure must not propagate.
	}
}

// --- ConsoleLogStrategy ---
ConsoleLogStrategy::ConsoleLogStrategy(std::shared_ptr<LogFormatter> fmt)
	: LogOutputStrategy(std::move(fmt)) {
	EnsureConsole();
}

bool ConsoleLogStrategy::EnsureConsole() noexcept {
	if (m_consoleAvailable.load(std::memory_order_relaxed)) return true;

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == nullptr || hOut == INVALID_HANDLE_VALUE) return false;

	DWORD mode = 0;
	if (!GetConsoleMode(hOut, &mode)) return false;

	m_consoleAvailable.store(true, std::memory_order_relaxed);
	return true;
}

void ConsoleLogStrategy::Output(LogLevel level, const string_view_t msg) noexcept {
	if (!EnsureConsole()) return;

	try {
		auto fmt = GetFormatter();
		if (!fmt) return;

		g_scratch.clear();
		fmt->FormatTo(g_scratch, level, msg);

		// Serialize console writes so lines from different threads never interleave.
		std::lock_guard<std::mutex> lock(m_consoleMutex);
		if (level == LogLevel::Error) {
			tcerr << g_scratch << TS("\n");
			tcerr.flush();
		}
		else {
			tcout << g_scratch << TS("\n");
			tcout.flush();
		}
	}
	catch (...) {
		// Output failure must not propagate.
	}
}

// --- DebugLogStrategy ---
DebugLogStrategy::DebugLogStrategy(std::shared_ptr<LogFormatter> fmt)
	: LogOutputStrategy(std::move(fmt)) {
}

void DebugLogStrategy::Output(LogLevel level, const string_view_t msg) noexcept {
	try {
		auto fmt = GetFormatter();
		if (!fmt) return;

		g_scratch.clear();
		fmt->FormatTo(g_scratch, level, msg);
		g_scratch += TS("\n");

		TF(OutputDebugString)(g_scratch.c_str());
	}
	catch (...) {
	}
}

// --- LoggerCore ---
LoggerCore::LoggerCore()
	: m_defaultFormatter(std::make_shared<LogFormatter>()) {
	m_strategies.store(std::make_shared<StrategyVec>(), std::memory_order_release);
}

LoggerCore::~LoggerCore() {
	// Release the default logger while this object is still fully constructed:
	// its destructor calls back into DeleteLogger.
	m_defaultLogger.reset();
	g_coreAlive.store(false, std::memory_order_release);
}

LoggerCore& LoggerCore::Inst() noexcept {
	static LoggerCore instance;
	return instance;
}

std::shared_ptr<LogFormatter> LoggerCore::GetDefaultFormatter() const {
	std::lock_guard<std::mutex> lock(m_formatterMutex);
	return m_defaultFormatter;
}

void LoggerCore::SetDefaultFormatter(std::shared_ptr<LogFormatter> fmt) {
	if (!fmt) return; // Never allow a null default: it would silence all output.
	std::lock_guard<std::mutex> lock(m_formatterMutex);
	m_defaultFormatter = std::move(fmt);
}

void LoggerCore::AddFileStrategy(string_view_t path, std::shared_ptr<LogFormatter> fmt) {
	AddStrategy<FileLogStrategy>(path, std::move(fmt));
}

void LoggerCore::AddConsoleStrategy(std::shared_ptr<LogFormatter> fmt) {
	AddStrategy<ConsoleLogStrategy>(std::move(fmt));
}

void LoggerCore::AddDebugStrategy(std::shared_ptr<LogFormatter> fmt) {
	AddStrategy<DebugLogStrategy>(std::move(fmt));
}

void LoggerCore::ClearStrategies() noexcept {
	try {
		std::lock_guard<std::mutex> lock(m_strategiesWriteMutex);
		m_strategies.store(std::make_shared<StrategyVec>(), std::memory_order_release);
	}
	catch (...) {
	}
}

void LoggerCore::SetDefaultStrategies(string_view_t logPath) noexcept {
	try {
		string_t path = logPath.empty()
			? (GetCurrentProcessDir2() + TS("log.txt"))
			: string_t(logPath);

		auto fmt = GetDefaultFormatter();

		// Build the new strategy list first, then swap under the write lock.
		auto vec = std::make_shared<StrategyVec>();
		vec->push_back(std::make_shared<FileLogStrategy>(path, fmt));
		vec->push_back(std::make_shared<DebugLogStrategy>(fmt));

		std::lock_guard<std::mutex> lock(m_strategiesWriteMutex);
		m_strategies.store(std::move(vec), std::memory_order_release);
	}
	catch (...) {
		// noexcept contract: keep existing strategies on failure.
	}
}

// --- Global level / apartment configuration ---
uint8_t LoggerCore::CalcThresholdUnlocked(string_view_t apartment) const noexcept {
	// Heterogeneous lookup: string_view_t compared directly, no temporary string.
	const bool found = m_except.find(apartment) != m_except.end();
	const bool enabled = m_allEnabled ? !found : found;
	return enabled ? static_cast<uint8_t>(m_globalLevel) : kLogDisabledThreshold;
}

void LoggerCore::RefreshAllLoggersUnlocked() noexcept {
	for (auto* l : m_loggers)
		l->UpdateThreshold(CalcThresholdUnlocked(l->m_apartment));
}

void LoggerCore::SetGlobalLevel(LogLevel level) noexcept {
	std::lock_guard<std::mutex> lock(m_configMutex);
	m_globalLevel = level;
	RefreshAllLoggersUnlocked();
}

LogLevel LoggerCore::GetGlobalLevel() const noexcept {
	std::lock_guard<std::mutex> lock(m_configMutex);
	return m_globalLevel;
}

void LoggerCore::EnableAllApartments() noexcept {
	std::lock_guard<std::mutex> lock(m_configMutex);
	m_allEnabled = true;
	m_except.clear();
	RefreshAllLoggersUnlocked();
}

void LoggerCore::DisableAllApartments() noexcept {
	std::lock_guard<std::mutex> lock(m_configMutex);
	m_allEnabled = false;
	m_except.clear();
	RefreshAllLoggersUnlocked();
}

void LoggerCore::EnableApartment(string_view_t apartment) {
	std::lock_guard<std::mutex> lock(m_configMutex);
	if (m_allEnabled) {
		if (auto it = m_except.find(apartment); it != m_except.end())
			m_except.erase(it);
	}
	else {
		m_except.insert(string_t(apartment));
	}
	RefreshAllLoggersUnlocked();
}

void LoggerCore::DisableApartment(string_view_t apartment) {
	std::lock_guard<std::mutex> lock(m_configMutex);
	if (m_allEnabled) {
		m_except.insert(string_t(apartment));
	}
	else {
		if (auto it = m_except.find(apartment); it != m_except.end())
			m_except.erase(it);
	}
	RefreshAllLoggersUnlocked();
}

bool LoggerCore::IsApartmentEnabled(string_view_t apartment) const {
	std::lock_guard<std::mutex> lock(m_configMutex);
	const bool found = m_except.find(apartment) != m_except.end();
	return m_allEnabled ? !found : found;
}

// --- Logger registry ---
void LoggerCore::AddLogger(Logger& logger) {
	if (!g_coreAlive.load(std::memory_order_acquire)) return;
	std::lock_guard<std::mutex> lock(m_configMutex);
	m_loggers.insert(&logger);
	logger.UpdateThreshold(CalcThresholdUnlocked(logger.m_apartment));
}

void LoggerCore::DeleteLogger(Logger& logger) {
	if (!g_coreAlive.load(std::memory_order_acquire)) return;
	std::lock_guard<std::mutex> lock(m_configMutex);
	m_loggers.erase(&logger);
}

Logger& LoggerCore::GetDefaultLogger() {
	std::call_once(m_defaultLoggerOnce, [this]() {
		m_defaultLogger = std::make_unique<Logger>(DftLogger);
		});
	return *m_defaultLogger;
}

std::optional<std::reference_wrapper<Logger>> LoggerCore::GetLogger(string_view_t apartment) {
	std::lock_guard<std::mutex> lock(m_configMutex);
	for (auto* l : m_loggers) {
		if (string_view_t(l->m_apartment) == apartment)
			return *l;
	}
	return std::nullopt;
}

// --- Logger ---
Logger::Logger(string_view_t apartment, LoggerInitProc proc)
	: m_apartment(apartment) {
	LoggerCore::Inst().AddLogger(*this);
	try {
		proc(*this);
	}
	catch (...) {
		// Roll back registration if user init throws; keep registry consistent.
		LoggerCore::Inst().DeleteLogger(*this);
		throw;
	}
}

Logger::~Logger() {
	LoggerCore::Inst().DeleteLogger(*this);
}
