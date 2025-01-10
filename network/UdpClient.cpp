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
#include "UdpClient.h"
#include "MyLog.h"
#include "sockutil.h"
#include "uv_errno.h"

using namespace DLNetwork;

UdpClient::UdpClient(EventThread* thread, INetAddress serverAddr, INetAddress localAddr, std::string name)
    : _thread(thread), _serverAddr(serverAddr), _localAddr(localAddr), _name(name) {
}

UdpClient::~UdpClient() {
    stop();
}

bool UdpClient::startConnect() {
    if (!_serverAddr.isValid()) {
        mCritical() << "UdpClient::startConnect invalid addr" << _name;
        return false;
    }

    _connection = UdpConnection::create(_thread, _serverAddr, _localAddr);
    if (!_connection) {
        mCritical() << "UdpClient::startConnect create connection failed" << _name;
        return false;
    }

    _connection->setConnectCallback(std::bind(&UdpClient::onConnectionChange, this, std::placeholders::_1, std::placeholders::_2));
    _connection->setOnMessage(std::bind(&UdpClient::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    _connection->startConnect();

    return true;
}

void UdpClient::stop() {
    if (_connection) {
        _connection->close();
        _connection.reset();
    }
}

void UdpClient::write(const char* buf, size_t size) {
    if (_connection) {
        _connection->write(buf, size);
    }
}

void UdpClient::onConnectionChange(Connection::Ptr conn, ConnectEvent e) {
    if (_connectCallback) {
        _connectCallback(*this, e);
    }
}

bool UdpClient::onMessage(Connection::Ptr conn, Buffer* buf) {
    if (_messageCallback) {
        return _messageCallback(*this, buf);
    }
    return true;
} 