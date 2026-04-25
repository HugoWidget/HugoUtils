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
#include "WinUtils/WinPch.h"

#include <cstring>
#include <cerrno>
#include <ws2tcpip.h>

#include "WinUtils/HttpConnect.h"
#include "WinUtils/StrConvert.h"
#include "WinUtils/Logger.h"
#pragma comment(lib,"ws2_32.lib")
using namespace WinUtils;
using namespace std;
static Logger logger(TS("HttpConnect"));
HttpConnect::HttpConnect()
{
	WSADATA wsa = { 0 };
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		logger.DLog(LogLevel::Error, format(TS("WSAStartup failed! Error: {}\n"), WSAGetLastError()));
	}
}

HttpConnect::~HttpConnect()
{
	WSACleanup();
}

void HttpConnect::setPort(short port)
{
	this->port = port;
}

string_t HttpConnect::socketHttp(string_t host, string_t request, int code_page)const
{
	SOCKET sockfd = -1;

	TF(ADDRINFO) hints, * servinfo = nullptr, * p = nullptr;
	int rv = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((rv = TF(GetAddrInfo)(host.c_str(), to_tstring(port).c_str(), &hints, &servinfo)) != 0) {
		logger.DLog(LogLevel::Error, format(TS("GetAddrInfo failed! Error:{} "), TF(gai_strerror)(rv)));
		return TS("");
	}
	for (p = servinfo; p != nullptr; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			logger.DLog(LogLevel::Error, format(TS("socket create failed! Error: {}"), WSAGetLastError()));
			continue;
		}
		if (connect(sockfd, p->ai_addr, (int)p->ai_addrlen) == -1) {
			closesocket(sockfd);
			logger.DLog(LogLevel::Error, format(TS("connection error! Error:{} "), WSAGetLastError()));
			continue;
		}

		break;
	}

	TF(FreeAddrInfo)(servinfo);
	if (p == nullptr) {
		logger.DLog(LogLevel::Error, format(TS("Failed to connect to {}:{}"), host, port));
		return TS("");
	}

	logger.DLog(LogLevel::Error, format(TS("{}"), request.c_str()));
	string temp_request = ConvertString<string>(request);
	const char* ansi_request = temp_request.c_str();
	send(sockfd, ansi_request, (int)request.length(), 0);
	ansi_request = 0;

	char buf[4096] = { 0 };
	int offset = 0;
	int rc;

	while ((rc = recv(sockfd, buf + offset, 1024, 0)) > 0)
	{
		offset += rc;
		if (offset >= sizeof(buf) - 1) {
			logger.DLog(LogLevel::Error, TS("Response buffer full, truncating data"));
			break;
		}
	}
#ifdef WU_NARROW_STRING
	string_t res = ConvertString<string_t>((string)buf);
#else
	string_t res = MultiByteToWide(buf, code_page);
#endif
	logger.DLog(LogLevel::Debug, TS("socketHttp response:\n") + res);
	closesocket(sockfd);
	buf[offset] = 0;
	return res;
}

string_t HttpConnect::postData(string_t host, string_t path, string_t post_content, int code_page)const
{
	stringstream_t stream;
	stream << TS("POST ") << path;
	stream << TS(" HTTP/1.0\r\n");
	stream << TS("Host: ") << host << TS("\r\n");
	stream << TS("User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3\r\n");
	stream << TS("Content-Type:application/x-www-form-urlencoded\r\n");
	stream << TS("Content-Length:") << post_content.length() << TS("\r\n");
	stream << TS("Connection:close\r\n\r\n");
	stream << post_content.c_str();
	string_t res = socketHttp(host, stream.str(), code_page);
	logger.DLog(LogLevel::Debug, TS("postData response:\n") + res);
	return res;
}

string_t HttpConnect::getData(string_t host, string_t path, string_t get_content, int code_page)const
{
	stringstream_t stream;
	stream << TS("GET ") << path << (get_content.length() ? TS("?") : TS("")) << get_content;
	stream << TS(" HTTP/1.0\r\n");
	stream << TS("Host: ") << host << TS("\r\n");
	stream << TS("User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3\r\n");
	stream << TS("Connection:close\r\n\r\n");

	return socketHttp(host, stream.str(), code_page);
}