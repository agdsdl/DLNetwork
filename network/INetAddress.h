/*
 * MIT License
 *
 * Copyright (c) 2019-2022 agdsdl <agdsdl@sina.com.cn>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
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
	const sockaddr_in& addr4() const {
		return _addr4;
	}
	const sockaddr_in6& addr6() const {
		return _addr6;
	}
	static INetAddress getPeerAddress(SOCKET s);
	static INetAddress getSelfAddress(SOCKET s);
	static INetAddress fromIp4Port(const char* ip, int port);
	static INetAddress fromIp4PortInHost(uint32_t ip, int port);
	static INetAddress fromIp4PortInNet(uint32_t ip, int port);
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