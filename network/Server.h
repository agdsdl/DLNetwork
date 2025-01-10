#pragma once
#include <memory>
#include <map>
#include "INetAddress.h"
#include "Session.h"
#include "Connection.h"
#include "TcpConnection.h"
#include "UdpConnection.h"

namespace DLNetwork {

class EventThread;
class Timer;

class Server : public std::enable_shared_from_this<Server> {
public:
    using SessionCreator = std::function<std::shared_ptr<Session>()>;

    virtual ~Server();

    virtual bool start(EventThread* loop, INetAddress listenAddr, SessionCreator sessionCreator, bool reusePort) = 0;
    void onManager();

protected:
    Server();
    void destroy();
    void onConnectionChange(Connection::Ptr conn, ConnectEvent event);

protected:
    SOCKET _listenSock;
    EventThread* _thread;
    INetAddress _listenAddr;
    SessionCreator _sessionCreator;
    bool _reusePort;
    Timer* _timer;
    std::map<Connection::Ptr, std::shared_ptr<Session>> _conn_session;
};

} // namespace DLNetwork 