#include "UdpServer2.h"
#include "sockutil.h"
#include "EventThread.h"
#include "MyLog.h"
#include "uv_errno.h"

using namespace DLNetwork;

UdpServer2::UdpServer2()
{
}

UdpServer2::~UdpServer2()
{
    stop();
}

bool UdpServer2::start(EventThread* loop, INetAddress listenAddr, std::string name, bool reusePort)
{
    _thread = loop;
    _listenAddr = std::move(listenAddr);
    _name = std::move(name);
    _reusePort = reusePort;

    _listenSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_listenSock == SOCKET_ERROR) {
        mCritical() << "UdpServer create socket error" << get_uv_errmsg();
        return false;
    }
    SockUtil::setNoBlocked(_listenSock, true);
    SockUtil::setReuseable(_listenSock, _reusePort);
    int ret = bind(_listenSock, (sockaddr*)&_listenAddr.addr4(), sizeof(_listenAddr.addr4()));
    if (ret != 0) {
        mCritical() << "UdpServer bind error" << _listenSock << get_uv_errmsg();
        return false;
    }

    _thread->addEvent(_listenSock, EventType::Read, std::bind(&UdpServer2::onEvent, this, std::placeholders::_1, std::placeholders::_2));
    return true;
}

void UdpServer2::stop()
{
    if (_listenSock >= 0) {
        _thread->removeEvents(_listenSock);
        myclose(_listenSock);
    }
}

void UdpServer2::onEvent(SOCKET sock, int eventType) {
    if (eventType & EventType::Hangup) {
        mWarning() << "UdpServer hangup" << sock;
        return;
    }
    if (eventType & EventType::Error) {
        mWarning() << "UdpServer error" << sock;
        return;
    }

    if (eventType & EventType::Read) {
        char buffer[65536];

        struct sockaddr_in cliaddr;
        memset(&cliaddr, 0, sizeof(cliaddr));

        int n;
        socklen_t len = sizeof(cliaddr);  //len is value/result 
        n = recvfrom(sock, (char*)buffer, 65536, 0, (struct sockaddr*)&cliaddr, &len);
        if (n > 0) {
            INetAddress addr(cliaddr);
            _dataCb(sock, addr, buffer, n);
        }
        else if (n == 0) {
            // eof
            mWarning() << "UdpServer recv eof";
        }
        else { // n == -1
            // error
            mWarning() << "UdpServer recv error" << get_uv_errmsg();
        }
    }
}