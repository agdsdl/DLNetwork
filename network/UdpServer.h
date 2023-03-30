#pragma once

#include <functional>
#include <string.h>
#include <memory>
#include "platform.h"
#include "Buffer.h"
#include "EventThread.h"
#include "INetAddress.h"

namespace DLNetwork {
class UdpServer
{
public:
    UdpServer();
    ~UdpServer();
    typedef std::function<void(SOCKET sock, sockaddr_in clientAddr, char* data, int size)> RecvDataCallback;


    bool start(EventThread* loop, sockaddr_in listenAddr, std::string name, bool reusePort = true);
    void stop();

    void setRecvDataCallback(const RecvDataCallback& cb) {
        _dataCb = cb;
    }

    EventThread* thread() { return _thread; }
    SOCKET sock() { return _listenSock; }
private:
    void onEvent(SOCKET sock, int eventType);

    RecvDataCallback _dataCb;
    sockaddr_in _listenAddr;
    SOCKET _listenSock;
    EventThread* _thread;
    std::string _name;
    bool _reusePort;
};
} // DLNetwork