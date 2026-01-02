#pragma once
#include<iostream>
#include<Windows.h>
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
private:
	void redirect();
	bool hasConsole = false;
	FILE* stream = nullptr;
};

