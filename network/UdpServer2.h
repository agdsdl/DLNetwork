#pragma once

#include <functional>
#include <string.h>
#include <memory>
#include "platform.h"
#include "Buffer.h"
#include "EventThread.h"
#include "INetAddress.h"

namespace DLNetwork {
class UdpServer2
{
public:
    UdpServer2();
    ~UdpServer2();
    typedef std::function<void(SOCKET sock, INetAddress& clientAddr, char* data, int size)> RecvDataCallback;


    bool start(EventThread* loop, INetAddress listenAddr, std::string name, bool reusePort = true);
    void stop();

    void setRecvDataCallback(const RecvDataCallback& cb) {
        _dataCb = cb;
    }

    EventThread* thread() { return _thread; }
    SOCKET sock() { return _listenSock; }
private:
    void onEvent(SOCKET sock, int eventType);

    RecvDataCallback _dataCb;
    INetAddress _listenAddr;
    SOCKET _listenSock;
    EventThread* _thread;
    std::string _name;
    bool _reusePort;
};
} // DLNetwork