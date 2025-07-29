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
#include "TcpClient.h"
#include "uv_errno.h"
#include "sockutil.h"
#include <sstream>
#include <assert.h>

using namespace DLNetwork;

DLNetwork::MyOut& operator<<(DLNetwork::MyOut& o, DLNetwork::TcpClient& c) {
    o << c.description();
    return o;
}

std::string TcpClient::description() {
    std::stringstream o;
    o << "cli " << sock();
    if (connection()) {
        o << " to " << connection()->peerAddress().description();
    }
    else {
        o << " noConn";
    }
    o << " "<< name();
    return o.str();;
}
TcpClient::TcpClient(EventThread* loop, const INetAddress& serverAddr, const INetAddress& localAddr, const std::string& name, bool enableTls):
	_serverAddr(serverAddr), _localAddr(localAddr), _thread(loop), _name(name), _enableTls(enableTls)
{
}

TcpClient::~TcpClient() {
    mDebug() << "TcpClient::~TcpClient" << description();
    close();
}

bool TcpClient::startConnect() {
    if (!_serverAddr.isIP4() && !_serverAddr.isIP6()) {
        mCritical() << "TcpClient::startConnect badAddr!" << _name << _serverAddr.description();
        return false;
    }
    SOCKET fsock = socket(AF_INET, SOCK_STREAM, 0);
    if (fsock == -1) {
        mCritical() << "TcpClient::startConnect init sock error" << _name << get_uv_errmsg();
        return false;
    }
    _sock = fsock;
    SockUtil::setNoBlocked(fsock, true);
    if(_localAddr.isValid()) {
        int nret = 0;
        SockUtil::setReuseable(fsock, true);
        if(_localAddr.isIP4()) {
            nret = ::bind(fsock, (sockaddr*)&_localAddr.addr4(), sizeof(_localAddr.addr4()));
        }
        else if(_localAddr.isIP6()) {
            nret = ::bind(fsock, (sockaddr*)&_localAddr.addr6(), sizeof(_localAddr.addr6()));
        }
        if (nret < 0) {
            mCritical() << "TcpClient::startConnect bind local port error" << get_uv_errmsg();
            myclose(fsock);
            return false;
        }
    }

    int ecode = connect(fsock, (sockaddr*)&_serverAddr.addr4(), _serverAddr.isIP4() ? sizeof(_serverAddr.addr4()) : sizeof(_serverAddr.addr6()));
    int uvErr = 0;
    if (ecode < 0) {
        uvErr = get_uv_error();
    }

    if (ecode == 0 || uvErr == UV_EAGAIN) {
        _state = State::connecting;
        //mInfo() << "TcpClient::startConnect" << "ecode" << ecode << fsock << _name << _serverAddr.description();
        _thread->addEvent(fsock, EventType::Write, std::bind(&TcpClient::onEvent, this, std::placeholders::_1, std::placeholders::_2));
        return true;
    }
    else {
        mCritical() << "TcpClient::startConnect" << _name<< _serverAddr.description() << "error" << get_uv_errmsg();
        return false;
    }
}

void TcpClient::write(const char* buf, size_t size) {
    if (_state != State::connected) {
        mCritical() << "TcpClient::write state is not connected!";
        return;
    }

    _conn->write(buf, size);
}

void TcpClient::close() {
    setConnectCallback(nullptr);
    setOnMessage(nullptr);
    setOnWriteDone(nullptr);
    setOnConnectError(nullptr);
    if (_conn) {
        _conn->close();
    }
    else {
        if (_sock != -1) {
            _thread->removeEvents(_sock);
            myclose(_sock);
            _sock = -1;
        }
    }
}

void TcpClient::connectionCallback(TcpConnection::Ptr conn, ConnectEvent e) {
    if (_connectionCb) {
        _connectionCb(*this, e);
    }
}

bool TcpClient::messageCallback(TcpConnection::Ptr conn, Buffer* buf) {
    if (_messageCb) {
        return _messageCb(*this, buf);
    }
    return true;
}

void TcpClient::writedCallback(TcpConnection::Ptr conn) {
    if (_writedCb) {
        _writedCb(*this);
    }
}

void TcpClient::onEvent(SOCKET sock, int eventType) {
    if (eventType & EventType::Hangup) {
        handleHangup(sock);
        return;
    }
    if (eventType & EventType::Error) {
        handleError(sock);
        return;
    }
    if (eventType & EventType::Write) {
        handleWrite(sock);
    }
    if (eventType & EventType::Read) {
        handleRead(sock);
    }
}
void TcpClient::handleRead(SOCKET sock) {
    assert(0);
}

void TcpClient::handleWrite(SOCKET sock) {
    assert(_state == State::connecting);
    if (_state != State::connecting) {
        mCritical() << "TcpClient::handleWrite state is not connecting!";
        return;
    }
    _thread->removeEvents(sock);
    _conn = TcpConnection::create(_thread, sock);
    _conn->setConnectCallback(std::bind(&TcpClient::connectionCallback, this, std::placeholders::_1, std::placeholders::_2));
    _conn->setOnMessage(std::bind(&TcpClient::messageCallback, this, std::placeholders::_1, std::placeholders::_2));
    _conn->setOnWriteDone(std::bind(&TcpClient::writedCallback, this, std::placeholders::_1));
    _conn->attach();
#ifdef ENABLE_OPENSSL
    if (_enableTls) {
        _conn->enableTlsClient("", "");
    }
#endif
    if (_conn->isSelfConnection()) {
        mCritical() << "TcpClient self connection:" << *_conn.get();
        _conn.reset();
        _state = State::disconnected;
        if (_errorCb) {
            _errorCb(*this, sock, -999, "self connection");
        }
        _sock = -1;
        return;
    }

    _state = State::connected;
    if (_connectionCb) {
        _connectionCb(*this, ConnectEvent::Established);
    }
}

void TcpClient::handleHangup(SOCKET sock) {
    mCritical() << "TcpClient net hangup!" << sock;
    _thread->removeEvents(sock);
    myclose(sock);
    _sock = -1;
    _state = State::disconnected;
    if (_connectionCb) {
        _connectionCb(*this, ConnectEvent::Closed);
    }
}

void TcpClient::handleError(SOCKET sock) {
    int ecode = SockUtil::getSockError(sock);
    const char* eMsg = uv_strerror(ecode);
    mWarning() << "TcpClient net error:" << sock << ecode << eMsg;
    _thread->removeEvents(sock);
    myclose(sock);
    _sock = -1;
    _state = State::disconnected;
    if (_errorCb) {
        _errorCb(*this, sock, ecode, eMsg);
    }
}
