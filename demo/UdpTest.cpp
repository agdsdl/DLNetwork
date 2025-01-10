/*
 * MIT License
 *
 * Copyright (c) 2019-2022 agdsdl <agdsdl@sina.com.cn>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "MyLog.h"
#include "EventThread.h"
#include "UdpServer.h"
#include "UdpClient.h"
#include "UdpConnection.h"
#include "Session.h"
#include "UdpTest.h"

using namespace DLNetwork;

class MyUdpSession : public Session {
public:
    MyUdpSession(EventThread* thread) : Session(thread) {}
    virtual ~MyUdpSession() {
        mInfo() << "~MyUdpSession";
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
    virtual void onManager() {
        time_t now = time(nullptr);
        if (now - lastActiveTime() > 10) {  
            mInfo() << "onManager timeout" << sendSize() << recvSize() << lastActiveTime();
            destroy();
        }
    }
};

class UdpTestServer {
public:
    bool start() {
        INetAddress addr = INetAddress::fromIp4Port("127.0.0.1", 9998);

        auto sessionFactory = []() -> Session::Ptr {
            return std::make_shared<MyUdpSession>(EventThreadPool::instance().getIdlestThread());
        };

        _udpServer = UdpServer::create();
        return _udpServer->start(
            EventThreadPool::instance().getIdlestThread(), 
            addr, 
            sessionFactory
        );
    }
    ~UdpTestServer() {
    }
private:
    UdpServer::Ptr _udpServer;
};

class UdpTestClient {
public:
    bool start() {
        _client = new UdpClient(EventThreadPool::instance().getIdlestThread(), 
                               INetAddress::fromIp4Port("127.0.0.1", 9998), 
                               INetAddress::fromIp4Port("127.0.0.1", 8999),
                               "udp_test_client");
        _client->setConnectCallback(std::bind(&UdpTestClient::onCliChange, this, std::placeholders::_1, std::placeholders::_2));
        _client->setOnConnectError(std::bind(&UdpTestClient::onCliError, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        _client->setOnMessage(std::bind(&UdpTestClient::onCliMessage, this, std::placeholders::_1, std::placeholders::_2));
        return _client->startConnect();
    }
    void write(const char *buf, size_t size) {
        _client->write(buf, size);
    }
    ~UdpTestClient() {
        if(_client) {
            _client->stop();
            delete _client;
            _client = nullptr;
        }
    }
private:
    void onCliChange(UdpClient& cli2, ConnectEvent e) {
        if (e == ConnectEvent::Established) {
            mInfo() << "UDP connected, input text to send, input quit to quit";
        }
        else if (e == ConnectEvent::Closed) {
            mInfo() << "UDP connection closed";
        }
    }

    bool onCliMessage(UdpClient& cli2, Buffer* buf) {
        std::string inbuf;
        inbuf.append(buf->peek(), buf->readableBytes());
        buf->retrieveAll();
        mInfo() << "clientGot:" << inbuf;
        return true;
    }

    void onCliError(UdpClient& cli2, SOCKET sock, int ecode, std::string eMsg) {
        mInfo() << "clientError:" << eMsg;
    }

    UdpClient* _client = nullptr;
};

void testUdp() {
    UdpTestServer server;
    server.start();
    UdpTestClient client;
    client.start();

    char buf[8*1024];
    for (int i = 0; i < 5; ++i) {
        char* r = gets_s(buf, sizeof(buf));
        if (!r) {
            break;
        }
        if (strcmp(buf, "quit") == 0) {
            break;
        }
        mInfo() << "udpClientSend:" << buf;
        client.write(buf, strlen(buf));
    }
} 