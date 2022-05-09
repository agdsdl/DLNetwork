#pragma once
#include "platform.h"
#include <string>

namespace DLNetwork {

class INetAddress {
public:
	//INetAddress(sockaddr* addr4or6, bool isIP6 = false) {
	//	if (isIP6) {
	//		_addr6 = *(sockaddr_in6*)addr4or6;
	//	}
	//	else {
	//		_addr4 = *(sockaddr_in*)addr4or6;
	//	}
	//}
	INetAddress() {
		_addr4.sin_family = 0;
	}
	INetAddress(const sockaddr_in& addr4) {
		_addr4 = addr4;
	}
	INetAddress(const sockaddr_in6& addr6) {
		_addr6 = addr6;
	}

	~INetAddress() {}

	const std::string& description() {
		genDesc();
		return _desc;
	}
	const std::string& ip() {
		genDesc();
		return _ip;
	}
	int port() {
		genDesc();
		return _port;
	}

	bool isIP6() {
		return _addr6.sin6_family == AF_INET6;
	}
	bool isIP4() {
		return _addr4.sin_family == AF_INET;
	}
	sockaddr_in& addr4() {
		return _addr4;
	}
	sockaddr_in6& addr6() {
		return _addr6;
	}
	static INetAddress getPeerAddress(SOCKET s);
	static INetAddress getSelfAddress(SOCKET s);
	static INetAddress fromIp4Port(const char* ip, int port);
	static INetAddress fromIp6Port(const char* ip, int port);
	static INetAddress fromDomainPort(const char* domain, int port);

private:
	void genDesc();
	union
	{
		sockaddr_in _addr4;
		sockaddr_in6 _addr6;
	};
	std::string _domain;
	std::string _desc;
	std::string _ip;
	int _port;
};
} //DLNetwork