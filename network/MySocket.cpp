#include "MySocket.h"
#include "StringUtil.h"
#include <sstream>
#include <assert.h>

using namespace DLNetwork;

SOCKET DLNetwork::MySocket::create(ProtoType proto, IPType ipType)
{
    assert(ipType == IPType::IPV4);
    SOCKET sock;
    if (proto == ProtoType::TCP) {
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    }
    else {
        sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    }
    return sock;
}
