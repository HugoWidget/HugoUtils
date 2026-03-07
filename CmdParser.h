#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <stdexcept>
#include <cctype>
#include <string_view>

#include "WinUtilsDef.h"

using ParserData = std::map<WinUtils::string_t, std::vector<WinUtils::string_t>>;

class WinUtils::CmdParser {
public:
    enum class ParseMode { None, Normal, NoFlag };
    explicit CmdParser(bool caseInsensitive = false)
        : m_caseInsensitive(caseInsensitive) {
    }

    // Parse the command line std::string
    [[nodiscard]] bool parse(string_view_t commandLine, ParseMode mode = ParseMode::Normal);

    // Get parsing results (const/non-const overloads)
    [[nodiscard]] const ParserData& result() const noexcept { return m_commands; }
    [[nodiscard]] ParserData& result() noexcept { return m_commands; }

    // Check if the specified command exists
    [[nodiscard]] bool hasCommand(string_view_t cmd) const;

    // Get the number of parameters for the specified command
    [[nodiscard]] size_t getParamCount(string_view_t cmd) const noexcept;

    // Safely get the parameter at the specified index for the given command
    [[nodiscard]] std::optional<string_t> getParam(
        string_view_t cmd, size_t index) const noexcept;

    // Safely get all parameters for the specified command
    std::vector<string_t> getParams(string_view_t cmd) const noexcept;

    // Get all parsed command names
    [[nodiscard]] std::vector<string_t> getAllCommands() const;

    // Clear all parsing results
    void clear() noexcept { m_commands.clear(); }

    // Check if a token is valid
    [[nodiscard]] static bool isTokenValid(string_view_t token) noexcept;

    // Check if the token is wrapped with paired quotation marks
    [[nodiscard]] static bool hasQuotation(string_view_t token) noexcept;

    // Remove quotation marks from both ends of the token
    [[nodiscard]] static string_t removeQuotation(string_view_t token) noexcept;

    // Check if all quotation marks in the command line are matched
    [[nodiscard]] static bool isQuotationMatched(string_view_t input) noexcept;

    // Tokenize the input command line std::string into segments
    [[nodiscard]] std::vector<string_t> tokenize(string_view_t input);

private:
    // Determine if the token is a valid command identifier
    [[nodiscard]] bool isCommand(string_view_t token) const noexcept;

    // Normalize command name (case-insensitive if enabled)
    [[nodiscard]] string_t normalizeCommand(string_view_t cmd) const noexcept;

private:
    ParserData m_commands;                // Storage for parsing results
    bool m_caseInsensitive = false;       // Whether command matching is case-insensitive
    bool m_parseSuccess = false;          // Flag indicating if parsing succeeded
};