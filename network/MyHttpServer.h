#pragma once
#include <memory>
#include <unordered_map>
#include <map>
#include "TcpServer.h"
#include "TcpConnection.h"
#include "MyHttpSession.h"

namespace DLNetwork {
class MyHttpServer
{
public:
    typedef std::function<void(HTTP::Request& request, std::shared_ptr<MyHttpSession> sess) > UrlHandler;

    MyHttpServer();
    ~MyHttpServer();
    bool start(const char* ip, int port);
    void setHandler(UrlHandler handler) {
        _handler = handler;
    }

private:
    void onConnection(std::unique_ptr<TcpConnection>&& conn);
    //void onConnectionChange(TcpConnection& conn, ConnectEvent e);
    //bool onMessage(TcpConnection& conn, DLNetwork::Buffer* buf);
    //void onWriteDone(TcpConnection& conn);

    void closedHandler(std::shared_ptr<MyHttpSession> sess);
    void urlHanlder(HTTP::Request& request, std::shared_ptr<MyHttpSession> sess);
    TcpServer _tcpServer;
    std::unordered_map<TcpConnection*, std::shared_ptr<MyHttpSession>> _conn_session;
    UrlHandler _handler;
};
} //DLNetwork
