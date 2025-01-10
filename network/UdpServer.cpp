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
#include "UdpServer.h"
#include "sockutil.h"
#include "MyLog.h"
#include "uv_errno.h"

namespace DLNetwork {

UdpServer::UdpServer() : Server() {
}

UdpServer::~UdpServer() {
    destroy();
}

bool UdpServer::start(EventThread* loop, INetAddress listenAddr, SessionCreator sessionCreator, bool reusePort) {
    _thread = loop;
    _listenAddr = listenAddr;
    _sessionCreator = sessionCreator;
    _reusePort = reusePort;

    _listenSock = socket(listenAddr.isIP6() ? AF_INET6 : AF_INET, SOCK_DGRAM, 0);
    if (_listenSock == INVALID_SOCKET) {
        mCritical() << "UdpServer::start create socket failed:" << get_uv_errmsg();
        return false;
    }

    SockUtil::setNoBlocked(_listenSock, true);
    SockUtil::setReuseable(_listenSock, _reusePort);
    int ret = 0;
    if (listenAddr.isIP4()) {
        ret = bind(_listenSock, (sockaddr*)&listenAddr.addr4(), sizeof(listenAddr.addr4()));
    } else if (listenAddr.isIP6()) {
        ret = bind(_listenSock, (sockaddr*)&listenAddr.addr6(), sizeof(listenAddr.addr6()));
    }
    if (ret != 0) {
        mWarning() << "TcpServer bind error" << get_uv_errmsg();
        return false;
    }

    _thread->addEvent(_listenSock, EventType::Read, std::bind(&UdpServer::onEvent, this, std::placeholders::_1, std::placeholders::_2));
    std::weak_ptr<Server> weak_this = shared_from_this();
    EventThreadPool::instance().forEach([weak_this](EventThread* thread) {
        thread->addTimer(2000, [weak_this](void*) {
            auto shared_this = weak_this.lock();
            if (shared_this) {
                shared_this->onManager();
            }
            return 2000;
        }, nullptr);
    });
    return true;
}

void UdpServer::onEvent(SOCKET sock, int eventType) {
    if (eventType & EventType::Read) {
        struct sockaddr_in cliaddr;
        memset(&cliaddr, 0, sizeof(cliaddr));
        socklen_t len = sizeof(cliaddr);  //len is value/result 
        char buf[65536];
        int n = recvfrom(_listenSock, buf, sizeof(buf), 0, (struct sockaddr*)&cliaddr, &len);
        if (n > 0) {
            INetAddress peerAddr(cliaddr);
            // 先查找是否已存在对应peer地址的连接
            Connection::Ptr existingConn = nullptr;
            for(const auto& pair : _conn_session) {
                if(pair.first->peerAddress() == peerAddr) {
                    existingConn = pair.first;
                    break;
                }
            }

            if(!existingConn) {
                if (_sessionCreator) {
                    auto session = _sessionCreator();
                    auto conn = UdpConnection::create(session->thread(), peerAddr, _listenAddr);
                    if (conn) {
                        conn->startConnect();
                        conn->setConnectCallback(std::bind(&UdpServer::onConnectionChange, this, std::placeholders::_1, std::placeholders::_2));
                        _conn_session[conn] = session;
                        session->takeoverConn(conn);
                        session->onConnectionChange(conn, ConnectEvent::Established);
                        existingConn = conn;
                    }
                    else {
                        mWarning() << "UdpServer::onEvent create session failed local:" << _listenAddr << " peer:" << peerAddr;
                    }
                }
                else {
                    destroy();
                }
            }

            if(existingConn) {
                // udp可以乱序，但要保证线程安全
                if (existingConn->getThread()->isCurrentThread()) {
                    existingConn->handleReceivedData(buf, n);
                }
                else {
                    std::shared_ptr<std::string> data = std::make_shared<std::string>(buf, n);
                    existingConn->getThread()->dispatch([existingConn, data]() {
                        existingConn->handleReceivedData(data->data(), data->size());
                    }, true, true);
                }
            }
        }
    }
    else if (eventType & EventType::Hangup) {
        assert(false);
        return;
    }
    else if (eventType & EventType::Error) {

    }
}

} // DLNetwork 