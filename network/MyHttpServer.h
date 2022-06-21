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
