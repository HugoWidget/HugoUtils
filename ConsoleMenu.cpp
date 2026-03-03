#include "ConsoleMenu.h"
#include "Logger.h"
#include <sstream>
#include <cstdlib>
#include <utility>
#include <conio.h>
#include <iomanip>
using namespace std;
using namespace WinUtils;
static Logger logger(TS("ConsoleMenu"));

// Clear the console screen
void clearScreen() {
#if defined(_WIN32) || defined(_WIN64)
	system("cls");
#else
	system("clear");
#endif
}

// Split wide-character path into segments
vector<string_t> splitPath(const string_t& path) {
	vector<string_t> segments;
	string_t segment;
	for (auto it = path.begin(); it != path.end(); it++) {
		if (*it == TS('/')) {
			segments.push_back(segment);
			segment = TS("");
		}
		else {
			segment += *it;
		}
	}
	segments.push_back(segment);
	return segments;
}

// Read full input from console (until newline/EOF)
string_t readAll() {
	char_t ch = 0;
	string_t input;
	rewind(stdin);
	fflush(stdin);
#if USE_WIDE_STRING
	while ((ch = getwchar()) != L'\n' && ch != EOF) {
		input += ch;
	}
#else
	while ((ch = getchar()) != '\n' && ch != EOF) {
		input += ch;
	}
#endif
	rewind(stdin);
	fflush(stdin);
	return input;
}

// ========== MenuNode Implementation ==========
MenuNode::MenuNode(string_t name, MenuNode* parent)
	: m_name(move(name)), m_description(TS("")), m_parent(parent) {
}

MenuNode& MenuNode::addSubmenu(string_t submenuName, const string_t& description) {
	auto [it, created] = m_submenus.emplace(
		piecewise_construct,
		forward_as_tuple(move(submenuName)),
		forward_as_tuple(TS(""), this)
	);

	it->second.m_name = it->first;
	it->second.m_parent = this;
	it->second.m_description = description;
	it->second.setConsoleMenu(m_menu);
	return it->second;
}

void MenuNode::addCommand(string_t cmdName, string_t description, CmdProc func) {
	m_commands.emplace(move(cmdName), CommandItem(move(description), move(func)));
}

string_t MenuNode::getFullPath() const {
	string_t path;
	const MenuNode* current = this;
	while (current != nullptr) {
		if (!path.empty()) {
			path = TS('/') + path;
		}
		path = current->m_name + path;
		current = current->m_parent;
	}
	return path.empty() ? TS("/") : path;
}

bool MenuNode::navigate(const vector<string_t>& segments, MenuNode*& currentNode, Args& args) {
	MenuNode* tempNode = currentNode;
	for (const auto& seg : segments) {
		if (seg == TS("..")) {
			if (tempNode->m_parent != nullptr) {
				tempNode = tempNode->m_parent;
			}
			else {
				wcerr << TS("Already at root menu!\n");
				(void)_getwch();
				return false;
			}
		}
		else if (tempNode->m_submenus.contains(seg)) {
			tempNode = &tempNode->m_submenus.at(seg);
		}
		else if (tempNode->m_commands.contains(seg)) {
			tempNode->m_commands.at(seg).m_handler(*m_menu, args);
			tcout << currentNode->getFullPath() << TS(">> ") << TS("Command executed successfully");
			(void)_getwch();
			m_menu->setDisplayMode(MenuDisplayMode::Normal);
			return true;
		}
#if USE_WIDE_STRING
		else if (int num = _wtoi(seg.substr(1).c_str())) {
#else
		else if (int num = stoi(seg.substr(1).c_str())) {
#endif
			if (seg[0] == TS('c')) {
				if (num <= 0 || num > tempNode->m_commands.size()) {
					tcerr << TS("Invalid numeric command index: ") << seg << TS("\n");
					(void)_getwch();
					return false;
				}
				auto it = tempNode->m_commands.begin();
				advance(it, num - 1);
				navigate({ it->first }, currentNode, args);
			}
			else if (seg[0] == TS('s')) {
				if (num <= 0 || num > tempNode->m_submenus.size()) {
					tcerr << TS("Invalid numeric submenu index: ") << seg << TS("\n");
					(void)_getwch();
					return false;
				}
				auto it = tempNode->m_submenus.begin();
				advance(it, num - 1);
				tempNode = &it->second;
			}
		}
		else {
			tcerr << TS("Invalid path/command: ") << seg << TS("\n");
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
		tcout << TS("[SubMenu]\n");
		vector<string_t> submenuNames;
		for (auto const& submenu : m_submenus) {
			submenuNames.push_back(submenu.first);
		}
		for (const auto& [name, node] : m_submenus) {
			string_t desc = node.m_description.empty() ? (TS("Enter ") + name + TS(" Menu")) : node.m_description;
			tcout << left << setw(4) << (to_tstring(index) + TS(".")) << setw(getMaxOptionLength(submenuNames)) << name << TS(" - ") << desc << TS("\n");
			++index;
		}
	}

	if (!m_commands.empty()) {
		int index = 1;
		tcout << TS("[Command]\n");
		vector <string_t> commandNames;
		for (auto const& cmd : m_commands) {
			commandNames.push_back(cmd.first);
		}
		for (const auto& [name, cmdItem] : m_commands) {
			tcout << left << setw(4) << (to_tstring(index) + TS(".")) << setw(getMaxOptionLength(commandNames)) << name << TS(" - ") << cmdItem.m_description << TS("\n");
			++index;
		}
	}

	wcout << L"====================\n";
}

MenuNode* MenuNode::getParent() const {
	return m_parent;
}

bool MenuNode::hasSubmenu(const string_t & name) const {
	return m_submenus.contains(name);
}

bool MenuNode::hasCommand(const string_t & name) const {
	return m_commands.contains(name);
}


int WinUtils::MenuNode::getMaxOptionLength(vector<string_t> options) const
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

MenuNode& WinUtils::MenuNode::setConsoleMenu(ConsoleMenu * menu)
{
	m_menu = menu;
	return *this;
}

// ========== ConsoleMenu Implementation ==========
ConsoleMenu::ConsoleMenu() : m_root(TS("Root"), nullptr), m_currentNode(&m_root) {
	m_root.setConsoleMenu(this);
	addCommonCommand(TS("common"), TS("Common Commands"), [this](ConsoleMenu&, Args) {
		tcout << TS(".. - Navigate to parent menu\n");
		for (auto const& cmd : m_commonCommands) {
			tcout << cmd.first << TS(" - ") << cmd.second.m_description << endl;
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

MenuNode& ConsoleMenu::addSubmenu(string_t submenuName, const string_t & description) {
	return m_currentNode->addSubmenu(move(submenuName), description).setConsoleMenu(this);
}

MenuNode& ConsoleMenu::addSubmenuAtPath(const string_t & parentPath, string_t submenuName, const string_t & description) {
	MenuNode* parentNode = &m_root;
	CmdParser parser;
	auto segments = splitPath(parentPath);
	if (!parentNode->navigate(segments, parentNode, parser)) {
		logger.DLog(LogLevel::Info, format(TS("Parent path does not exist: {}"), parentPath));
		return *parentNode;
	}
	return parentNode->addSubmenu(move(submenuName), description).setConsoleMenu(this);
}

void ConsoleMenu::addCommand(string_t cmdName, string_t description, CmdProc func) {
	m_currentNode->addCommand(move(cmdName), move(description), move(func));
}

void ConsoleMenu::addCommandAtPath(const string_t & menuPath, string_t cmdName, string_t description, CmdProc func) {
	MenuNode* targetNode = &m_root;
	CmdParser parser;
	if (menuPath != TS("")) {
		auto segments = splitPath(menuPath);
		if (!targetNode->navigate(segments, targetNode, parser)) {
			logger.DLog(LogLevel::Info, format(TS("Menu path does not exist: {}"), menuPath));
			return;
		}
	}
	targetNode->addCommand(move(cmdName), move(description), move(func));
}

void ConsoleMenu::addCommonCommand(string_t cmdName, string_t description, CmdProc func) {
	m_commonCommands.emplace(move(cmdName), CommandItem(move(description), move(func)));
}

void ConsoleMenu::run() {
	string_t input;
	refresh();
	while (true) {
		tcout << m_currentNode->getFullPath() << TS(">> ");
		input = readAll();
		size_t pos = input.find(TS(' '));
		string_t strArgs;
		if (pos != string_t::npos)
			strArgs = input.substr(pos + 1);
		string_t strCmd = input.substr(0, pos);
		Args args;
		(void)args.parse(strArgs, CmdParser::ParseMode::None);
		// Execute common commands
		for (auto const& cmd : m_commonCommands) {
			if (input == cmd.first) {
				cmd.second.m_handler(*this, args);
				tcout << TS('\n') << m_currentNode->getFullPath() << TS(">> ") << TS("Command executed successfully\n");
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