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
#include "TcpServer.h"
#include "sockutil.h"
#include "EventThread.h"
#include "MyLog.h"
#include "uv_errno.h"

using namespace DLNetwork;

TcpServer::TcpServer()
{
}

TcpServer::~TcpServer()
{
    stop();
}

bool TcpServer::start(EventThread* loop, sockaddr_in listenAddr, std::string name, bool reusePort)
{
    _thread = loop;
    _listenAddr = listenAddr;
    _name = std::move(name);
    _reusePort = reusePort;

    _listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (_listenSock == SOCKET_ERROR) {
        return false;
    }
    SockUtil::setNoBlocked(_listenSock, true);
    SockUtil::setReuseable(_listenSock, _reusePort);
    int ret = bind(_listenSock, (sockaddr*)&listenAddr, sizeof(listenAddr));
    if (ret != 0) {
        mWarning() << "TcpServer bind error" << get_uv_errmsg();
        return false;
    }

    ret = ::listen(_listenSock, SOMAXCONN);
    if (ret < 0) {
        mWarning() << "TcpServer listen error" << get_uv_errmsg();
        return false;
    }

    _thread->addEvent(_listenSock, EventType::Read, std::bind(&TcpServer::onEvent, this, std::placeholders::_1, std::placeholders::_2));
    return true;
}

void TcpServer::stop()
{
    if (_listenSock >= 0) {
        _thread->removeEvents(_listenSock);
        myclose(_listenSock);
    }
}

void TcpServer::onEvent(SOCKET sock, int eventType) {
    EventThread* t = EventThreadPool::instance().getIdlestThread();
    t->dispatch([sock, t, this, eventType]() {
        sockaddr_in clientAddr;
        socklen_t nLen = sizeof(sockaddr_in);
        SOCKET fsock = ::accept(sock, (sockaddr*)&clientAddr, &nLen);
        if (fsock != -1) {
            auto conn = std::make_unique<TcpConnection>(t, fsock);
            if (_connectionCb) {
                _connectionCb(std::move(conn));
            }
        }
        else {
            //mWarning() << "TcpServer::onEvent accept error" << get_uv_errmsg();
        }
    });
}