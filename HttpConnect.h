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
	std::wstring socketHttp(std::wstring host, std::wstring request)const;
	std::wstring postData(std::wstring host, std::wstring path, std::wstring post_content)const;
	std::wstring getData(std::wstring host, std::wstring path, std::wstring get_content)const;
private:
	short port = 80;
};
