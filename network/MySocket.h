#pragma once
#include <memory>
#include <map>
#include <unordered_map>
#include <set>
#include "platform.h"
#include "INetAddress.h"
#include "StringUtil.h"
#include "MyLog.h"

namespace DLNetwork {

class MySocket {
	enum IPType {
		IPV4,
		IPV6
	};
	enum SockType {
	};
	enum ProtoType {
		TCP,
		UDP
	};
public:
	//MySocket(SOCKET sock) : _sock(sock) {}
	//MySocket(ProtoType type){}
	//int bind(const INetAddress& addr);
	//int bind(sockaddr_in addr);
	//int sendto(const INetAddress& addr);
	//int sendto(sockaddr_in addr);
	//SOCKET sock() { return _sock; }
	//~MySocket(){}

	static SOCKET create(ProtoType proto, IPType ipType=IPType::IPV4);
	static int sendto(SOCKET sock, INetAddress& addr, const char* data, size_t size)
	{
		mDebug() << "sendto" << addr.description() << hexmem(data, size);
		sockaddr_in addr4 = addr.addr4();
		int ret = ::sendto(sock, (const char*)data, size, 0, (sockaddr*)&addr4, sizeof(addr4));
		return ret;
	}

	static int bind(SOCKET sock, const INetAddress& addr)
	{
		int ret = ::bind(sock, (sockaddr*)&(addr.addr4()), sizeof(addr.addr4()));
		return ret;
	}
private:
	//SOCKET _sock;
};

} //DLNetwork