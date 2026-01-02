#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <iostream>
#include <sstream> 
#include "WinUtilsDef.h"
class WinUtils::HttpConnect {
public:
	HttpConnect();
	~HttpConnect();
	void setPort(short port);
	std::wstring socketHttp(std::wstring host, std::wstring request);
	std::wstring postData(std::wstring host, std::wstring path, std::wstring post_content);
	std::wstring getData(std::wstring host, std::wstring path, std::wstring get_content);
private:
	short port = 80;
};
