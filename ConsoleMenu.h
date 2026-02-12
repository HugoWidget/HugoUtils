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
		std::wstring m_description;       // Command description
		CmdProc m_handler;                // Command handler function

		CommandItem(std::wstring desc, CmdProc func)
			: m_description(std::move(desc)), m_handler(std::move(func)) {
		}
	};

	class MenuNode {
	private:
		std::wstring m_name;                // Menu name
		std::wstring m_description;         // Menu description
		MenuNode* m_parent;                 // Parent menu node
		std::map<std::wstring, MenuNode> m_submenus;  // Submenu collection
		std::map<std::wstring, CommandItem> m_commands;  // Node-specific commands
		ConsoleMenu* m_menu = nullptr;      // Associated console menu instance
	public:
		MenuNode(std::wstring name, MenuNode* parent = nullptr);

		// Prohibit copy, allow move semantics
		MenuNode(const MenuNode&) = delete;
		MenuNode& operator=(const MenuNode&) = delete;
		MenuNode(MenuNode&&) = default;
		MenuNode& operator=(MenuNode&&) = default;

		// Member functions
		MenuNode& addSubmenu(std::wstring submenuName, const std::wstring& description);
		void addCommand(std::wstring cmdName, std::wstring description, CmdProc func = [](ConsoleMenu&, Args){});
		std::wstring getFullPath() const;
		bool navigate(const std::vector<std::wstring>& segments, MenuNode*& currentNode, Args& args);
		void showOptions() const;
		[[nodiscard]] MenuNode* getParent() const;
		[[nodiscard]] bool hasSubmenu(const std::wstring& name) const;
		[[nodiscard]] bool hasCommand(const std::wstring& name) const;

	private:
		friend ConsoleMenu;
		int getMaxOptionLength(std::vector<std::wstring> options) const;
		MenuNode& setConsoleMenu(ConsoleMenu* menu);
	};

	enum class MenuDisplayMode { Normal, Exclusive };
	class ConsoleMenu {
	private:
		MenuNode m_root;                    // Root menu node
		MenuNode* m_currentNode;            // Current active menu node
		std::map<std::wstring, CommandItem> m_commonCommands;  // Global common commands
		MenuDisplayMode m_mode = MenuDisplayMode::Normal;  // Menu display mode
		void refresh();                     // Refresh console menu display

	public:
		ConsoleMenu();
		void setDisplayMode(MenuDisplayMode mode);
		MenuNode& addSubmenu(std::wstring submenuName, const std::wstring& description = L"");
		MenuNode& addSubmenuAtPath(const std::wstring& parentPath, std::wstring submenuName, const std::wstring& description = L"");
		void addCommand(std::wstring cmdName, std::wstring description, CmdProc func);
		void addCommandAtPath(const std::wstring& menuPath, std::wstring cmdName, std::wstring description, CmdProc func);
		void addCommonCommand(std::wstring cmdName, std::wstring description, CmdProc func);
		void run();                         // Start the console menu main loop
	};

}  // namespace WinUtils