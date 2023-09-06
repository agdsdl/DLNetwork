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
#include <memory>
#include <unordered_map>
#include <map>
#include <string>
#include "uv_errno.h"
#include "TcpServer.h"
#include "TcpConnection.h"
#include "MyHttpSession.h"
#include "MyHttp2Session.h"
#include "MyLog.h"

namespace DLNetwork {
template <class SESSION>
class _MyHttpServer
{
public:
    using CallbackSession = typename SESSION::CallbackSession;
    //typedef SESSION::CallbackSession CallbackSession;
    typedef std::function<void(HTTP::Request& request, std::shared_ptr<CallbackSession> sess) > UrlHandler;

    _MyHttpServer() {

    }
    ~_MyHttpServer() {

    }
    bool start(const char* ip, int port, const char* certFile = nullptr, const char* keyFile = nullptr) {
        if (certFile) {
            _certFile = certFile;
        }
        if (keyFile) {
            _keyFile = keyFile;
        }

        _thread = EventThreadPool::instance().getIdlestThread();
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = DLNetwork::hostToNetwork16(port);

        if (::inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
            mCritical() << "_MyHttpServer inet_pton failed" << get_uv_errmsg();
            return false;
        }
        _tcpServer.setConnectionAcceptCallback(std::bind(&_MyHttpServer::onConnection, this, std::placeholders::_1));
        return _tcpServer.start(_thread, addr, "myhttpserver", true);
    }
    void setHandler(UrlHandler handler) {
        _handler = handler;
    }
    EventThread* thread() {
        return _thread;
    }
private:
    void onConnection(std::unique_ptr<TcpConnection>&& conn) {
        TcpConnection* pConn = conn.get();
        auto session = std::make_shared<SESSION>(std::move(conn));
        if (_certFile.size()) {
            pConn->enableTls(_certFile, _keyFile, SESSION::supportH2);
        }
        session->setUrlHandler(std::bind(&_MyHttpServer::urlHanlder, this, std::placeholders::_1, std::placeholders::_2));
        session->addClosedHandler(std::bind(&_MyHttpServer::closedHandler, this, std::placeholders::_1));
        _conn_session[pConn] = session;
        session->takeoverConn();
    }
    //void onConnectionChange(TcpConnection& conn, ConnectEvent e);
    //bool onMessage(TcpConnection& conn, DLNetwork::Buffer* buf);
    //void onWriteDone(TcpConnection& conn);

    void closedHandler(std::shared_ptr<SESSION> sess) {
        // don't self delete
        _thread->dispatch([this, sess]() {
            _conn_session.erase(&sess->connection());
            });
    }
    void urlHanlder(HTTP::Request& request, std::shared_ptr<CallbackSession> sess) {
        if (_handler) {
            _handler(request, sess);
        }
    }
    TcpServer _tcpServer;
    std::unordered_map<TcpConnection*, std::shared_ptr<SESSION>> _conn_session;
    UrlHandler _handler;
    EventThread* _thread;

    std::string _certFile;
    std::string _keyFile;
};

typedef _MyHttpServer<MyHttpSession> MyHttpServer;
typedef _MyHttpServer<MyHttp2Session> MyHttp2Server;
} //DLNetwork
