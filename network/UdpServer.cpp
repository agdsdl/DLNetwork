#include "UdpServer.h"
#include "sockutil.h"
#include "EventThread.h"
#include "MyLog.h"
#include "uv_errno.h"

using namespace DLNetwork;

UdpServer::UdpServer()
{
}

UdpServer::~UdpServer()
{
    stop();
}

bool UdpServer::start(EventThread* loop, sockaddr_in listenAddr, std::string name, bool reusePort)
{
    _thread = loop;
    _listenAddr = listenAddr;
    _name = std::move(name);
    _reusePort = reusePort;

    _listenSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (_listenSock == SOCKET_ERROR) {
        mCritical() << "UdpServer create socket error" << get_uv_errmsg();
        return false;
    }
    SockUtil::setNoBlocked(_listenSock, true);
    SockUtil::setReuseable(_listenSock, _reusePort);
    int ret = bind(_listenSock, (sockaddr*)&listenAddr, sizeof(listenAddr));
    if (ret != 0) {
        mCritical() << "UdpServer bind error" << _listenSock << get_uv_errmsg();
        return false;
    }

    _thread->addEvent(_listenSock, EventType::Read, std::bind(&UdpServer::onEvent, this, std::placeholders::_1, std::placeholders::_2));
    return true;
}

void UdpServer::stop()
{
    if (_listenSock >= 0) {
        _thread->removeEvents(_listenSock);
        myclose(_listenSock);
    }
}

void UdpServer::onEvent(SOCKET sock, int eventType) {
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
            _dataCb(sock, cliaddr, buffer, n);
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