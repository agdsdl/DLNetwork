#include <cstdlib>
#include <ctime>
#include <thread>
#include <platform.h>
#include <string>
#include <iostream>
#include <sstream>
#include <csignal>
#include <set>
#include <memory>
#include <mutex>
#include <stdio.h>

#include "MyLog.h"
#include "EventThread.h"
#include "TcpServer.h"
#include "TcpClient.h"
#include "TcpConnection.h"

using namespace DLNetwork;

bool InitSocket()
{
#ifdef WIN32
    int Error;
    WORD VersionRequested;
    WSADATA WsaData;
    VersionRequested = MAKEWORD(2, 2);
    Error = WSAStartup(VersionRequested, &WsaData); //WinSock2
    if (Error != 0) {
        printf("Error:Start WinSock failed!\n");
        return false;
    }
    else {
        if (LOBYTE(WsaData.wVersion) != 2 || HIBYTE(WsaData.wHighVersion) != 2) {
            printf("Error:The version is WinSock2!\n");
            WSACleanup();
            return false;
        }
    }
#endif
    return true;
}

class Server {
public:
    bool start() {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = ADDR_ANY;
        addr.sin_port = DLNetwork::hostToNetwork16(9999);

        _tcpServer.setConnectionAcceptCallback(std::bind(&Server::onConnection, this, std::placeholders::_1));
        return _tcpServer.start(EventThreadPool::instance().getIdlestThread(), addr, "myTcpServer", true);
    }
    ~Server() {
        _tcpServer.stop();
        std::set<TcpConnection*> temp;
        {
            std::lock_guard<std::mutex> l(_mutex);
            temp.swap(_connetions);
        }
        _tcpServer.thread()->dispatch([temp]() {
            for (auto c : temp) {
                delete c;
            }
        });
    }
private:
    void onConnection(std::unique_ptr<TcpConnection>&& conn) {
        conn->setConnectCallback(std::bind(&Server::onConnectionChange, this, std::placeholders::_1, std::placeholders::_2));
        conn->setOnMessage(std::bind(&Server::onMessage, this, std::placeholders::_1, std::placeholders::_2));
        conn->setOnWriteDone(std::bind(&Server::onWriteDone, this, std::placeholders::_1));
        conn->attach();

        std::lock_guard<std::mutex> l(_mutex);
        _connetions.insert(conn.release());
    }

    void onConnectionChange(TcpConnection& conn, ConnectEvent e) {
        if (e == ConnectEvent::Closed) {
            {
                std::lock_guard<std::mutex> l(_mutex);
                _connetions.erase(&conn);
            }
            delete &conn;
        }
    }

    bool onMessage(TcpConnection& conn, DLNetwork::Buffer* buf) {
        std::string inbuf;
        inbuf.append(buf->peek(), buf->readableBytes());
        buf->retrieveAll();
        mInfo() << "serverGot:" << inbuf;
        return true;
    }

    void onWriteDone(TcpConnection& conn) {
    }

    std::set<TcpConnection*> _connetions;
    TcpServer _tcpServer;
    std::mutex _mutex;
};

class Client {
public:
    bool start() {
        _client = new TcpClient(EventThreadPool::instance().getIdlestThread(), INetAddress::fromIp4Port("127.0.0.1", 9999), "myclient");
        _client->setConnectCallback(std::bind(&Client::onCliChange, this, std::placeholders::_1, std::placeholders::_2));
        _client->setOnConnectError(std::bind(&Client::onCliError, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        _client->setOnMessage(std::bind(&Client::onCliMessage, this, std::placeholders::_1, std::placeholders::_2));
        return _client->startConnect();
    }
    void write(const char *buf, size_t size) {
        _client->write(buf, size);
    }
    ~Client() {
        EventThread* t = _client->thread();
        t->dispatch([c = _client]() mutable {
            delete c;
            });
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

int main(int argc, char** argv) {
    InitSocket();

    EventThreadPool::instance().init(1);
    Server s;
    s.start();

    Client c;
    c.start();
    char buf[128];
    do {
        char* r = gets_s(buf, 128);
        if (!r) {
            break;
        }
        if (strcmp(buf, "quit") == 0) {
            break;
        }
        mInfo() << "clientSend:" << buf;
        c.write(buf, strlen(buf));
    } while (true);

#ifdef WIN32
    WSACleanup();
#endif
    return 0;
}
