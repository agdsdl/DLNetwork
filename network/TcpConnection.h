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
#pragma once

#include "platform.h"
#include <functional>
#include <memory>
#include <mutex>
#include <deque>
#include "EventThread.h"
#include "sockutil.h"
#include "Buffer.h"
#include "INetAddress.h"
#include "MyLog.h"
#include "SSLWrapper.h"

namespace DLNetwork {
enum class ConnectEvent {
    Established = 1,
    Closed
};

class TcpConnection : public std::enable_shared_from_this<TcpConnection>
{
public:
    using Ptr = std::shared_ptr<TcpConnection>;
    static Ptr create(EventThread* thread, SOCKET sock) {
        auto t = thread ? thread : EventThreadPool::instance().getIdlestThread();
        auto conn = std::shared_ptr<TcpConnection>(new TcpConnection(t, sock), [t](TcpConnection *ptr) {
            if (t) {
                t->dispatch([ptr]() { delete ptr; });
            } else {
                delete ptr;
            }
        });
        return conn;
    }
    typedef std::function<void(TcpConnection::Ptr conn, ConnectEvent e)> ConnectionCallback;
    typedef std::function<bool(TcpConnection::Ptr conn, DLNetwork::Buffer*)> MessageCallback;
    typedef std::function<void(TcpConnection::Ptr conn)> WritedCallback;

    ~TcpConnection();
#ifdef ENABLE_OPENSSL
    void enableTls(std::string certFile, std::string keyFile, bool supportH2=true);
#endif // ENABLE_OPENSSL
    static Ptr createClient(EventThread* thread, INetAddress addr);
    void startConnect();
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
protected:
    TcpConnection(EventThread* thread, SOCKET sock);

    void writeInner(const char* buf, size_t size);
    void onEvent(SOCKET sock, int eventType);
    bool handleRead(SOCKET sock);
    bool handleWrite(SOCKET sock);
    bool realSend();
    void handleHangup(SOCKET sock);
    void handleError(SOCKET sock);
    void writeInThread(const char* buf, size_t size);

    DLNetwork::Buffer _readBuf;
    std::deque<DLNetwork::Buffer> _writeBuf;
    SOCKET _sock;
    EventThread* _thread;
    ConnectionCallback _connectionCb;
    MessageCallback _messageCb;
    WritedCallback _writedcb;
    bool _closing;
    std::mutex _writeBufMutex;
    std::unique_ptr<SSLWrapper> _ssl;
    int _eventType;
    INetAddress _peerAddr;
    INetAddress _selfAddr;
    bool _closeAfterWrite = false;
    bool _clientMode = false;
    bool _connected = false;
};

} // DLNetwork

DLNetwork::MyOut& operator<<(DLNetwork::MyOut& o, DLNetwork::TcpConnection& c);
