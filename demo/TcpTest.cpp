#include <cstdlib>
#include <string>
#include <iostream>
#include <memory>
#include <stdio.h>

#include "MyLog.h"
#include "EventThread.h"
#include "TcpServer.h"
#include "TcpClient.h"
#include "TcpConnection.h"

using namespace DLNetwork;

class MySession : public Session {
public:
    MySession(EventThread* thread) : Session(thread) {}
    virtual ~MySession() {
        mInfo() << "~MySession";
    }

    virtual void onClosed() {
        mInfo() << "onClosed";
    }

    virtual bool onMessage(DLNetwork::Buffer* buf) {
        std::string inbuf;
        inbuf.append(buf->peek(), buf->readableBytes());
        buf->retrieveAll();
        mInfo() << "serverGot:" << inbuf;
        return true;
    }

    virtual void onWriteDone() {
        mInfo() << "onWriteDone";
    }
};

class MyTcpServer {
public:
    bool start() {
        INetAddress addr = INetAddress::fromIp4Port("127.0.0.1", 9999);

        auto sessionFactory = []() -> Session::Ptr {
            return std::make_shared<MySession>(EventThreadPool::instance().getIdlestThread());
        };

        _tcpServer = TcpServer::create();
        return _tcpServer->start(
            EventThreadPool::instance().getIdlestThread(), 
            addr, 
            sessionFactory
        );
    }
    ~MyTcpServer() {

    }
private:
    TcpServer::Ptr _tcpServer;
};

class MyTcpClient {
public:
    bool start() {
        _client = new TcpClient(EventThreadPool::instance().getIdlestThread(), INetAddress::fromIp4Port("127.0.0.1", 9999), "myclient");
        _client->setConnectCallback(std::bind(&MyTcpClient::onCliChange, this, std::placeholders::_1, std::placeholders::_2));
        _client->setOnConnectError(std::bind(&MyTcpClient::onCliError, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        _client->setOnMessage(std::bind(&MyTcpClient::onCliMessage, this, std::placeholders::_1, std::placeholders::_2));
        return _client->startConnect();
    }
    void write(const char *buf, size_t size) {
        _client->write(buf, size);
    }
    ~MyTcpClient() {
        _client = nullptr;
    }
private:
    void onCliChange(TcpClient& cli2, ConnectEvent e) {
        if (e == ConnectEvent::Established) {
            mInfo() << "input text to send, input quit to quit";
        }
        else if (e == ConnectEvent::Closed) {
        }
    }

    bool onCliMessage(TcpClient& cli2, Buffer* buf) {
        std::string inbuf;
        inbuf.append(buf->peek(), buf->readableBytes());
        buf->retrieveAll();
        mInfo() << "clientGot:" << inbuf;
        return true;
    }

    void onCliError(TcpClient& cli2, SOCKET sock, int ecode, std::string eMsg) {
        mInfo() << "clientError:" << eMsg;
    }

    TcpClient* _client;
};

void testTcp() {
    MyTcpServer s;
    s.start();

    MyTcpClient c;
    c.start();
    char buf[8*1024];
    for (int i = 0; i < 5; ++i) {
        char* r = gets_s(buf, sizeof(buf));
        if (!r) {
            break;
        }
        if (strcmp(buf, "quit") == 0) {
            break;
        }
        mInfo() << "clientSend:" << buf;
        c.write(buf, strlen(buf));
    }
} 