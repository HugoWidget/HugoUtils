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
#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <string_view>

#include "WinUtilsDef.h"

 // Convenience alias for the parsed data: map command name -> list of parameters
using ParserData = std::map<WinUtils::string_t, std::vector<WinUtils::string_t>>;

namespace WinUtils {

    /**
     * @brief A flexible command‑line parser that supports multiple prefixes,
     *        quoted arguments, and the `--option=value` syntax.
     *
     * The parser recognises commands starting with:
     *   - `-`   (short options, e.g. `-v`)
     *   - `--`  (long options, e.g. `--verbose`)
     *   - `/`   (Windows style, e.g. `/help`)
     *
     * A command may be immediately followed by an equals sign and a value:
     *   `--output=file.txt`   -> command "output" with one parameter "file.txt"
     *
     * Quoted strings are preserved and the quotes are removed from stored values.
     * The parser can operate in two modes:
     *   - Normal   : tokens that start with a prefix are commands, all others are parameters.
     *   - NoFlag   : the whole input is treated as a single command with no prefix.
     *   - None     : auto‑detects based on the presence of any prefix character.
     *
     * Results are stored as a map from the normalized command name to a vector of parameters.
     * The anonymous command (used in NoFlag mode) is stored under the empty string key.
     */
    class WUAPI CmdParser {
    public:
        enum class ParseMode { None, Normal, NoFlag };

        /**
         * @brief Construct a parser.
         * @param caseInsensitive If true, command names are matched case‑insensitively.
         */
        explicit CmdParser(bool caseInsensitive = false)
            : m_caseInsensitive(caseInsensitive) {
        }

        /**
         * @brief Parse a command line.
         * @param commandLine The input string to parse.
         * @param mode        Parsing mode (auto‑detect, normal, or no‑flag).
         * @return true on success, false on syntax error (e.g. unmatched quotes).
         */
        [[nodiscard]] bool parse(string_view_t commandLine, ParseMode mode = ParseMode::Normal);

        /// Returns the parsed data (read‑only).
        [[nodiscard]] const ParserData& result() const noexcept { return m_commands; }

        /// Returns the parsed data (modifiable).
        [[nodiscard]] ParserData& result() noexcept { return m_commands; }

        /// Check if a command (with any allowed prefix) exists.
        [[nodiscard]] bool hasCommand(string_view_t cmd) const;

        /// Return the number of parameters for the given command.
        [[nodiscard]] size_t getParamCount(string_view_t cmd) const noexcept;

        /// Safely retrieve a parameter at index for the given command.
        [[nodiscard]] std::optional<string_t> getParam(string_view_t cmd, size_t index) const noexcept;

        /// Retrieve all parameters for the given command.
        std::vector<string_t> getParams(string_view_t cmd) const noexcept;

        /// Return a list of all command names that were parsed.
        [[nodiscard]] std::vector<string_t> getAllCommands() const;

        /// Clear all parsed results.
        void clear() noexcept { m_commands.clear(); }

        // ----- Static helper functions -----
        [[nodiscard]] static bool isTokenValid(string_view_t token) noexcept;
        [[nodiscard]] static bool hasQuotation(string_view_t token) noexcept;
        [[nodiscard]] static string_t removeQuotation(string_view_t token) noexcept;
        [[nodiscard]] static bool isQuotationMatched(string_view_t input) noexcept;
        [[nodiscard]] std::vector<string_t> tokenize(string_view_t input);

    private:
        // ----- Internal helpers -----
        [[nodiscard]] bool isCommand(string_view_t token) const noexcept;
        [[nodiscard]] string_t normalizeCommand(string_view_t cmd) const noexcept;
        [[nodiscard]] static size_t getPrefixLength(string_view_t token) noexcept;

        // ----- Data members -----
        ParserData m_commands;            // Parsed results: command → parameters
        bool m_caseInsensitive = false;   // Flag for case‑insensitive matching
        bool m_parseSuccess = false;      // Indicates if the last parse succeeded
    };

} // namespace WinUtils