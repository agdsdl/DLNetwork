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
#include "uv_errno.h"

namespace DLNetwork {
enum class ConnectEvent {
    Established = 1,
    Closed
};
class TcpConnection;
class Connection : public std::enable_shared_from_this<Connection>
{
public:
    using Ptr = std::shared_ptr<Connection>;
    typedef std::function<void(Connection::Ptr conn, ConnectEvent e)> ConnectionCallback;
    typedef std::function<bool(Connection::Ptr conn, DLNetwork::Buffer*)> MessageCallback;
    typedef std::function<void(Connection::Ptr conn)> WritedCallback;

    virtual ~Connection();
#ifdef ENABLE_OPENSSL
    void enableTls(std::string certFile, std::string keyFile, bool supportH2=true);
    void enableTlsClient(std::string hostName, std::string certFile);
#endif // ENABLE_OPENSSL
    //static Ptr createClient(EventThread* thread, INetAddress addr, bool isTcp = true);
    template<typename ConnectionType>
    static Connection::Ptr createClient(EventThread* thread, INetAddress peerAddr, INetAddress localAddr=INetAddress::invalidAddress()) {
        if (!peerAddr.isIP4() && !peerAddr.isIP6()) {
            mCritical() << "Connection::connectTo badAddr!" << peerAddr.description().c_str();
            return nullptr;
        }
        const bool isTcp = std::is_same<ConnectionType, TcpConnection>::value;
        SOCKET sock = socket(peerAddr.isIP6() ? AF_INET6 : AF_INET, isTcp ? SOCK_STREAM : SOCK_DGRAM, 0);
        if (sock == INVALID_SOCKET) {
            mCritical() << "Connection::connectTo create socket failed:" << get_uv_errmsg();
            return nullptr;
        }
		SockUtil::setNoBlocked(sock, true);
        if (!isTcp) {
            SockUtil::setReuseable(sock, true);
            SockUtil::setRecvBuf(sock, 1024 * 1024);
            SockUtil::setSendBuf(sock, 1024 * 1024);
        }

        if(localAddr.isValid()) {
			int nret = 0;
            if(localAddr.isIP4()) {
                nret = ::bind(sock, (sockaddr*)&localAddr.addr4(), sizeof(localAddr.addr4()));
            }
            else if(localAddr.isIP6()) {
                nret = ::bind(sock, (sockaddr*)&localAddr.addr6(), sizeof(localAddr.addr6()));
            }
            if (nret < 0) {
                mCritical() << "Connection::connectTo bind local port error" << localAddr << get_uv_errmsg();
                myclose(sock);
                return nullptr;
            }
        }

        auto conn = create<ConnectionType>(thread, sock);
        conn->setPeerAddr(peerAddr);
        //conn->attach();
        conn->_clientMode = true;
        return conn;
    }

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
    void close(bool notify=true);
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
    template<typename ConnectionType>
    static Ptr create(EventThread* thread, SOCKET sock) {
        auto t = thread ? thread : EventThreadPool::instance().getIdlestThread();
        auto conn = std::shared_ptr<ConnectionType>(new ConnectionType(t, sock), [t](ConnectionType *ptr) {
            if (t) {
                t->dispatch([ptr]() {
                    delete ptr;
                });
            } else {
                delete ptr;
            }
        });
        return conn;
    }
    bool handleReceivedData(const char* buf, size_t size); // 供UdpServer使用

    #ifdef ENABLE_OPENSSL
    void initTls();
    DLNetwork::Buffer _decodedBuf;
    #endif
    Connection(EventThread* thread, SOCKET sock);

    void writeInner(const char* buf, size_t size);
    bool readInner();
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
#ifdef ENABLE_OPENSSL
    std::unique_ptr<SSLWrapper> _ssl;
#endif
    int _eventType;
    INetAddress _peerAddr;
    INetAddress _selfAddr;
    bool _closeAfterWrite = false;
    bool _clientMode = false;
    bool _clientModeConnected = false;
    bool _attached = false;

    friend class UdpServer;
};

} // DLNetwork

DLNetwork::MyOut& operator<<(DLNetwork::MyOut& o, DLNetwork::Connection& c);
