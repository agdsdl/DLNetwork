#include "MyLog.h"
#include "EventThread.h"
#include "TcpServer.h"
#include "TcpClient.h"
#include "TcpConnection.h"
#include "Session.h"
#include "TlsTest.h"

using namespace DLNetwork;
class MyTlsSession : public Session {
public:
    MyTlsSession(EventThread* thread) : Session(thread) {}
    virtual ~MyTlsSession() {
        mInfo() << "~MyTlsSession";
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

class TlsServer {
public:
    bool start() {
        INetAddress addr = INetAddress::fromIp4Port("127.0.0.1", 9999);

        auto sessionFactory = []() -> Session::Ptr {
            auto ret = std::make_shared<MyTlsSession>(EventThreadPool::instance().getIdlestThread());
            ret->setSSLCert("server.crt", "server.key", true);
            return ret;
        };

        _tcpServer = TcpServer::create();
        return _tcpServer->start(
            EventThreadPool::instance().getIdlestThread(), 
            addr, 
            sessionFactory
        );
    }
    ~TlsServer() {

    }
private:
    TcpServer::Ptr _tcpServer;
};

class TlsClient {
public:
    bool start() {
        _client = new TcpClient(EventThreadPool::instance().getIdlestThread(), INetAddress::fromIp4Port("127.0.0.1", 9999), "myclient", true);
        _client->setConnectCallback(std::bind(&TlsClient::onCliChange, this, std::placeholders::_1, std::placeholders::_2));
        _client->setOnConnectError(std::bind(&TlsClient::onCliError, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
        _client->setOnMessage(std::bind(&TlsClient::onCliMessage, this, std::placeholders::_1, std::placeholders::_2));
        return _client->startConnect();
    }
    void write(const char *buf, size_t size) {
        _client->write(buf, size);
    }
    ~TlsClient() {
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

void testTls() {
    TlsServer server;
    server.start();
    TlsClient client;
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
        mInfo() << "tlsclientSend:" << buf;
        client.write(buf, strlen(buf));
    }

}
