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

#include <functional>
#include <string.h>
#include <memory>
#include "platform.h"
#include "Buffer.h"
#include "EventThread.h"
#include "TcpConnection.h"

namespace DLNetwork {
class TcpServer
{
public:
    TcpServer();
    ~TcpServer();
    typedef std::function<void(std::unique_ptr<TcpConnection>&& conn)> ConnectionAcceptCallback;


    bool start(EventThread* loop, sockaddr_in listenAddr, std::string name, bool reusePort = true);
    void stop();

    void setConnectionAcceptCallback(const ConnectionAcceptCallback& cb) {
        _connectionCb = cb;
    }
    EventThread* thread() { return _thread; }
private:
    void onEvent(SOCKET sock, int eventType);

    ConnectionAcceptCallback _connectionCb;
    sockaddr_in _listenAddr;
    SOCKET _listenSock;
    EventThread* _thread;
    std::string _name;
    bool _reusePort;
};
} // DLNetwork