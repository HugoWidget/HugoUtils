#include "ConsoleMenu.h"
#include "Logger.h"
#include <sstream>
#include <cstdlib>
#include <utility>
#include <conio.h>
using namespace std;
using namespace WinUtils;

// 清空控制台
void clearScreen() {
#if defined(_WIN32) || defined(_WIN64)
    system("cls");
#else
    system("clear");
#endif
}

// 分割宽字节路径
std::vector<std::wstring> splitPath(const std::wstring& path) {
    std::vector<std::wstring> segments;
    std::wstring segment;
    for (auto it = path.begin(); it != path.end(); it++) {
        if (*it == L'/') {
            segments.push_back(segment);
            segment = L"";
        }
        else {
            segment += *it;
        }
    }
    segments.push_back(segment);
    return segments;
}

// 读取控制台输入
std::wstring readAll() {
    wchar_t ch;
    wstring input;
    rewind(stdin);
    fflush(stdin);
    while ((ch = getwchar()) != '\n' && ch != EOF) {
        input += ch;
    }
    rewind(stdin);
    fflush(stdin);
    return input;
}

// ========== MenuNode 实现 ==========
MenuNode::MenuNode(std::wstring name, MenuNode* parent)
    : m_name(std::move(name)), m_description(L""), m_parent(parent) {
}

MenuNode& MenuNode::addSubmenu(std::wstring submenuName, const std::wstring& description) {
    auto [it, created] = m_submenus.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(std::move(submenuName)),
        std::forward_as_tuple(L"", this)
    );

    it->second.m_name = it->first;
    it->second.m_parent = this;
    it->second.m_description = description;

    return it->second;
}

void MenuNode::addCommand(std::wstring cmdName, std::wstring description, std::function<void()> func) {
    m_commands.emplace(std::move(cmdName), CommandItem(std::move(description), std::move(func)));
}

std::wstring MenuNode::getFullPath() const {
    std::wstring path;
    const MenuNode* current = this;
    while (current != nullptr) {
        if (!path.empty()) {
            path = L"/" + path;
        }
        path = current->m_name + path;
        current = current->m_parent;
    }
    return path.empty() ? L"/" : path;
}

bool MenuNode::navigate(const std::vector<std::wstring>& segments, MenuNode*& currentNode) {
    MenuNode* tempNode = currentNode;
    for (const auto& seg : segments) {
        if (seg == L"..") {
            if (tempNode->m_parent != nullptr) {
                tempNode = tempNode->m_parent;
            }
            else {
                std::wcerr << L"已处于根菜单，无法返回上一级！\n";
                (void)_getwch();
                return false;
            }
        }
        else if (tempNode->m_submenus.contains(seg)) {
            tempNode = &tempNode->m_submenus.at(seg);
        }
        else if (tempNode->m_commands.contains(seg)) {
            tempNode->m_commands.at(seg).m_handler();
            std::wcout << currentNode->getFullPath() << L">> " << L"命令执行完毕";
            (void)_getwch();
            return true;
        }
        else {
            std::wcerr << L"无效的路径/指令：" << seg << L"\n";
            (void)_getwch();
            return false;
        }
    }
    currentNode = tempNode;
    return true;
}

void MenuNode::showOptions() const {
    if (!m_submenus.empty()) {
        std::wcout << L"[SubMenu]\n";
        for (const auto& [name, node] : m_submenus) {
            std::wstring desc = node.m_description.empty() ? (L"Enter " + name + L" Menu") : node.m_description;
            std::wcout << L"  " << name << L" - " << desc << L"\n";
        }
    }

    if (!m_commands.empty()) {
        std::wcout << L"[Command]\n";
        for (const auto& [name, cmdItem] : m_commands) {
            // 替换pair的first为m_description
            std::wcout << L"  " << name << L" - " << cmdItem.m_description << L"\n";
        }
    }

    std::wcout << L"====================\n";
}

MenuNode* MenuNode::getParent() const {
    return m_parent;
}

bool MenuNode::hasSubmenu(const std::wstring& name) const {
    return m_submenus.contains(name);
}

bool MenuNode::hasCommand(const std::wstring& name) const {
    return m_commands.contains(name);
}

// ========== ConsoleMenu 实现 ==========
ConsoleMenu::ConsoleMenu() : m_root(L"Root", nullptr), m_currentNode(&m_root) {
    addCommonCommand(L"help", L"帮助", [this]() {
        wcout << L".. - 跳转到上级菜单\n";
        for (auto const& cmd : m_commonCommands) {
            wcout << cmd.first << L" - " << cmd.second.m_description << endl;
        }
        (void)_getwch();
        });
}

void ConsoleMenu::refresh() {
    clearScreen();
    std::wcout.flush();
    m_currentNode->showOptions();
}

MenuNode& ConsoleMenu::addSubmenu(std::wstring submenuName, const std::wstring& description) {
    return m_currentNode->addSubmenu(std::move(submenuName), description);
}

MenuNode& ConsoleMenu::addSubmenuAtPath(const std::wstring& parentPath, std::wstring submenuName, const std::wstring& description) {
    MenuNode* parentNode = &m_root;
    auto segments = splitPath(parentPath);
    if (!parentNode->navigate(segments, parentNode)) {
        WuDebug(LogLevel::Info, format(L"父路径不存在：{}", parentPath));
        return *parentNode;
    }
    return parentNode->addSubmenu(std::move(submenuName), description);
}

void ConsoleMenu::addCommand(std::wstring cmdName, std::wstring description, std::function<void()> func) {
    m_currentNode->addCommand(std::move(cmdName), std::move(description), std::move(func));
}

void ConsoleMenu::addCommandAtPath(const std::wstring& menuPath, std::wstring cmdName, std::wstring description, std::function<void()> func) {
    MenuNode* targetNode = &m_root;
    if (menuPath != L"") {
        auto segments = splitPath(menuPath);
        if (!targetNode->navigate(segments, targetNode)) {
            WuDebug(LogLevel::Info, format(L"菜单路径不存在：{}", menuPath));
            return;
        }
    }
    targetNode->addCommand(std::move(cmdName), std::move(description), std::move(func));
}

void ConsoleMenu::addCommonCommand(std::wstring cmdName, std::wstring description, std::function<void()> func) {
    m_commonCommands.emplace(std::move(cmdName), CommandItem(std::move(description), std::move(func)));
}

void ConsoleMenu::run() {
    std::wstring input;
    refresh();
    while (true) {
        std::wcout << m_currentNode->getFullPath() << L">> ";
        input = readAll();

        // 执行通用命令
        for (auto const& cmd : m_commonCommands) {
            if (input == cmd.first) {
                cmd.second.m_handler();
                std::wcout << m_currentNode->getFullPath() << L">> " << L"命令执行完毕\n";
                (void)_getwch();
                goto nextLoop;
            }
        }

        if (!input.empty()) {
            auto segments = splitPath(input);
            m_currentNode->navigate(segments, m_currentNode);
        }
        else continue;
    nextLoop:
        refresh();
    }
}