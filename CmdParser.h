#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <stdexcept>
#include <cctype>
#include <string_view>
#include "WinUtilsDef.h"

using ParserData = std::map<std::wstring, std::vector<std::wstring>>;

class WinUtils::CmdParser {
public:
    explicit CmdParser(bool caseInsensitive = false)
        : m_caseInsensitive(caseInsensitive) {
    }

    // 解析命令行
    [[nodiscard]] bool parse(std::wstring_view commandLine);

    // 获取解析结果
    [[nodiscard]] const ParserData& result() const noexcept { return m_commands; }
    [[nodiscard]] ParserData& result() noexcept { return m_commands; }

    // 检查是否包含指定命令
    [[nodiscard]] bool hasCommand(std::wstring_view cmd) const;

    // 获取指定命令的参数数量
    [[nodiscard]] size_t getParamCount(std::wstring_view cmd) const noexcept;

    // 安全获取指定命令的指定索引参数
    [[nodiscard]] std::optional<std::wstring> getParam(
        std::wstring_view cmd, size_t index) const noexcept;

    // 获取所有命令名
    [[nodiscard]] std::vector<std::wstring> getAllCommands() const;

    // 清空解析结果
    void clear() noexcept { m_commands.clear(); }

    // 检查token是否有效
    [[nodiscard]] static bool isTokenValid(std::wstring_view token) noexcept;

    // 检查是否包含配对的引号
    [[nodiscard]] static bool hasQuotation(std::wstring_view token) noexcept;

    // 移除token两端的引号
    [[nodiscard]] static std::wstring removeQuotation(std::wstring_view token) noexcept;

    // 检查命令行引号是否配对
    [[nodiscard]] static bool isQuotationMatched(std::wstring_view input) noexcept;

private:
    [[nodiscard]] bool isCommand(std::wstring_view token) const noexcept;
    [[nodiscard]] std::vector<std::wstring> tokenize(std::wstring_view input);
    [[nodiscard]] std::wstring normalizeCommand(std::wstring_view cmd) const noexcept;

private:
    ParserData m_commands;                // 解析结果存储
    bool m_caseInsensitive = false;       // 是否大小写不敏感
    bool m_parseSuccess = false;          // 解析是否成功
};