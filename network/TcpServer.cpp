#include "TcpServer.h"
#include "sockutil.h"
#include "EventThread.h"
#include "MyLog.h"
#include "uv_errno.h"

using namespace DLNetwork;

TcpServer::TcpServer()
{
}

TcpServer::~TcpServer()
{
    stop();
}

bool TcpServer::start(EventThread* loop, sockaddr_in listenAddr, std::string name, bool reusePort)
{
    _thread = loop;
    _listenAddr = listenAddr;
    _name = std::move(name);
    _reusePort = reusePort;

    _listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_listenSock == SOCKET_ERROR) {
        return false;
    }
    SockUtil::setNoBlocked(_listenSock, true);
    SockUtil::setReuseable(_listenSock, _reusePort);
    int ret = bind(_listenSock, (sockaddr*)&listenAddr, sizeof(listenAddr));
    if (ret != 0) {
        mWarning() << "TcpServer bind error" << get_uv_errmsg();
        return false;
    }

    ret = ::listen(_listenSock, SOMAXCONN);
    if (ret < 0) {
        mWarning() << "TcpServer listen error" << get_uv_errmsg();
        return false;
    }

    _thread->addEvent(_listenSock, EventType::Read, std::bind(&TcpServer::onEvent, this, std::placeholders::_1, std::placeholders::_2));
    return true;
}

void TcpServer::stop()
{
    if (_listenSock >= 0) {
        _thread->removeEvents(_listenSock);
        myclose(_listenSock);
    }
}

void TcpServer::onEvent(SOCKET sock, int eventType) {
    EventThread* t = EventThreadPool::instance().getIdlestThread();
    t->dispatch([sock, t, this, eventType]() {
        sockaddr_in clientAddr;
        socklen_t nLen = sizeof(sockaddr_in);
        SOCKET fsock = ::accept(sock, (sockaddr*)&clientAddr, &nLen);
        if (fsock != -1) {
            auto conn = std::make_unique<TcpConnection>(t, fsock);
            if (_connectionCb) {
                _connectionCb(std::move(conn));
            }
        }
        else {
            //mWarning() << "TcpServer::onEvent accept error" << get_uv_errmsg();
        }
    });
}