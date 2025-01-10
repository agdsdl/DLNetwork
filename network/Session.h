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
#include "Connection.h"
#include "MyLog.h"

namespace DLNetwork {

class Session : public std::enable_shared_from_this<Session>
{
public:
    using Ptr = std::shared_ptr<Session>;
    Session(EventThread* thread){_thread = thread;}
    virtual ~Session(){
        if (_conn) {
            mInfo() << "Session::~Session" << this << *_conn;
        }
        else {
            mInfo() << "Session::~Session" << this;
        }
    }

    EventThread* thread() { return _thread; }
    // 设置证书相关方法
    void setSSLCert(const std::string& certFile, const std::string& keyfile, bool supportH2) {
        _certFile = certFile;
        _keyFile = keyfile;
        _enableTls = true;
        _supportH2 = supportH2;
    }

    void destroy() {
        if (_conn) {
            _conn->close(false);
            _conn = nullptr;
        }
    }

    int sendSize() {
        return _sendSize;
    }
    int recvSize() {
        return _recvSize;
    }
    int lastActiveTime() {
        return _lastActiveTime;
    }

    // 下面需子类实现
    virtual void onClosed() = 0;
    // 返回true表示消息正常处理，返回false表示消息异常，连接需关闭，无需后续处理
    virtual bool onMessage(DLNetwork::Buffer* buf) = 0;
    virtual void onWriteDone() = 0;

    /**
     * 连接成功后每2秒触发一次该事件
     */
    virtual void onManager() {}
    virtual void send(const char* buf, size_t size){
        if (_conn) {
            _conn->write(buf, size);
            _sendSize += size;
            _lastActiveTime = time(nullptr);
        }
    }

protected:
    void onConnectionChange(Connection::Ptr conn, ConnectEvent e){
        if (e == ConnectEvent::Closed) {
            //mDebug() << "onConnectionChange Closed" << *this;
            onClosed();
        }
    }
    bool onConnMessage(Connection::Ptr conn, DLNetwork::Buffer* buf){
        int size = buf->readableBytes();
        bool ret = onMessage(buf);
        _recvSize += size - buf->readableBytes();
        _lastActiveTime = time(nullptr);
        return ret;
    }
    void onConnWriteDone(Connection::Ptr conn){
        onWriteDone();
    }

    virtual void takeoverConn(Connection::Ptr conn){
        _conn = conn;
        // 这里不能设置回调，因为TcpServer已经设置过了
        //_conn->setConnectCallback(std::bind(&Session::onConnectionChange, this, std::placeholders::_1, std::placeholders::_2));
        _conn->setOnMessage(std::bind(&Session::onConnMessage, this, std::placeholders::_1, std::placeholders::_2));
        _conn->setOnWriteDone(std::bind(&Session::onConnWriteDone, this, std::placeholders::_1));
        _conn->attach();
        if (_enableTls) {
            _conn->enableTls(_certFile, _keyFile, _supportH2);
        }
    }

    Connection::Ptr _conn;
    EventThread* _thread;
    std::string _certFile;
    std::string _keyFile;
    bool _enableTls = false;
    bool _supportH2 = false;
    int _sendSize = 0;
    int _recvSize = 0;
    int _lastActiveTime = 0;

    friend class Server;
    friend class TcpServer;
    friend class UdpServer;

};

typedef std::function<Session::Ptr()> SessionCreator;

} // DLNetwork

DLNetwork::MyOut& operator<<(DLNetwork::MyOut& o, DLNetwork::Session& s);
