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
#include <sstream>
#include <cstdlib>
#include <utility>
#include <iomanip>
#include <conio.h>
#include <cctype>

#include "WinUtils/ConsoleMenu.h"
using namespace std;
using namespace WinUtils;

static Logger logger(TS("ConsoleMenu"));

// Helper: clear console screen
static void clearScreen() {
#if defined(_WIN32) || defined(_WIN64)
	system("cls");
#else
	system("clear");
#endif
}

// Helper: check if a character can be part of a variable name
static bool isVarNameChar(char_t ch) {
	return (ch >= TS('a') && ch <= TS('z')) ||
		(ch >= TS('A') && ch <= TS('Z')) ||
		(ch >= TS('0') && ch <= TS('9')) ||
		ch == TS('_');
}

// Helper: split a path by '/'
static vector<string_t> splitPath(const string_t& path) {
	vector<string_t> segments;
	string_t segment;
	for (auto ch : path) {
		if (ch == TS('/')) {
			segments.push_back(segment);
			segment.clear();
		}
		else {
			segment += ch;
		}
	}
	segments.push_back(segment);
	return segments;
}

// Helper: read a whole line from stdin
static string_t readLine() {
	string_t input;
#if WU_WIDE_STRING
	wint_t ch;
	while ((ch = getwchar()) != L'\n' && ch != WEOF) {
		input += static_cast<wchar_t>(ch);
	}
#else
	int ch;
	while ((ch = getchar()) != '\n' && ch != EOF) {
		input += static_cast<char>(ch);
	}
#endif
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

bool MenuNode::navigate(const vector<string_t>& segments, MenuNode*& currentNode, Args& args, bool silent) {
	MenuNode* temp = currentNode;
	for (const auto& seg : segments) {
		if (seg.empty()) {
			if (!silent) {
				tcerr << TS("Invalid empty path component\n");
				(void)_getwch();
			}
			return false;
		}

		if (seg == TS("..")) {
			if (temp->m_parent) {
				temp = temp->m_parent;
			}
			else {
				if (!silent) {
					tcerr << TS("Already at root menu\n");
					(void)_getwch();
				}
				return false;
			}
		}
		else if (temp->m_submenus.count(seg)) {
			temp = &temp->m_submenus.at(seg);
		}
		else if (temp->m_commands.count(seg)) {
			temp->m_commands.at(seg).m_handler(*m_menu, args);
			if (!silent) {
				tcout << currentNode->getFullPath() << TS(">> Command executed successfully\n");
				(void)_getwch();
			}
			m_menu->setDisplayMode(MenuDisplayMode::Normal);
			return true;
		}
		else if (seg.size() >= 2 && (seg[0] == TS('c') || seg[0] == TS('s'))) {
			// Numeric shortcut: cN for command, sN for submenu
			int num = 0;
#if WU_WIDE_STRING
			num = _wtoi(seg.substr(1).c_str());
#else
			try {
				num = stoi(seg.substr(1));
			}
			catch (...) { num = 0; }
#endif
			if (num <= 0) {
				if (!silent) {
					tcerr << TS("Invalid numeric index: ") << seg << TS("\n");
					(void)_getwch();
				}
				return false;
			}
			if (seg[0] == TS('c')) {
				if (static_cast<size_t>(num) > temp->m_commands.size()) {
					if (!silent) {
						tcerr << TS("Command index out of range: ") << seg << TS("\n");
						(void)_getwch();
					}
					return false;
				}
				auto it = temp->m_commands.begin();
				advance(it, num - 1);
				// Recursively navigate to the command name
				if (!navigate({ it->first }, temp, args, silent))
					return false;
				currentNode = temp; // update after command execution
				return true;
			}
			else { // 's'
				if (static_cast<size_t>(num) > temp->m_submenus.size()) {
					if (!silent) {
						tcerr << TS("Submenu index out of range: ") << seg << TS("\n");
						(void)_getwch();
					}
					return false;
				}
				auto it = temp->m_submenus.begin();
				advance(it, num - 1);
				temp = &it->second;
			}
		}
		else {
			if (!silent) {
				tcerr << TS("Invalid path/command: ") << seg << TS("\n");
				(void)_getwch();
			}
			return false;
		}
	}
	currentNode = temp;
	return true;
}

void MenuNode::showOptions() const {
	// Submenus
	if (!m_submenus.empty()) {
		tcout << TS("[Submenus]\n");
		vector<string_t> names;
		for (const auto& pair : m_submenus) names.push_back(pair.first);
		int idx = 1;
		for (const auto& [name, node] : m_submenus) {
			string_t desc = node.m_description.empty() ? (TS("Enter ") + name + TS(" menu")) : node.m_description;
			tcout << left << setw(4) << (to_tstring(idx++) + TS("."))
				<< setw(getMaxOptionLength(names)) << name << TS(" - ") << desc << TS("\n");
		}
	}
	// Commands
	if (!m_commands.empty()) {
		tcout << TS("[Commands]\n");
		vector<string_t> names;
		for (const auto& pair : m_commands) names.push_back(pair.first);
		int idx = 1;
		for (const auto& [name, cmd] : m_commands) {
			tcout << left << setw(4) << (to_tstring(idx++) + TS("."))
				<< setw(getMaxOptionLength(names)) << name << TS(" - ") << cmd.m_description << TS("\n");
		}
	}
	tcout << TS("====================\n");
}

MenuNode* MenuNode::getParent() const {
	return m_parent;
}

bool MenuNode::hasSubmenu(const string_t& name) const {
	return m_submenus.count(name) > 0;
}

bool MenuNode::hasCommand(const string_t& name) const {
	return m_commands.count(name) > 0;
}

int MenuNode::getMaxOptionLength(const vector<string_t>& options) const {
	int maxLen = 0;
	for (const auto& opt : options) {
		int len = static_cast<int>(opt.length());
		if (len > maxLen) maxLen = len;
	}
	return maxLen;
}

MenuNode& MenuNode::setConsoleMenu(ConsoleMenu* menu) {
	m_menu = menu;
	return *this;
}

// ========== ConsoleMenu Implementation ==========

ConsoleMenu::ConsoleMenu() : m_root(TS("Root"), nullptr), m_currentNode(&m_root) {
	m_root.setConsoleMenu(this);

	// Helper to show all common commands
	addCommonCommand(TS("common"), TS("List all common commands"), [this](ConsoleMenu&, Args) {
		tcout << TS("Available common commands:\n");
		for (const auto& [name, cmd] : m_commonCommands) {
			tcout << TS("  ") << name << TS(" - ") << cmd.m_description << TS("\n");
		}
		});

	// var: set or display a variable
	addCommonCommand(TS("var"), TS("var <name> [value]  -- set or show variable"), [this](ConsoleMenu&, Args args) {
		auto name = args.getParam(TS(""), 0);
		auto value = args.getParam(TS(""), 1);
		if (!name) {
			tcout << TS("Usage: var <name> [value]\n");
			return;
		}
		if (value) {
			m_variables[*name] = *value;
			tcout << TS("Variable set: ") << *name << TS(" = ") << *value << TS("\n");
		}
		else {
			auto it = m_variables.find(*name);
			if (it != m_variables.end())
				tcout << TS("Variable ") << *name << TS(" = ") << it->second << TS("\n");
			else
				tcout << TS("Variable ") << *name << TS(" not defined\n");
		}

		});

	// set: show all variables
	addCommonCommand(TS("set"), TS("set  -- display all variables"), [this](ConsoleMenu&, Args) {
		if (m_variables.empty()) {
			tcout << TS("No variables defined.\n");
		}
		else {
			tcout << TS("Variables:\n");
			for (const auto& [name, value] : m_variables) {
				tcout << TS("  ") << name << TS(" = ") << value << TS("\n");
			}
		}

		});

	// unset: delete a variable
	addCommonCommand(TS("unset"), TS("unset <name>  -- delete a variable"), [this](ConsoleMenu&, Args args) {
		auto name = args.getParam(TS(""), 0);
		if (!name) {
			tcout << TS("Usage: unset <variable_name>\n");
		}
		else if (m_variables.erase(*name)) {
			tcout << TS("Variable removed: ") << *name << TS("\n");
		}
		else {
			tcout << TS("Variable not found: ") << *name << TS("\n");
		}

		});

	// echo: print text with variable substitution
	addCommonCommand(TS("echo"), TS("echo <text>  -- print text (variables are expanded)"), [this](ConsoleMenu&, Args args) {
		if (args.getParamCount(TS("")) == 0) {
			tcout << TS("\n");
		}
		else {
			string_t line;
			for (size_t i = 0; i < args.getParamCount(TS("")); ++i) {
				auto part = args.getParam(TS(""), i);
				if (part) line += *part;
				if (i + 1 < args.getParamCount(TS(""))) line += TS(' ');
			}
			// Variables are already substituted by the caller, but just in case:
			tcout << substituteVariables(line) << TS("\n");
		}

		});

	// len: length of a string variable
	addCommonCommand(TS("len"), TS("len <destVar> <srcVar>  -- store length of srcVar into destVar"), [this](ConsoleMenu&, Args args) {
		auto dest = args.getParam(TS(""), 0);
		auto src = args.getParam(TS(""), 1);
		if (!dest || !src) {
			tcout << TS("Usage: len <destVar> <srcVar>\n");

			return;
		}
		auto it = m_variables.find(*src);
		if (it == m_variables.end()) {
			tcout << TS("Error: variable ") << *src << TS(" not defined\n");
		}
		else {
			size_t len = it->second.size();
			m_variables[*dest] = to_tstring(len);
			tcout << TS("Variable ") << *dest << TS(" = ") << len << TS("\n");
		}

		});

	// substr: substring extraction
	addCommonCommand(TS("substr"), TS("substr <destVar> <srcVar> <start> [length]  -- extract substring"), [this](ConsoleMenu&, Args args) {
		auto dest = args.getParam(TS(""), 0);
		auto src = args.getParam(TS(""), 1);
		auto startOpt = args.getParam(TS(""), 2);
		if (!dest || !src || !startOpt) {
			tcout << TS("Usage: substr <destVar> <srcVar> <start> [length]\n");

			return;
		}
		auto srcIt = m_variables.find(*src);
		if (srcIt == m_variables.end()) {
			tcout << TS("Error: variable ") << *src << TS(" not defined\n");

			return;
		}
		const string_t& str = srcIt->second;
		int start = 0;
		try {
			start = stoi(*startOpt);
		}
		catch (...) {
			tcout << TS("Error: start index must be an integer\n");

			return;
		}
		if (start < 0) start = 0;
		if (static_cast<size_t>(start) >= str.size()) {
			m_variables[*dest] = TS("");
			tcout << TS("Warning: start index out of range, empty string assigned\n");

			return;
		}
		size_t len = string_t::npos;
		auto lenOpt = args.getParam(TS(""), 3);
		if (lenOpt) {
			try {
				int l = stoi(*lenOpt);
				if (l >= 0) len = static_cast<size_t>(l);
			}
			catch (...) {
				tcout << TS("Error: length must be a non-negative integer\n");

				return;
			}
		}
		string_t sub = str.substr(start, len);
		m_variables[*dest] = sub;
		tcout << TS("Variable ") << *dest << TS(" = ") << sub << TS("\n");

		});

	// Arithmetic helpers
	auto getNumber = [this](const string_t& arg, string_t& errorMsg) -> long long {
		auto it = m_variables.find(arg);
		if (it != m_variables.end()) {
			try {
				return stoll(it->second);
			}
			catch (...) {
				errorMsg = TS("Variable '") + arg + TS("' does not contain a valid integer");
				throw runtime_error("");
			}
		}
		else {
			try {
				return stoll(arg);
			}
			catch (...) {
				errorMsg = TS("'") + arg + TS("' is not a valid integer");
				throw runtime_error("");
			}
		}
		};

	// add: addition
	addCommonCommand(TS("add"), TS("add <destVar> <a> <b>  -- dest = a + b"), [this, getNumber](ConsoleMenu&, Args args) {
		auto dest = args.getParam(TS(""), 0);
		auto aStr = args.getParam(TS(""), 1);
		auto bStr = args.getParam(TS(""), 2);
		if (!dest || !aStr || !bStr) {
			tcout << TS("Usage: add <destVar> <num1> <num2>\n");

			return;
		}
		string_t err;
		try {
			long long a = getNumber(*aStr, err);
			long long b = getNumber(*bStr, err);
			long long res = a + b;
			m_variables[*dest] = to_tstring(res);
			tcout << TS("Variable ") << *dest << TS(" = ") << res << TS("\n");
		}
		catch (...) {
			tcout << TS("Error: ") << err << TS("\n");
		}

		});

	// sub: subtraction
	addCommonCommand(TS("sub"), TS("sub <destVar> <a> <b>  -- dest = a - b"), [this, getNumber](ConsoleMenu&, Args args) {
		auto dest = args.getParam(TS(""), 0);
		auto aStr = args.getParam(TS(""), 1);
		auto bStr = args.getParam(TS(""), 2);
		if (!dest || !aStr || !bStr) {
			tcout << TS("Usage: sub <destVar> <a> <b>\n");

			return;
		}
		string_t err;
		try {
			long long a = getNumber(*aStr, err);
			long long b = getNumber(*bStr, err);
			long long res = a - b;
			m_variables[*dest] = to_tstring(res);
			tcout << TS("Variable ") << *dest << TS(" = ") << res << TS("\n");
		}
		catch (...) {
			tcout << TS("Error: ") << err << TS("\n");
		}

		});

	// mul: multiplication
	addCommonCommand(TS("mul"), TS("mul <destVar> <a> <b>  -- dest = a * b"), [this, getNumber](ConsoleMenu&, Args args) {
		auto dest = args.getParam(TS(""), 0);
		auto aStr = args.getParam(TS(""), 1);
		auto bStr = args.getParam(TS(""), 2);
		if (!dest || !aStr || !bStr) {
			tcout << TS("Usage: mul <destVar> <a> <b>\n");

			return;
		}
		string_t err;
		try {
			long long a = getNumber(*aStr, err);
			long long b = getNumber(*bStr, err);
			long long res = a * b;
			m_variables[*dest] = to_tstring(res);
			tcout << TS("Variable ") << *dest << TS(" = ") << res << TS("\n");
		}
		catch (...) {
			tcout << TS("Error: ") << err << TS("\n");
		}

		});

	// div: integer division
	addCommonCommand(TS("div"), TS("div <destVar> <a> <b>  -- dest = a / b (integer division, b != 0)"), [this, getNumber](ConsoleMenu&, Args args) {
		auto dest = args.getParam(TS(""), 0);
		auto aStr = args.getParam(TS(""), 1);
		auto bStr = args.getParam(TS(""), 2);
		if (!dest || !aStr || !bStr) {
			tcout << TS("Usage: div <destVar> <a> <b>\n");

			return;
		}
		string_t err;
		try {
			long long a = getNumber(*aStr, err);
			long long b = getNumber(*bStr, err);
			if (b == 0) throw runtime_error("");
			long long res = a / b;
			m_variables[*dest] = to_tstring(res);
			tcout << TS("Variable ") << *dest << TS(" = ") << res << TS("\n");
		}
		catch (...) {
			if (err.empty()) err = TS("Division by zero");
			tcout << TS("Error: ") << err << TS("\n");
		}

		});

	// mod: modulus
	addCommonCommand(TS("mod"), TS("mod <destVar> <a> <b>  -- dest = a % b (b != 0)"), [this, getNumber](ConsoleMenu&, Args args) {
		auto dest = args.getParam(TS(""), 0);
		auto aStr = args.getParam(TS(""), 1);
		auto bStr = args.getParam(TS(""), 2);
		if (!dest || !aStr || !bStr) {
			tcout << TS("Usage: mod <destVar> <a> <b>\n");

			return;
		}
		string_t err;
		try {
			long long a = getNumber(*aStr, err);
			long long b = getNumber(*bStr, err);
			if (b == 0) throw runtime_error("");
			long long res = a % b;
			m_variables[*dest] = to_tstring(res);
			tcout << TS("Variable ") << *dest << TS(" = ") << res << TS("\n");
		}
		catch (...) {
			if (err.empty()) err = TS("Modulo by zero");
			tcout << TS("Error: ") << err << TS("\n");
		}

		});

	// toupper
	addCommonCommand(TS("toupper"), TS("toupper <destVar> <srcVar>  -- convert to uppercase"), [this](ConsoleMenu&, Args args) {
		auto dest = args.getParam(TS(""), 0);
		auto src = args.getParam(TS(""), 1);
		if (!dest || !src) {
			tcout << TS("Usage: toupper <destVar> <srcVar>\n");

			return;
		}
		auto it = m_variables.find(*src);
		if (it == m_variables.end()) {
			tcout << TS("Error: variable ") << *src << TS(" not defined\n");
		}
		else {
			string_t res = it->second;
			for (auto& ch : res) ch = toupper(ch);
			m_variables[*dest] = res;
			tcout << TS("Variable ") << *dest << TS(" = ") << res << TS("\n");
		}

		});

	// tolower
	addCommonCommand(TS("tolower"), TS("tolower <destVar> <srcVar>  -- convert to lowercase"), [this](ConsoleMenu&, Args args) {
		auto dest = args.getParam(TS(""), 0);
		auto src = args.getParam(TS(""), 1);
		if (!dest || !src) {
			tcout << TS("Usage: tolower <destVar> <srcVar>\n");

			return;
		}
		auto it = m_variables.find(*src);
		if (it == m_variables.end()) {
			tcout << TS("Error: variable ") << *src << TS(" not defined\n");
		}
		else {
			string_t res = it->second;
			for (auto& ch : res) ch = tolower(ch);
			m_variables[*dest] = res;
			tcout << TS("Variable ") << *dest << TS(" = ") << res << TS("\n");
		}

		});

	// trim
	addCommonCommand(TS("trim"), TS("trim <destVar> <srcVar>  -- remove leading/trailing whitespace"), [this](ConsoleMenu&, Args args) {
		auto dest = args.getParam(TS(""), 0);
		auto src = args.getParam(TS(""), 1);
		if (!dest || !src) {
			tcout << TS("Usage: trim <destVar> <srcVar>\n");

			return;
		}
		auto it = m_variables.find(*src);
		if (it == m_variables.end()) {
			tcout << TS("Error: variable ") << *src << TS(" not defined\n");
		}
		else {
			string_t s = it->second;
			size_t start = s.find_first_not_of(TS(" \t\n\r\f\v"));
			if (start == string_t::npos) {
				m_variables[*dest] = TS("");
			}
			else {
				size_t end = s.find_last_not_of(TS(" \t\n\r\f\v"));
				m_variables[*dest] = s.substr(start, end - start + 1);
			}
			tcout << TS("Variable ") << *dest << TS(" = ") << m_variables[*dest] << TS("\n");
		}

		});

	// replace
	addCommonCommand(TS("replace"), TS("replace <destVar> <srcVar> <old> <new>  -- replace all occurrences"), [this](ConsoleMenu&, Args args) {
		auto dest = args.getParam(TS(""), 0);
		auto src = args.getParam(TS(""), 1);
		auto oldStr = args.getParam(TS(""), 2);
		auto newStr = args.getParam(TS(""), 3);
		if (!dest || !src || !oldStr || !newStr) {
			tcout << TS("Usage: replace <destVar> <srcVar> <oldSubstr> <newSubstr>\n");

			return;
		}
		auto it = m_variables.find(*src);
		if (it == m_variables.end()) {
			tcout << TS("Error: variable ") << *src << TS(" not defined\n");
		}
		else {
			string_t str = it->second;
			string_t oldS = *oldStr;
			string_t newS = *newStr;
			size_t pos = 0;
			while ((pos = str.find(oldS, pos)) != string_t::npos) {
				str.replace(pos, oldS.size(), newS);
				pos += newS.size();
			}
			m_variables[*dest] = str;
			tcout << TS("Variable ") << *dest << TS(" = ") << str << TS("\n");
		}

		});

	// if: conditional execution of a command (simple)
	addCommonCommand(TS("if"), TS("if <var> <op> <value> then <command>  -- e.g., if x eq 5 then echo x is five"), [this](ConsoleMenu&, Args args) {
		// Minimal implementation: if var op value then cmd
		// Supports eq, ne, lt, gt, le, ge.
		if (args.getParamCount(TS("")) < 5 || args.getParam(TS(""), 3)->find(TS("then")) == string_t::npos) {
			tcout << TS("Usage: if <var> <eq|ne|lt|gt|le|ge> <value> then <command>\n");

			return;
		}
		string_t varName = *args.getParam(TS(""), 0);
		string_t op = *args.getParam(TS(""), 1);
		string_t rhs = *args.getParam(TS(""), 2);
		// skip "then"
		size_t thenIdx = 3;
		while (thenIdx < args.getParamCount(TS("")) && *args.getParam(TS(""), thenIdx) != TS("then")) ++thenIdx;
		if (thenIdx + 1 >= args.getParamCount(TS(""))) {
			tcout << TS("Missing command after 'then'\n");

			return;
		}
		// rebuild command string
		string_t command;
		for (size_t i = thenIdx + 1; i < args.getParamCount(TS("")); ++i) {
			if (i > thenIdx + 1) command += TS(' ');
			command += *args.getParam(TS(""), i);
		}

		// evaluate condition
		auto varIt = m_variables.find(varName);
		string_t lhsStr = (varIt != m_variables.end()) ? varIt->second : TS("");
		long long lhs = 0, rhsNum = 0;
		bool isNumeric = false;
		try {
			lhs = stoll(lhsStr);
			rhsNum = stoll(rhs);
			isNumeric = true;
		}
		catch (...) {}

		bool cond = false;
		if (isNumeric) {
			if (op == TS("eq")) cond = (lhs == rhsNum);
			else if (op == TS("ne")) cond = (lhs != rhsNum);
			else if (op == TS("lt")) cond = (lhs < rhsNum);
			else if (op == TS("gt")) cond = (lhs > rhsNum);
			else if (op == TS("le")) cond = (lhs <= rhsNum);
			else if (op == TS("ge")) cond = (lhs >= rhsNum);
			else cond = false;
		}
		else {
			// string comparison
			if (op == TS("eq")) cond = (lhsStr == rhs);
			else if (op == TS("ne")) cond = (lhsStr != rhs);
			else cond = false;
		}

		if (cond) {
			execute(command, true);
		}
		else {
			tcout << TS("Condition false, command skipped.\n");

		}
		});

	addCommonCommand(TS("pause"), TS("pause"), [this](ConsoleMenu&, Args args) {
		tcout << TS("Press any key to continue...\n");
		(void)_getwch();
		});
}

void ConsoleMenu::setDisplayMode(MenuDisplayMode mode) {
	m_mode = mode;
	if (m_mode == MenuDisplayMode::Exclusive) {
		clearScreen();
	}
}

void ConsoleMenu::refresh() {
	clearScreen();
	m_currentNode->showOptions();
}

MenuNode& ConsoleMenu::addSubmenu(string_t submenuName, const string_t& description) {
	return m_currentNode->addSubmenu(move(submenuName), description).setConsoleMenu(this);
}

MenuNode& ConsoleMenu::addSubmenuAtPath(const string_t& parentPath, string_t submenuName, const string_t& description) {
	MenuNode* target = &m_root;
	Args dummy;
	vector<string_t> segs = splitPath(parentPath);
	if (!target->navigate(segs, target, dummy, true)) {
		logger.DLog(LogLevel::Info, format(TS("Parent path does not exist: {}"), parentPath));
		return *target;
	}
	return target->addSubmenu(move(submenuName), description).setConsoleMenu(this);
}

void ConsoleMenu::addCommand(string_t cmdName, string_t description, CmdProc func) {
	m_currentNode->addCommand(move(cmdName), move(description), move(func));
}

void ConsoleMenu::addCommandAtPath(const string_t& menuPath, string_t cmdName, string_t description, CmdProc func) {
	MenuNode* target = &m_root;
	Args dummy;
	if (!menuPath.empty()) {
		vector<string_t> segs = splitPath(menuPath);
		if (!target->navigate(segs, target, dummy, true)) {
			logger.DLog(LogLevel::Info, format(TS("Menu path does not exist: {}"), menuPath));
			return;
		}
	}
	target->addCommand(move(cmdName), move(description), move(func));
}

void ConsoleMenu::addCommonCommand(string_t cmdName, string_t description, CmdProc func) {
	m_commonCommands.emplace(move(cmdName), CommandItem(move(description), move(func)));
}

void ConsoleMenu::execute(string_t input, bool relative) {
	input = substituteVariables(input);
	size_t pos = input.find(TS(' '));
	string_t argsStr = (pos != string_t::npos) ? input.substr(pos + 1) : TS("");
	string_t cmdName = input.substr(0, pos);
	Args args;
	args.parse(argsStr, CmdParser::ParseMode::NoFlag);

	// Check common commands first
	auto commonIt = m_commonCommands.find(cmdName);
	if (commonIt != m_commonCommands.end()) {
		commonIt->second.m_handler(*this, args);
		return;
	}

	// Then navigate as a path
	if (!cmdName.empty()) {
		vector<string_t> segs = splitPath(cmdName);
		MenuNode* start = relative ? m_currentNode : &m_root;
		start->navigate(segs, start, args, true);
	}
}

string_t ConsoleMenu::substituteVariables(const string_t& input) const {
	string_t result;
	size_t i = 0;
	while (i < input.length()) {
		if (input[i] == TS('$')) {
			if (i + 1 < input.length() && input[i + 1] == TS('{')) {
				// ${var} syntax
				size_t end = input.find(TS('}'), i + 2);
				if (end != string_t::npos) {
					string_t varName = input.substr(i + 2, end - i - 2);
					auto it = m_variables.find(varName);
					if (it != m_variables.end())
						result += it->second;
					else
						result += TS("${") + varName + TS("}");
					i = end + 1;
					continue;
				}
			}
			else if (i + 1 < input.length() && isVarNameChar(input[i + 1])) {
				// $var syntax
				size_t start = i + 1;
				size_t end = start;
				while (end < input.length() && isVarNameChar(input[end]))
					++end;
				string_t varName = input.substr(start, end - start);
				auto it = m_variables.find(varName);
				if (it != m_variables.end())
					result += it->second;
				else
					result += TS("$") + varName;
				i = end;
				continue;
			}
		}
		result += input[i];
		++i;
	}
	return result;
}

void ConsoleMenu::run() {
	refresh();
	while (true) {
		tcout << m_currentNode->getFullPath() << TS(">> ");
		string_t line = readLine();
		if (line.empty()) continue;

		// Variable substitution is done inside execute, but do it here as well for consistency
		line = substituteVariables(line);
		size_t pos = line.find(TS(' '));
		string_t argsStr = (pos != string_t::npos) ? line.substr(pos + 1) : TS("");
		string_t cmdName = line.substr(0, pos);
		Args args;
		args.parse(argsStr, CmdParser::ParseMode::NoFlag);

		// Check common commands
		auto commonIt = m_commonCommands.find(cmdName);
		if (commonIt != m_commonCommands.end()) {
			commonIt->second.m_handler(*this, args);
			tcout << TS("\nPress any key to continue...");
			(void)_getwch();
			refresh();
			continue;
		}

		// Otherwise navigate
		if (!cmdName.empty()) {
			vector<string_t> segs = splitPath(cmdName);
			m_currentNode->navigate(segs, m_currentNode, args, false);
		}
		refresh();
	}
}