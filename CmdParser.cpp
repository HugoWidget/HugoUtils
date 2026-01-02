#include "Logger.h"
#include "CmdParser.h"
using namespace WinUtils;
using namespace std;
// 解析命令行
bool CmdParser::parse(std::wstring_view commandLine) {
    clear();
    m_parseSuccess = false;

    if (!isQuotationMatched(commandLine)) {
        WuDebug(LogLevel::Error, L"CmdParser:无法匹配的括号");
        return false;
    }

    // 分词处理
    vector<wstring> tokens = tokenize(commandLine);
    if (tokens.empty()) {
        m_parseSuccess = true;
        return true;
    }

    // 解析命令-参数映射
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

// 检查是否包含指定命令
bool CmdParser::hasCommand(std::wstring_view cmd) const {
    if (cmd.empty() || !isCommand(cmd)) return false;
    return m_commands.find(normalizeCommand(cmd)) != m_commands.end();
}

// 获取指定命令的参数数量
size_t CmdParser::getParamCount(std::wstring_view cmd) const noexcept {
    auto normalizedCmd = normalizeCommand(cmd);
    auto it = m_commands.find(normalizedCmd);
    return (it != m_commands.end()) ? it->second.size() : 0;
}

// 安全获取指定索引的参数
optional<wstring> CmdParser::getParam(
    std::wstring_view cmd, size_t index) const noexcept {
    auto normalizedCmd = normalizeCommand(cmd);
    auto it = m_commands.find(normalizedCmd);
    if (it == m_commands.end() || index >= it->second.size()) {
        return nullopt;
    }
    return it->second[index];
}

// 获取所有命令名
vector<wstring> CmdParser::getAllCommands() const {
    vector<wstring> cmds;
    cmds.reserve(m_commands.size());
    for (const auto& [cmd, _] : m_commands) {
        cmds.push_back(cmd);
    }
    return cmds;
}

// 检查token是否有效
bool CmdParser::isTokenValid(std::wstring_view token) noexcept {
    if (hasQuotation(token)) return true;
    return token.find(L' ') == wstring_view::npos;
}

// 检查是否包含配对的引号
bool CmdParser::hasQuotation(std::wstring_view token) noexcept {
    return token.size() >= 2
        && token.front() == L'"'
        && token.back() == L'"';
}

// 移除token两端的引号
wstring CmdParser::removeQuotation(std::wstring_view token) noexcept {
    if (!hasQuotation(token)) {
        return wstring(token);
    }
    return wstring(token.substr(1, token.size() - 2));
}

// 检查命令行引号是否配对
bool CmdParser::isQuotationMatched(std::wstring_view input) noexcept {
    bool inQuotes = false;
    for (wchar_t c : input) {
        if (c == L'"') {
            inQuotes = !inQuotes;
        }
    }
    return !inQuotes;
}

// 判断是否是命令
bool CmdParser::isCommand(std::wstring_view token) const noexcept {
    return token.size() > 1 && token.front() == L'-';
}

// 分词核心逻辑
vector<wstring> CmdParser::tokenize(std::wstring_view input) {
    vector<wstring> tokens;
    wstring currentToken;
    bool inQuotes = false;

    for (wchar_t c : input) {
        if (c == L'"') {
            inQuotes = !inQuotes;
            currentToken += c; // 保留引号
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

    // 处理最后一个token
    if (!currentToken.empty()) {
        tokens.push_back(move(currentToken));
    }

    return tokens;
}

// 标准化命令名
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