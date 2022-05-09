#pragma once

#include <functional>
#include <string.h>
#include <memory>
#include "platform.h"
#include "Buffer.h"
#include "EventThread.h"
#include "TcpConnection.h"

namespace DLNetwork {
class TcpServer
{
public:
    TcpServer();
    ~TcpServer();
    typedef std::function<void(std::unique_ptr<TcpConnection>&& conn)> ConnectionAcceptCallback;


    bool start(EventThread* loop, sockaddr_in listenAddr, std::string name, bool reusePort = true);
    void stop();

    void setConnectionAcceptCallback(const ConnectionAcceptCallback& cb) {
        _connectionCb = cb;
    }

private:
    void onEvent(SOCKET sock, int eventType);

    ConnectionAcceptCallback _connectionCb;
    sockaddr_in _listenAddr;
    SOCKET _listenSock;
    EventThread* _thread;
    std::string _name;
    bool _reusePort;
};
} // DLNetwork