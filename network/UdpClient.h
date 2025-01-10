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
#include <string>
#include <memory>
#include "EventThread.h"
#include "UdpConnection.h"
#include "Buffer.h"

namespace DLNetwork {

class UdpClient {
public:
    using ConnectCallback = std::function<void(UdpClient&, ConnectEvent)>;
    using MessageCallback = std::function<bool(UdpClient&, Buffer*)>;
    using ConnectErrorCallback = std::function<void(UdpClient&, SOCKET, int, std::string)>;

    UdpClient(EventThread* thread, INetAddress serverAddr, INetAddress localAddr, std::string name);
    ~UdpClient();

    bool startConnect();
    void stop();
    void write(const char* buf, size_t size);

    void setConnectCallback(ConnectCallback cb) { _connectCallback = cb; }
    void setOnMessage(MessageCallback cb) { _messageCallback = cb; }
    void setOnConnectError(ConnectErrorCallback cb) { _connectErrorCallback = cb; }

    std::string name() { return _name; }
    EventThread* thread() { return _thread; }
    Connection::Ptr connection() { return _connection; }

private:
    void onConnectionChange(Connection::Ptr conn, ConnectEvent e);
    bool onMessage(Connection::Ptr conn, Buffer* buf);

    EventThread* _thread;
    INetAddress _serverAddr;
    INetAddress _localAddr;
    std::string _name;
    Connection::Ptr _connection;

    ConnectCallback _connectCallback;
    MessageCallback _messageCallback;
    ConnectErrorCallback _connectErrorCallback;
};

} // DLNetwork 