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
    enum class ParseMode { None, Normal, NoFlag };
    explicit CmdParser(bool caseInsensitive = false)
        : m_caseInsensitive(caseInsensitive) {
    }

    // Parse the command line std::string
    [[nodiscard]] bool parse(std::wstring_view commandLine, ParseMode mode = ParseMode::Normal);

    // Get parsing results (const/non-const overloads)
    [[nodiscard]] const ParserData& result() const noexcept { return m_commands; }
    [[nodiscard]] ParserData& result() noexcept { return m_commands; }

    // Check if the specified command exists
    [[nodiscard]] bool hasCommand(std::wstring_view cmd) const;

    // Get the number of parameters for the specified command
    [[nodiscard]] size_t getParamCount(std::wstring_view cmd) const noexcept;

    // Safely get the parameter at the specified index for the given command
    [[nodiscard]] std::optional<std::wstring> getParam(
        std::wstring_view cmd, size_t index) const noexcept;

    // Safely get all parameters for the specified command
    std::vector<std::wstring> getParams(std::wstring_view cmd) const noexcept;

    // Get all parsed command names
    [[nodiscard]] std::vector<std::wstring> getAllCommands() const;

    // Clear all parsing results
    void clear() noexcept { m_commands.clear(); }

    // Check if a token is valid
    [[nodiscard]] static bool isTokenValid(std::wstring_view token) noexcept;

    // Check if the token is wrapped with paired quotation marks
    [[nodiscard]] static bool hasQuotation(std::wstring_view token) noexcept;

    // Remove quotation marks from both ends of the token
    [[nodiscard]] static std::wstring removeQuotation(std::wstring_view token) noexcept;

    // Check if all quotation marks in the command line are matched
    [[nodiscard]] static bool isQuotationMatched(std::wstring_view input) noexcept;

    // Tokenize the input command line std::string into segments
    [[nodiscard]] std::vector<std::wstring> tokenize(std::wstring_view input);

private:
    // Determine if the token is a valid command identifier
    [[nodiscard]] bool isCommand(std::wstring_view token) const noexcept;

    // Normalize command name (case-insensitive if enabled)
    [[nodiscard]] std::wstring normalizeCommand(std::wstring_view cmd) const noexcept;

private:
    ParserData m_commands;                // Storage for parsing results
    bool m_caseInsensitive = false;       // Whether command matching is case-insensitive
    bool m_parseSuccess = false;          // Flag indicating if parsing succeeded
};