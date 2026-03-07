#pragma once
#include<Windows.h>

#include<iostream>

#include"WinUtilsDef.h"
class WinUtils::Console
{
public:
	bool attach();
	bool alloc();
	bool require();
	bool isUnderConsole();
	bool isOwnConsole();
	void setLocale();
	bool setVisible(bool visible);
	bool setVirtualConsole(bool enable);
private:
	void redirect();
	bool hasConsole = false;
	FILE* stream = nullptr;
};

