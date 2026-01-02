#include "HttpConnect.h"
#include "StrConvert.h"
#include "Logger.h"
#include <cstring>
#include <cerrno>
#pragma comment(lib,"ws2_32.lib")
using namespace WinUtils;
using namespace std;

HttpConnect::HttpConnect()
{
	WSADATA wsa = { 0 };
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
		WuDebug(LogLevel::Error, format(L"WSAStartup failed! Error: {}\n", WSAGetLastError()));
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

std::wstring HttpConnect::socketHttp(std::wstring host, std::wstring request)
{
	SOCKET sockfd = -1;
	ADDRINFOW hints, * servinfo = nullptr, * p = nullptr;
	int rv = 0;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((rv = GetAddrInfo(host.c_str(), to_wstring(port).c_str(), &hints, &servinfo)) != 0) {
		WuDebug(LogLevel::Error, format(L"GetAddrInfo failed! Error:{} ", gai_strerrorW(rv)));
		return L"";
	}
	for (p = servinfo; p != nullptr; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			WuDebug(LogLevel::Error, format(L"socket create failed! Error: {}", WSAGetLastError()));
			continue;
		}
		if (connect(sockfd, p->ai_addr, (int)p->ai_addrlen) == -1) {
			closesocket(sockfd);
			WuDebug(LogLevel::Error, format(L"connection error! Error:{} ", WSAGetLastError()));
			continue;
		}

		break;
	}

	FreeAddrInfo(servinfo);
	if (p == nullptr) {
		WuDebug(LogLevel::Error, format(L"Failed to connect to {}:{}", host, port));
		return L"";
	}

	// 发送HTTP请求
	WuDebug(LogLevel::Error, format(L"{}", request.c_str()));
	char* ansi_request = WideStringToAnsi(request);
	send(sockfd, ansi_request, (int)request.length(), 0);
	delete ansi_request;
	ansi_request = 0;
	// 接收响应
	char buf[4096] = { 0 };
	int offset = 0;
	int rc;

	while ((rc = recv(sockfd, buf + offset, 1024, 0)) > 0)
	{
		offset += rc;
		if (offset >= sizeof(buf) - 1) {
			WuDebug(LogLevel::Error, L"Response buffer full, truncating data");
			break;
		}
	}

	closesocket(sockfd);
	buf[offset] = 0;
	return AnsiToWideString(buf);
}

std::wstring HttpConnect::postData(std::wstring host, std::wstring path, std::wstring post_content)
{
	wstringstream stream;
	stream << L"POST " << path;
	stream << L" HTTP/1.0\r\n";
	stream << L"Host: " << host << "\r\n";
	stream << L"User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3\r\n";
	stream << L"Content-Type:application/x-www-form-urlencoded\r\n";
	stream << L"Content-Length:" << post_content.length() << "\r\n";
	stream << L"Connection:close\r\n\r\n";
	stream << post_content.c_str();

	return socketHttp(host, stream.str());
}

std::wstring HttpConnect::getData(std::wstring host, std::wstring path, std::wstring get_content)
{
	wstringstream stream;
	stream << L"GET " << path << (get_content.length() ? L"?" : L"") << get_content;
	stream << L" HTTP/1.0\r\n";
	stream << L"Host: " << host << "\r\n";
	stream << L"User-Agent: Mozilla/5.0 (Windows; U; Windows NT 5.1; zh-CN; rv:1.9.2.3) Gecko/20100401 Firefox/3.6.3\r\n";
	stream << L"Connection:close\r\n\r\n";

	return socketHttp(host, stream.str());
}