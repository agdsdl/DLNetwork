#include "INetAddress.h"
#include "MyLog.h"
#include <memory>
#include <string.h>
#include "uv_errno.h"

using namespace DLNetwork;

INetAddress INetAddress::getPeerAddress(SOCKET s) {
	sockaddr_in6 myaddr;
	socklen_t len = sizeof(myaddr);
	if (0 != getpeername(s, (sockaddr*)&myaddr, &len)) {
		mCritical() << "getpeername error:" << strerror(errno);
	}
	if (len > sizeof(myaddr)) {
		mCritical() << "to small addr to getpeername";
	}

	if (myaddr.sin6_family == AF_INET6) {
		return INetAddress(myaddr);
	}
	else if (myaddr.sin6_family == AF_INET) {
		return INetAddress(*(sockaddr_in*)&myaddr);
	}
	else {
		return INetAddress();
	}
}

INetAddress INetAddress::getSelfAddress(SOCKET s) {
	sockaddr_in6 myaddr;
	socklen_t len = sizeof(myaddr);
	if (0 != getsockname(s, (sockaddr*)&myaddr, &len)) {
		mCritical() << "getsockname error:" << strerror(errno);
	}
	if (len > sizeof(myaddr)) {
		mCritical() << "to small addr to getsockname";
	}

	if (myaddr.sin6_family == AF_INET6) {
		return INetAddress(myaddr);
	}
	else if (myaddr.sin6_family == AF_INET) {
		return INetAddress(*(sockaddr_in*)&myaddr);
	}
	else {
		return INetAddress();
	}
}

INetAddress INetAddress::fromIp4Port(const char* ip, int port) {
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	if (::inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
		mCritical() << "INetAddress::fromIp4Port failed" << get_uv_errmsg();
		return INetAddress();
	}
	else {
		return INetAddress(addr);
	}
}

INetAddress INetAddress::fromIp6Port(const char* ip, int port) {
	sockaddr_in6 addr;
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(port);
	if (::inet_pton(AF_INET6, ip, &addr.sin6_addr) <= 0) {
		mCritical() << "INetAddress::fromIp6Port failed" << get_uv_errmsg();
		return INetAddress();
	}
	else {
		return INetAddress(addr);
	}
}

INetAddress INetAddress::fromDomainPort(const char* domain, int port) {
	INetAddress ret;
	struct addrinfo hints, * res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_INET;
	hints.ai_protocol = IPPROTO_TCP;
	int ecode = getaddrinfo(domain, std::to_string(port).c_str(), &hints, &res);
	if (ecode != 0) {
		const char* errMsg = gai_strerror(ecode);
		mCritical() << "INetAddress::fromDomainPort error:" << domain << errMsg;
		ret = INetAddress();
	}
	else {
		if (res->ai_family == AF_INET) {
			ret = INetAddress(*(sockaddr_in*)res->ai_addr);
		}
		else if (res->ai_family == AF_INET6) {
			ret = INetAddress(*(sockaddr_in6*)res->ai_addr);
		}
		else {
			ret = INetAddress();
		}
	}
	ret._domain = domain;
	return ret;
}

void INetAddress::genDesc() {
	if (_desc.length()) {
		return;
	}

	char buf[128] = { 0 }; //INET6_ADDRSTRLEN
	memset(buf, 0, sizeof(buf));
	if (_addr4.sin_family == AF_INET) {
		::inet_ntop(AF_INET, &_addr4.sin_addr, buf, sizeof(buf));
		_port = ntohs(_addr4.sin_port);
	}
	else if (_addr6.sin6_family == AF_INET6) {
		::inet_ntop(AF_INET6, &_addr6.sin6_addr, buf, sizeof(buf));
		_port = ntohs(_addr6.sin6_port);
	}
	else {
		memcpy(buf, "unspecify", sizeof("unspecify"));
		_port = 0;
	}
	_ip = buf;

	snprintf(buf + _ip.length(), sizeof(buf) - _ip.length(), ":%u", _port);
	_desc = buf;
	if (_domain.length()) {
		_desc += "|";
		_desc += _domain;
	}
}
