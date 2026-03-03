#include "Logger.h"
#include "CmdParser.h"
#include <vector>
#include <string>
#include <ranges>
#include <cctype>
using namespace WinUtils;
using namespace std;
static Logger logger(TS("CmdParser"));

std::vector<string_t> split_whitespace(const string_t& input)
{
    namespace ranges = std::ranges;
    namespace views = std::views;
    auto token_views = input
        | views::split(TS(' '))                    
        | views::transform([](auto&& r) {       
        return string_t(ranges::begin(r), ranges::end(r));
            })
        | views::filter([](const string_t& s) {  
        return !s.empty();
            });
    return std::vector<string_t>(ranges::begin(token_views), ranges::end(token_views));
}

bool CmdParser::parse(string_view_t commandLine, ParseMode mode) {
    clear();
    if (mode == ParseMode::None) {
        size_t found = commandLine.find_first_of(TS("\\-"));
        size_t len = commandLine.length();
        if (found == string_t::npos)
            mode = ParseMode::NoFlag;
        else
            mode = ParseMode::Normal;
    }
    if (mode == ParseMode::NoFlag) {
        string_t cmd = string_t(commandLine);
        m_commands.insert({ TS(""), split_whitespace(cmd) });
        return true;
    }
    m_parseSuccess = false;

    if (!isQuotationMatched(commandLine)) {
        logger.DLog(LogLevel::Error, TS("CmdParser: Unmatched quotation marks"));
        return false;
    }

    // Tokenize the input command line
    vector<string_t> tokens = tokenize(commandLine);
    if (tokens.empty()) {
        m_parseSuccess = true;
        return true;
    }

    // Parse command-parameter mappings
    string_t currentCmd;
    for (const auto& token : tokens) {
        if (isCommand(token)) {
            currentCmd = normalizeCommand(token);
            m_commands[currentCmd];
        }
        else if (!currentCmd.empty()) {
            m_commands[currentCmd].push_back(removeQuotation(token));
        }
    }

    m_parseSuccess = true;
    return true;
}

// Check if the specified command exists
bool CmdParser::hasCommand(string_view_t cmd) const {
    if (cmd.empty() || !isCommand(cmd)) return false;
    return m_commands.find(normalizeCommand(cmd)) != m_commands.end();
}

// Get the number of parameters for the specified command
size_t CmdParser::getParamCount(string_view_t cmd) const noexcept {
    auto normalizedCmd = normalizeCommand(cmd);
    auto it = m_commands.find(normalizedCmd);
    return (it != m_commands.end()) ? it->second.size() : 0;
}

// Safely get the parameter at the specified index
optional<string_t> CmdParser::getParam(
    string_view_t cmd, size_t index) const noexcept {
    auto normalizedCmd = normalizeCommand(cmd);
    auto it = m_commands.find(normalizedCmd);
    if (it == m_commands.end() || index >= it->second.size()) {
        return nullopt;
    }
    return it->second[index];
}

// Safely get all parameters for the specified command
vector<string_t> CmdParser::getParams(
    string_view_t cmd) const noexcept {
    auto normalizedCmd = normalizeCommand(cmd);
    auto it = m_commands.find(normalizedCmd);
    if (it == m_commands.end()) {
        return {};
    }
    return it->second;
}

// Get all command names
vector<string_t> CmdParser::getAllCommands() const {
    vector<string_t> cmds;
    cmds.reserve(m_commands.size());
    for (const auto& [cmd, _] : m_commands) {
        cmds.push_back(cmd);
    }
    return cmds;
}

// Check if a token is valid
bool CmdParser::isTokenValid(string_view_t token) noexcept {
    if (hasQuotation(token)) return true;
    return token.find(TS(' ')) == string_view_t::npos;
}

// Check if the token is wrapped with paired quotation marks
bool CmdParser::hasQuotation(string_view_t token) noexcept {
    return token.size() >= 2
        && token.front() == TS('"')
        && token.back() == TS('"');
}

// Remove quotation marks from both ends of the token
string_t CmdParser::removeQuotation(string_view_t token) noexcept {
    if (!hasQuotation(token)) {
        return string_t(token);
    }
    return string_t(token.substr(1, token.size() - 2));
}

// Check if all quotation marks in the command line are matched
bool CmdParser::isQuotationMatched(string_view_t input) noexcept {
    bool inQuotes = false;
    for (wchar_t c : input) {
        if (c == TS('"')) {
            inQuotes = !inQuotes;
        }
    }
    return !inQuotes;
}

// Determine if the token is a valid command
bool CmdParser::isCommand(string_view_t token) const noexcept {
    return token.size() > 1 && token.front() == TS('-');
}

// Core tokenization logic for command line
vector<string_t> CmdParser::tokenize(string_view_t input) {
    vector<string_t> tokens;
    string_t currentToken;
    bool inQuotes = false;

    for (char_t c : input) {
        if (c == TS('"')) {
            inQuotes = !inQuotes;
            currentToken += c; // Preserve quotation marks
        }
        else if (isspace(static_cast<wint_t>(c)) && !inQuotes) {
            if (!currentToken.empty()) {
                tokens.push_back(move(currentToken));
                currentToken.clear();
            }
        }
        else {
            currentToken += c;
        }
    }

    // Process the last remaining token
    if (!currentToken.empty()) {
        tokens.push_back(move(currentToken));
    }

    return tokens;
}

// Normalize the command name (case-insensitive if enabled)
string_t CmdParser::normalizeCommand(string_view_t cmd) const noexcept {
    if (!m_caseInsensitive) {
        return string_t(cmd);
    }
    string_t normalized(cmd);
    for (auto& c : normalized) {
#if USE_WIDE_STRING
        c = static_cast<wchar_t>(towlower(c));
#else
		c = static_cast<char>(tolower(c));
#endif
    }
    return normalized;
}