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
    bool start(EventThread* thread,INetAddress listenAddr, const char* certFile = nullptr, const char* keyFile = nullptr) {
        _thread = thread;
        if (certFile) {
            _certFile = certFile;
        }
        if (keyFile) {
            _keyFile = keyFile;
        }

        return _tcpServer.start(_thread, listenAddr, [this](){
            auto sess = std::make_shared<SESSION>(_thread);
            sess->setSSLCert(_certFile, _keyFile, SESSION::supportH2);
            return sess;
        }, true);
    }
    void setHandler(UrlHandler handler) {
        _handler = handler;
    }
    EventThread* thread() {
        return _tcpServer.thread();
    }
private:
    void urlHanlder(HTTP::Request& request, std::shared_ptr<CallbackSession> sess) {
        if (request.method == DLNetwork::HTTP::Method::HTTP_OPTIONS) {
            sess->response("");
            //sess->stop();
            return;
        }

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
