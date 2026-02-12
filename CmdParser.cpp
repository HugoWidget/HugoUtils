#include "Logger.h"
#include "CmdParser.h"
#include <vector>
#include <string>
#include <ranges>
#include <cctype>
using namespace WinUtils;
using namespace std;
static Logger logger(L"CmdParser");

std::vector<std::wstring> wstring_split_whitespace(const std::wstring& input)
{
    namespace ranges = std::ranges;
    namespace views = std::views;
    auto token_views = input
        | views::split(L' ')                    
        | views::transform([](auto&& r) {       
        return std::wstring(ranges::begin(r), ranges::end(r));
            })
        | views::filter([](const std::wstring& s) {  
        return !s.empty();
            });
    return std::vector<std::wstring>(ranges::begin(token_views), ranges::end(token_views));
}

bool CmdParser::parse(std::wstring_view commandLine, ParseMode mode) {
    clear();
    if (mode == ParseMode::None) {
        size_t found = commandLine.find(L"\\-");
        size_t len = commandLine.length();
        if (found == wstring::npos)
            mode = ParseMode::NoFlag;
        else
            mode = ParseMode::Normal;
    }
    if (mode == ParseMode::NoFlag) {
        wstring cmd = wstring(commandLine);
        m_commands.insert({ L"", wstring_split_whitespace(cmd) });
        return true;
    }
    m_parseSuccess = false;

    if (!isQuotationMatched(commandLine)) {
        logger.DLog(LogLevel::Error, L"CmdParser: Unmatched quotation marks");
        return false;
    }

    // Tokenize the input command line
    vector<wstring> tokens = tokenize(commandLine);
    if (tokens.empty()) {
        m_parseSuccess = true;
        return true;
    }

    // Parse command-parameter mappings
    wstring currentCmd;
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
bool CmdParser::hasCommand(std::wstring_view cmd) const {
    if (cmd.empty() || !isCommand(cmd)) return false;
    return m_commands.find(normalizeCommand(cmd)) != m_commands.end();
}

// Get the number of parameters for the specified command
size_t CmdParser::getParamCount(std::wstring_view cmd) const noexcept {
    auto normalizedCmd = normalizeCommand(cmd);
    auto it = m_commands.find(normalizedCmd);
    return (it != m_commands.end()) ? it->second.size() : 0;
}

// Safely get the parameter at the specified index
optional<wstring> CmdParser::getParam(
    std::wstring_view cmd, size_t index) const noexcept {
    auto normalizedCmd = normalizeCommand(cmd);
    auto it = m_commands.find(normalizedCmd);
    if (it == m_commands.end() || index >= it->second.size()) {
        return nullopt;
    }
    return it->second[index];
}

// Safely get all parameters for the specified command
vector<wstring> CmdParser::getParams(
    std::wstring_view cmd) const noexcept {
    auto normalizedCmd = normalizeCommand(cmd);
    auto it = m_commands.find(normalizedCmd);
    if (it == m_commands.end()) {
        return {};
    }
    return it->second;
}

// Get all command names
vector<wstring> CmdParser::getAllCommands() const {
    vector<wstring> cmds;
    cmds.reserve(m_commands.size());
    for (const auto& [cmd, _] : m_commands) {
        cmds.push_back(cmd);
    }
    return cmds;
}

// Check if a token is valid
bool CmdParser::isTokenValid(std::wstring_view token) noexcept {
    if (hasQuotation(token)) return true;
    return token.find(L' ') == wstring_view::npos;
}

// Check if the token is wrapped with paired quotation marks
bool CmdParser::hasQuotation(std::wstring_view token) noexcept {
    return token.size() >= 2
        && token.front() == L'"'
        && token.back() == L'"';
}

// Remove quotation marks from both ends of the token
wstring CmdParser::removeQuotation(std::wstring_view token) noexcept {
    if (!hasQuotation(token)) {
        return wstring(token);
    }
    return wstring(token.substr(1, token.size() - 2));
}

// Check if all quotation marks in the command line are matched
bool CmdParser::isQuotationMatched(std::wstring_view input) noexcept {
    bool inQuotes = false;
    for (wchar_t c : input) {
        if (c == L'"') {
            inQuotes = !inQuotes;
        }
    }
    return !inQuotes;
}

// Determine if the token is a valid command
bool CmdParser::isCommand(std::wstring_view token) const noexcept {
    return token.size() > 1 && token.front() == L'-';
}

// Core tokenization logic for command line
vector<wstring> CmdParser::tokenize(std::wstring_view input) {
    vector<wstring> tokens;
    wstring currentToken;
    bool inQuotes = false;

    for (wchar_t c : input) {
        if (c == L'"') {
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
wstring CmdParser::normalizeCommand(std::wstring_view cmd) const noexcept {
    if (!m_caseInsensitive) {
        return wstring(cmd);
    }
    wstring normalized(cmd);
    for (auto& c : normalized) {
        c = static_cast<wchar_t>(tolower(static_cast<wint_t>(c)));
    }
    return normalized;
}