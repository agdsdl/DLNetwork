#pragma once

#include "platform.h"
#include <functional>
#include <memory>
#include <mutex>
#include "EventThread.h"
#include "sockutil.h"
#include "Buffer.h"
#include "INetAddress.h"
#include "MyLog.h"

namespace DLNetwork {
enum class ConnectEvent {
    Established = 1,
    Closed
};

class TcpConnection
{
public:
    typedef std::function<void(TcpConnection& conn, ConnectEvent e)> ConnectionCallback;
    typedef std::function<bool(TcpConnection& conn, DLNetwork::Buffer*)> MessageCallback;
    typedef std::function<void(TcpConnection& conn)> WritedCallback;

    TcpConnection(EventThread* thread, SOCKET sock);
    ~TcpConnection();
    static std::unique_ptr<TcpConnection> connectTo(EventThread* thread, INetAddress addr);

    void attach();
    void setPeerAddr(INetAddress addr) {
        _peerAddr = addr;
    }
    void setConnectCallback(ConnectionCallback cb) {
        _connectionCb = cb;
    }
    void setOnMessage(MessageCallback cb) {
        _messageCb = cb;
    }
    void setOnWriteDone(WritedCallback cb) {
        _writedcb = cb;
    }
    void write(const char* buf, size_t size);
    void close();
    void reset();
    EventThread* getThread() {
        return _thread;
    }
    INetAddress& peerAddress() {
        return _peerAddr;
    }
    INetAddress& selfAddress() {
        return _selfAddr;
    }
    SOCKET sock() {
        return _sock;
    }
    bool isSelfConnection() {
        if (_selfAddr.isIP4() && _peerAddr.isIP4()) {
            return _selfAddr.addr4().sin_addr.s_addr == _peerAddr.addr4().sin_addr.s_addr && _selfAddr.addr4().sin_port == _peerAddr.addr4().sin_port;
        }
        else if (_selfAddr.isIP6() && _peerAddr.isIP6()) {
            return memcmp(_selfAddr.addr6().sin6_addr.s6_addr, _peerAddr.addr6().sin6_addr.s6_addr, 16) == 0 && _selfAddr.addr6().sin6_port == _peerAddr.addr6().sin6_port;
        }
        else {
            return false;
        }
    }
    std::string description();
    void closeAfterWrite();
private:
    void onEvent(SOCKET sock, int eventType);
    bool handleRead(SOCKET sock);
    bool handleWrite(SOCKET sock);
    void handleHangup(SOCKET sock);
    void handleError(SOCKET sock);
    void writeInThread(const char* buf, size_t size);

    DLNetwork::Buffer _readBuf;
    DLNetwork::Buffer _writeBuf;
    SOCKET _sock;
    EventThread* _thread;
    ConnectionCallback _connectionCb;
    MessageCallback _messageCb;
    WritedCallback _writedcb;
    bool _closing;
    std::mutex _writeBufMutex;
    int _eventType;
    INetAddress _peerAddr;
    INetAddress _selfAddr;
    bool _closeAfterWrite = false;
};

} // DLNetwork
