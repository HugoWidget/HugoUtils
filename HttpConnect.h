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
	string_t socketHttp(string_t host, string_t request)const;
	string_t postData(string_t host, string_t path, string_t post_content)const;
	string_t getData(string_t host, string_t path, string_t get_content)const;
private:
	short port = 80;
};
