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
#include <vector>
#include <string>
#include <ranges>
#include <cctype>
#include <wctype.h>

#include "WinUtils/Logger.h"
#include "WinUtils/CmdParser.h"
using namespace std;
namespace WinUtils {

	static Logger logger(TS("CmdParser"));

	// -----------------------------------------------------------------------------
	// Static helpers
	// -----------------------------------------------------------------------------
	vector<string_t> split_whitespace(const string_t& input)
	{
		vector<string_t> tokens;
		bool inToken = false;
		string_t currentToken;

		for (char_t c : input) {
#if WU_WIDE_STRING
			bool isSpace = iswspace(static_cast<wint_t>(c));
#else
			bool isSpace = isspace(static_cast<unsigned char>(c));
#endif
			if (isSpace) {
				if (inToken) {
					tokens.push_back(move(currentToken));
					currentToken.clear();
					inToken = false;
				}
			}
			else {
				currentToken += c;
				inToken = true;
			}
		}
		if (inToken) {
			tokens.push_back(move(currentToken));
		}
		return tokens;
	}

	bool CmdParser::isTokenValid(string_view_t token) noexcept
	{
		if (hasQuotation(token)) return true;
		return token.find(TS(' ')) == string_view_t::npos;
	}

	bool CmdParser::hasQuotation(string_view_t token) noexcept
	{
		return token.size() >= 2 && token.front() == TS('"') && token.back() == TS('"');
	}

	string_t CmdParser::removeQuotation(string_view_t token) noexcept
	{
		if (!hasQuotation(token)) {
			return string_t(token);
		}
		return string_t(token.substr(1, token.size() - 2));
	}

	bool CmdParser::isQuotationMatched(string_view_t input) noexcept
	{
		bool inQuotes = false;
		for (wchar_t c : input) {
			if (c == TS('"')) {
				inQuotes = !inQuotes;
			}
		}
		return !inQuotes;
	}
	vector<string_t> CmdParser::tokenize(string_view_t input)
	{
		vector<string_t> tokens;
		string_t current;
		bool inQuotes = false;
		bool escape = false;

		for (size_t i = 0; i < input.size(); ++i) {
			char_t c = input[i];

			if (escape) {
				current += c;
				escape = false;
				continue;
			}

			if (c == TS('\\')) {
				if (i + 1 < input.size() && input[i + 1] == TS('"')) {
					escape = true;
					continue;
				}
				else {
					current += c;
				}
				continue;
			}

			if (c == TS('"')) {
				if (!inQuotes) {
					inQuotes = true;
				}
				else {
					inQuotes = false;
				}
				continue;
			}

			if ((static_cast<wint_t>(c) == L' ') && !inQuotes) {
				if (!current.empty()) {
					tokens.push_back(std::move(current));
					current.clear();
				}
			}
			else {
				current += c;
			}
		}

		if (!current.empty()) {
			tokens.push_back(std::move(current));
		}

		// 若引号未闭合，可根据策略抛异常或忽略；此处按原行为保持原样（由调用方 isQuotationMatched 提前拦截）
		return tokens;
	}

	// -----------------------------------------------------------------------------
	// Prefix detection and command classification
	// -----------------------------------------------------------------------------
	bool CmdParser::isCommand(string_view_t token) const noexcept
	{
		if (token.empty()) return false;

		if (token.size() >= 2 && token[0] == TS('-') && token[1] == TS('-')) {
			return true;
		}
		if (token.size() >= 1 && (token[0] == TS('-') || token[0] == TS('/'))) {
			return true;
		}
		return false;
	}

	size_t CmdParser::getPrefixLength(string_view_t token) noexcept
	{
		if (token.size() >= 2 && token[0] == TS('-') && token[1] == TS('-')) {
			return 2;
		}
		if (token.size() >= 1 && (token[0] == TS('-') || token[0] == TS('/'))) {
			return 1;
		}
		return 0;
	}

	string_t CmdParser::normalizeCommand(string_view_t cmd) const noexcept
	{
		if (!m_caseInsensitive) {
			return string_t(cmd);
		}

		string_t normalized(cmd);
		for (auto& c : normalized) {
			if (c >= TS('A') && c <= TS('Z')) {
				c = c + (TS('a') - TS('A'));
			}
		}
		return normalized;
	}

	// -----------------------------------------------------------------------------
	// Public parsing logic
	// -----------------------------------------------------------------------------
	bool CmdParser::parse(string_view_t commandLine, ParseMode mode)
	{
		clear();

		if (mode == ParseMode::None) {
			if (commandLine.find_first_of(TS("-/")) != string_view_t::npos)
				mode = ParseMode::Normal;
			else
				mode = ParseMode::NoFlag;
		}

		if (mode == ParseMode::NoFlag) {
			auto tokens = tokenize(commandLine);
			for (auto& tok : tokens) {
				tok = removeQuotation(tok);
			}
			if (!tokens.empty()) {
				m_commands[TS("")] = std::move(tokens);
			}
			m_parseSuccess = true;
			return true;
		}

		m_parseSuccess = false;

		if (!isQuotationMatched(commandLine)) {
			logger.DLog(LogLevel::Error, TS("CmdParser: Unmatched quotation marks"));
			return false;
		}

		vector<string_t> tokens = tokenize(commandLine);
		if (tokens.empty()) {
			m_parseSuccess = true;
			return true;
		}

		string_t currentCommand;
		bool hasSeenCommand = false;   // track whether any command has appeared

		for (const auto& token : tokens) {
			if (isCommand(token)) {
				size_t prefixLen = getPrefixLength(token);
				string_view_t rawCmdPart(token.data() + prefixLen, token.size() - prefixLen);
				string_t cleanCmdPart = removeQuotation(rawCmdPart);

				string_t cmdName;
				optional<string_t> immediateParam;
				size_t eqPos = cleanCmdPart.find(TS('='));
				if (eqPos != string_t::npos) {
					cmdName = normalizeCommand(cleanCmdPart.substr(0, eqPos));
					immediateParam = cleanCmdPart.substr(eqPos + 1);
				}
				else {
					cmdName = normalizeCommand(cleanCmdPart);
				}

				currentCommand = cmdName;
				hasSeenCommand = true;
				m_commands[currentCommand]; // ensure existence

				if (immediateParam.has_value()) {
					m_commands[currentCommand].push_back(removeQuotation(immediateParam.value()));
				}
			}
			else {
				// Non‑command token
				if (!hasSeenCommand) {
					// Orphaned parameter → store under empty command
					m_commands[TS("")].push_back(removeQuotation(token));
				}
				else {
					// Attach to current command
					m_commands[currentCommand].push_back(removeQuotation(token));
				}
			}
		}

		m_parseSuccess = true;
		return true;
	}

	// -----------------------------------------------------------------------------
	// Query methods
	// -----------------------------------------------------------------------------
	bool CmdParser::hasCommand(string_view_t cmd) const
	{
		string_t normCmd;
		if (cmd.empty()) {
			normCmd = TS("");
		}
		else if (isCommand(cmd)) {
			size_t prefixLen = getPrefixLength(cmd);
			normCmd = normalizeCommand(cmd.substr(prefixLen));
		}
		else {
			normCmd = normalizeCommand(cmd);
		}
		return m_commands.find(normCmd) != m_commands.end();
	}

	size_t CmdParser::getParamCount(string_view_t cmd) const noexcept
	{
		string_t normCmd;
		if (isCommand(cmd)) {
			size_t prefixLen = getPrefixLength(cmd);
			normCmd = normalizeCommand(cmd.substr(prefixLen));
		}
		else {
			normCmd = normalizeCommand(cmd);
		}

		auto it = m_commands.find(normCmd);
		if (it == m_commands.end()) return 0;
		return it->second.size();
	}

	optional<string_t> CmdParser::getParam(string_view_t cmd, size_t index) const noexcept
	{
		string_t normCmd;
		if (isCommand(cmd)) {
			size_t prefixLen = getPrefixLength(cmd);
			normCmd = normalizeCommand(cmd.substr(prefixLen));
		}
		else {
			normCmd = normalizeCommand(cmd);
		}

		auto it = m_commands.find(normCmd);
		if (it == m_commands.end() || index >= it->second.size()) {
			return nullopt;
		}
		return it->second[index];
	}

	vector<string_t> CmdParser::getParams(string_view_t cmd) const noexcept
	{
		string_t normCmd;
		if (isCommand(cmd)) {
			size_t prefixLen = getPrefixLength(cmd);
			normCmd = normalizeCommand(cmd.substr(prefixLen));
		}
		else {
			normCmd = normalizeCommand(cmd);
		}

		auto it = m_commands.find(normCmd);
		if (it == m_commands.end()) {
			return {};
		}
		return it->second;
	}

	vector<string_t> CmdParser::getAllCommands() const
	{
		vector<string_t> cmds;
		cmds.reserve(m_commands.size());
		for (const auto& [cmd, _] : m_commands) {
			cmds.push_back(cmd);
		}
		return cmds;
	}

} // namespace WinUtils