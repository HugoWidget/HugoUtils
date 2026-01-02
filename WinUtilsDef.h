#pragma once
#ifdef _DEBUG 
#define WINUTILS_DEBUG 1
#else 
#define WINUTILS_DEBUG 0
#endif
namespace WinUtils {
	class CmdParser;
	class Console;
	class HttpConnect;
	class Injector;
	class WinSvcMgr;
	class Logger;
	class ConsoleMenu;
	struct CommandItem;
	class MenuNode;
}