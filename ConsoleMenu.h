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

	class MenuNode {
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
		void addCommand(string_t cmdName, string_t description, CmdProc func = [](ConsoleMenu&, Args){});
		string_t getFullPath() const;
		bool navigate(const std::vector<string_t>& segments, MenuNode*& currentNode, Args& args);
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
	class ConsoleMenu {
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
		void run();                         // Start the console menu main loop
	};

}  // namespace WinUtils