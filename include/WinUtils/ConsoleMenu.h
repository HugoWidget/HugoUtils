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
#include <iostream>
#include <string>
#include <map>
#include <functional>
#include <vector>

#include "Logger.h"
#include "CmdParser.h"

namespace WinUtils {
	using Args = CmdParser;
	using CmdProc = std::function<void(ConsoleMenu&, Args)>;
	struct CommandItem {
		string_t m_description;       // Command description
		CmdProc m_handler;                // Command handler function

		CommandItem(string_t desc, CmdProc func)
			: m_description(std::move(desc)), m_handler(std::move(func)) {
		}
	};

	class WUAPI MenuNode {
	private:
		string_t m_name;                // Menu name
		string_t m_description;         // Menu description
		MenuNode* m_parent;                 // Parent menu node
		std::map<string_t, MenuNode> m_submenus;  // Submenu collection
		std::map<string_t, CommandItem> m_commands;  // Node-specific commands
		ConsoleMenu* m_menu = nullptr;      // Associated console menu instance
	public:
		MenuNode(string_t name, MenuNode* parent = nullptr);

		// Prohibit copy, allow move semantics
		MenuNode(const MenuNode&) = delete;
		MenuNode& operator=(const MenuNode&) = delete;
		MenuNode(MenuNode&&) = default;
		MenuNode& operator=(MenuNode&&) = default;

		// Member functions
		MenuNode& addSubmenu(string_t submenuName, const string_t& description);
		void addCommand(string_t cmdName, string_t description, CmdProc func = [](ConsoleMenu&, Args) {});
		string_t getFullPath() const;
		bool navigate(const std::vector<string_t>& segments, MenuNode*& currentNode, Args& args,bool silent);
		void showOptions() const;
		[[nodiscard]] MenuNode* getParent() const;
		[[nodiscard]] bool hasSubmenu(const string_t& name) const;
		[[nodiscard]] bool hasCommand(const string_t& name) const;

	private:
		friend ConsoleMenu;
		int getMaxOptionLength(std::vector<string_t> options) const;
		MenuNode& setConsoleMenu(ConsoleMenu* menu);
	};

	enum class MenuDisplayMode { Normal, Exclusive };
	class WUAPI ConsoleMenu {
	private:
		MenuNode m_root;                    // Root menu node
		MenuNode* m_currentNode;            // Current active menu node
		std::map<string_t, CommandItem> m_commonCommands;  // Global common commands
		MenuDisplayMode m_mode = MenuDisplayMode::Normal;  // Menu display mode
		void refresh();                     // Refresh console menu display

	public:
		ConsoleMenu();
		void setDisplayMode(MenuDisplayMode mode);
		MenuNode& addSubmenu(string_t submenuName, const string_t& description = TS(""));
		MenuNode& addSubmenuAtPath(const string_t& parentPath, string_t submenuName, const string_t& description = TS(""));
		void addCommand(string_t cmdName, string_t description, CmdProc func);
		void addCommandAtPath(const string_t& menuPath, string_t cmdName, string_t description, CmdProc func);
		void addCommonCommand(string_t cmdName, string_t description, CmdProc func);
		void excute(string_t cmdPath, bool relative);
		void run();                         // Start the console menu main loop
	};

}  // namespace WinUtils