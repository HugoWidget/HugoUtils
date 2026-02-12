#include "ConsoleMenu.h"
#include "Logger.h"
#include <sstream>
#include <cstdlib>
#include <utility>
#include <conio.h>
#include <iomanip>
using namespace std;
using namespace WinUtils;
static Logger logger(L"ConsoleMenu");

// Clear the console screen
void clearScreen() {
#if defined(_WIN32) || defined(_WIN64)
	system("cls");
#else
	system("clear");
#endif
}

// Split wide-character path into segments
vector<wstring> splitPath(const wstring& path) {
	vector<wstring> segments;
	wstring segment;
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

// Read full input from console (until newline/EOF)
wstring readAll() {
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

// ========== MenuNode Implementation ==========
MenuNode::MenuNode(wstring name, MenuNode* parent)
	: m_name(move(name)), m_description(L""), m_parent(parent) {
}

MenuNode& MenuNode::addSubmenu(wstring submenuName, const wstring& description) {
	auto [it, created] = m_submenus.emplace(
		piecewise_construct,
		forward_as_tuple(move(submenuName)),
		forward_as_tuple(L"", this)
	);

	it->second.m_name = it->first;
	it->second.m_parent = this;
	it->second.m_description = description;
	it->second.setConsoleMenu(m_menu);
	return it->second;
}

void MenuNode::addCommand(wstring cmdName, wstring description, CmdProc func) {
	m_commands.emplace(move(cmdName), CommandItem(move(description), move(func)));
}

wstring MenuNode::getFullPath() const {
	wstring path;
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

bool MenuNode::navigate(const vector<wstring>& segments, MenuNode*& currentNode, Args& args) {
	MenuNode* tempNode = currentNode;
	for (const auto& seg : segments) {
		if (seg == L"..") {
			if (tempNode->m_parent != nullptr) {
				tempNode = tempNode->m_parent;
			}
			else {
				wcerr << L"Already at root menu!\n";
				(void)_getwch();
				return false;
			}
		}
		else if (tempNode->m_submenus.contains(seg)) {
			tempNode = &tempNode->m_submenus.at(seg);
		}
		else if (tempNode->m_commands.contains(seg)) {
			tempNode->m_commands.at(seg).m_handler(*m_menu, args);
			wcout << currentNode->getFullPath() << L">> " << L"Command executed successfully";
			(void)_getwch();
			m_menu->setDisplayMode(MenuDisplayMode::Normal);
			return true;
		}
		else if (int num = _wtoi(seg.substr(1).c_str())) {
			if (seg[0] == L'c') {
				if (num <= 0 || num > tempNode->m_commands.size()) {
					wcerr << L"Invalid numeric command index: " << seg << L"\n";
					(void)_getwch();
					return false;
				}
				auto it = tempNode->m_commands.begin();
				advance(it, num - 1);
				navigate({ it->first }, currentNode, args);
			}
			else if (seg[0] == L's') {
				if (num <= 0 || num > tempNode->m_submenus.size()) {
					wcerr << L"Invalid numeric submenu index: " << seg << L"\n";
					(void)_getwch();
					return false;
				}
				auto it = tempNode->m_submenus.begin();
				advance(it, num - 1);
				tempNode = &it->second;
			}
		}
		else {
			wcerr << L"Invalid path/command: " << seg << L"\n";
			(void)_getwch();
			return false;
		}
	}
	currentNode = tempNode;
	return true;
}

void MenuNode::showOptions() const {
	if (!m_submenus.empty()) {
		int index = 1;
		wcout << L"[SubMenu]\n";
		vector<wstring> submenuNames;
		for (auto const& submenu : m_submenus) {
			submenuNames.push_back(submenu.first);
		}
		for (const auto& [name, node] : m_submenus) {
			wstring desc = node.m_description.empty() ? (L"Enter " + name + L" Menu") : node.m_description;
			wcout << left << setw(4) << (to_wstring(index) + L".") << setw(getMaxOptionLength(submenuNames)) << name << L" - " << desc << L"\n";
			++index;
		}
	}

	if (!m_commands.empty()) {
		int index = 1;
		wcout << L"[Command]\n";
		vector <wstring> commandNames;
		for (auto const& cmd : m_commands) {
			commandNames.push_back(cmd.first);
		}
		for (const auto& [name, cmdItem] : m_commands) {
			wcout << left << setw(4) << (to_wstring(index) + L".") << setw(getMaxOptionLength(commandNames)) << name << L" - " << cmdItem.m_description << L"\n";
			++index;
		}
	}

	wcout << L"====================\n";
}

MenuNode* MenuNode::getParent() const {
	return m_parent;
}

bool MenuNode::hasSubmenu(const wstring& name) const {
	return m_submenus.contains(name);
}

bool MenuNode::hasCommand(const wstring& name) const {
	return m_commands.contains(name);
}


int WinUtils::MenuNode::getMaxOptionLength(vector<wstring> options) const
{
	int maxLength = 0;
	for (const auto& option : options) {
		int length = static_cast<int>(option.length());
		if (length > maxLength) {
			maxLength = length;
		}
	}
	return maxLength;
}

MenuNode& WinUtils::MenuNode::setConsoleMenu(ConsoleMenu* menu)
{
	m_menu = menu;
	return *this;
}

// ========== ConsoleMenu Implementation ==========
ConsoleMenu::ConsoleMenu() : m_root(L"Root", nullptr), m_currentNode(&m_root) {
	m_root.setConsoleMenu(this);
	addCommonCommand(L"common", L"Common Commands", [this](ConsoleMenu&, Args) {
		wcout << L".. - Navigate to parent menu\n";
		for (auto const& cmd : m_commonCommands) {
			wcout << cmd.first << L" - " << cmd.second.m_description << endl;
		}
		(void)_getwch();
		});
}

void WinUtils::ConsoleMenu::setDisplayMode(MenuDisplayMode mode)
{
	m_mode = mode;
	if (m_mode == MenuDisplayMode::Exclusive) {
		clearScreen();
	}
}

void ConsoleMenu::refresh() {
	clearScreen();
	wcout.flush();
	m_currentNode->showOptions();
}

MenuNode& ConsoleMenu::addSubmenu(wstring submenuName, const wstring& description) {
	return m_currentNode->addSubmenu(move(submenuName), description).setConsoleMenu(this);
}

MenuNode& ConsoleMenu::addSubmenuAtPath(const wstring& parentPath, wstring submenuName, const wstring& description) {
	MenuNode* parentNode = &m_root;
	CmdParser parser;
	auto segments = splitPath(parentPath);
	if (!parentNode->navigate(segments, parentNode, parser)) {
		logger.DLog(LogLevel::Info, format(L"Parent path does not exist: {}", parentPath));
		return *parentNode;
	}
	return parentNode->addSubmenu(move(submenuName), description).setConsoleMenu(this);
}

void ConsoleMenu::addCommand(wstring cmdName, wstring description, CmdProc func) {
	m_currentNode->addCommand(move(cmdName), move(description), move(func));
}

void ConsoleMenu::addCommandAtPath(const wstring& menuPath, wstring cmdName, wstring description, CmdProc func) {
	MenuNode* targetNode = &m_root;
	CmdParser parser;
	if (menuPath != L"") {
		auto segments = splitPath(menuPath);
		if (!targetNode->navigate(segments, targetNode, parser)) {
			logger.DLog(LogLevel::Info, format(L"Menu path does not exist: {}", menuPath));
			return;
		}
	}
	targetNode->addCommand(move(cmdName), move(description), move(func));
}

void ConsoleMenu::addCommonCommand(wstring cmdName, wstring description, CmdProc func) {
	m_commonCommands.emplace(move(cmdName), CommandItem(move(description), move(func)));
}

void ConsoleMenu::run() {
	wstring input;
	refresh();
	while (true) {
		wcout << m_currentNode->getFullPath() << L">> ";
		input = readAll();
		size_t pos = input.find(L' ');
		wstring strArgs;
		if (pos != wstring::npos)
			strArgs = input.substr(pos + 1);
		wstring strCmd = input.substr(0, pos);
		Args args;
		(void)args.parse(strArgs, CmdParser::ParseMode::None);
		// Execute common commands
		for (auto const& cmd : m_commonCommands) {
			if (input == cmd.first) {
				cmd.second.m_handler(*this, args);
				wcout << L'\n' << m_currentNode->getFullPath() << L">> " << L"Command executed successfully\n";
				(void)_getwch();
				goto nextLoop;
			}
		}

		if (!input.empty()) {
			auto segments = splitPath(strCmd);
			m_currentNode->navigate(segments, m_currentNode, args);
		}
		else continue;
	nextLoop:
		refresh();
	}
}