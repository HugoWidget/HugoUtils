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
 */
#pragma once

#include <iostream>
#include <string>
#include <map>
#include <functional>
#include <vector>

#include "Logger.h"
#include "CmdParser.h"

#ifndef WUAPI
#define WUAPI
#endif

namespace WinUtils {

    using Args = CmdParser;
    using CmdProc = std::function<void(class ConsoleMenu&, Args)>;

    struct CommandItem {
        string_t m_description;
        CmdProc m_handler;

        CommandItem(string_t desc, CmdProc func)
            : m_description(std::move(desc)), m_handler(std::move(func)) {
        }
    };

    class WUAPI MenuNode {
    public:
        MenuNode(string_t name, MenuNode* parent = nullptr);

        // disable copy, allow move
        MenuNode(const MenuNode&) = delete;
        MenuNode& operator=(const MenuNode&) = delete;
        MenuNode(MenuNode&&) = default;
        MenuNode& operator=(MenuNode&&) = default;

        MenuNode& addSubmenu(string_t submenuName, const string_t& description = TS(""));
        void addCommand(string_t cmdName, string_t description, CmdProc func = [](ConsoleMenu&, Args) {});
        string_t getFullPath() const;
        bool navigate(const std::vector<string_t>& segments, MenuNode*& currentNode, Args& args, bool silent);
        void showOptions() const;
        [[nodiscard]] MenuNode* getParent() const;
        [[nodiscard]] bool hasSubmenu(const string_t& name) const;
        [[nodiscard]] bool hasCommand(const string_t& name) const;

    private:
        friend class ConsoleMenu;

        int getMaxOptionLength(const std::vector<string_t>& options) const;
        MenuNode& setConsoleMenu(ConsoleMenu* menu);

        string_t m_name;
        string_t m_description;
        MenuNode* m_parent;
        std::map<string_t, MenuNode> m_submenus;
        std::map<string_t, CommandItem> m_commands;
        ConsoleMenu* m_menu = nullptr;
    };

    enum class MenuDisplayMode { Normal, Exclusive };

    class WUAPI ConsoleMenu {
    public:
        ConsoleMenu();

        void setDisplayMode(MenuDisplayMode mode);
        MenuNode& addSubmenu(string_t submenuName, const string_t& description = TS(""));
        MenuNode& addSubmenuAtPath(const string_t& parentPath, string_t submenuName, const string_t& description = TS(""));
        void addCommand(string_t cmdName, string_t description, CmdProc func);
        void addCommandAtPath(const string_t& menuPath, string_t cmdName, string_t description, CmdProc func);
        void addCommonCommand(string_t cmdName, string_t description, CmdProc func);
        void execute(string_t cmdPath, bool relative);
        string_t substituteVariables(const string_t& input) const;
        void run();

        // Access to variables (for commands)
        std::map<string_t, string_t>& getVariables() { return m_variables; }
        const std::map<string_t, string_t>& getVariables() const { return m_variables; }

    private:
        void refresh();

        MenuNode m_root;
        MenuNode* m_currentNode;
        std::map<string_t, CommandItem> m_commonCommands;
        MenuDisplayMode m_mode = MenuDisplayMode::Normal;
        std::map<string_t, string_t> m_variables;
    };

} // namespace WinUtils