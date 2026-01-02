#pragma once
#include <iostream>
#include <string>
#include <map>
#include <functional>
#include <vector>
#include "Logger.h"

namespace WinUtils {

	struct CommandItem {
		std::wstring m_description;       // 命令描述
		std::function<void()> m_handler;  // 命令处理函数

		CommandItem(std::wstring desc, std::function<void()> func)
			: m_description(std::move(desc)), m_handler(std::move(func)) {
		}
	};

	class MenuNode {
	private:
		std::wstring m_name;                // 菜单名称
		std::wstring m_description;         // 菜单说明
		MenuNode* m_parent;                 // 父节点
		std::map<std::wstring, MenuNode> m_submenus;  // 子菜单
		std::map<std::wstring, CommandItem> m_commands;  // 通用命令

	public:
		MenuNode(std::wstring name, MenuNode* parent = nullptr);

		// 禁止拷贝，允许移动
		MenuNode(const MenuNode&) = delete;
		MenuNode& operator=(const MenuNode&) = delete;
		MenuNode(MenuNode&&) = default;
		MenuNode& operator=(MenuNode&&) = default;

		// 成员函数
		MenuNode& addSubmenu(std::wstring submenuName, const std::wstring& description);
		void addCommand(std::wstring cmdName, std::wstring description, std::function<void()> func);
		std::wstring getFullPath() const;
		bool navigate(const std::vector<std::wstring>& segments, MenuNode*& currentNode);
		void showOptions() const;
		[[nodiscard]] MenuNode* getParent() const;
		[[nodiscard]] bool hasSubmenu(const std::wstring& name) const;
		[[nodiscard]] bool hasCommand(const std::wstring& name) const;
	};

	class ConsoleMenu {
	private:
		MenuNode m_root;
		MenuNode* m_currentNode;
		std::map<std::wstring, CommandItem> m_commonCommands;

		void refresh();

	public:
		ConsoleMenu();

		MenuNode& addSubmenu(std::wstring submenuName, const std::wstring& description = L"");
		MenuNode& addSubmenuAtPath(const std::wstring& parentPath, std::wstring submenuName, const std::wstring& description = L"");
		void addCommand(std::wstring cmdName, std::wstring description, std::function<void()> func);
		void addCommandAtPath(const std::wstring& menuPath, std::wstring cmdName, std::wstring description, std::function<void()> func);
		void addCommonCommand(std::wstring cmdName, std::wstring description, std::function<void()> func);
		void run();
	};

}  // namespace WinUtils