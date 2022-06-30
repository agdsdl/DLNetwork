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
#include "TcpConnection.h"
#include "uv_errno.h"
#include "sockutil.h"
#include "MyLog.h"
#include <assert.h>
#include <condition_variable>
#include <sstream>
#include "sockutil.h"

using namespace DLNetwork;

DLNetwork::MyOut& operator<<(DLNetwork::MyOut& o, DLNetwork::TcpConnection& c) {
    o << c.sock() << c.selfAddress().description() << "--" << c.peerAddress().description();
    return o;
}

TcpConnection::TcpConnection(EventThread *thread, SOCKET sock):_thread(thread),_closing(false),_eventType(EventType::Read),_sock(sock)
{
}


TcpConnection::~TcpConnection()
{
    mDebug() << "TcpConnection::~TcpConnection()" << this->description().c_str();
    close();
}

std::unique_ptr<TcpConnection> TcpConnection::connectTo(EventThread* thread, INetAddress addr) {
    assert(thread->isCurrentThread());

    if (!addr.isIP4() && !addr.isIP6()) {
        mCritical() << "TcpConnection::connectTo badAddr!" << addr.description().c_str();
        return nullptr;
    }
    SOCKET fsock = socket(AF_INET, SOCK_STREAM, 0);
    if (fsock < 0) {
        mCritical() << "TcpConnection::connectTo init sock error" << get_uv_errmsg();
        return nullptr;
    }

    //sockaddr_in   localAddr;
    //memset(&localAddr, 0, sizeof(sockaddr_in));
    //localAddr.sin_family = AF_INET;
    //localAddr.sin_addr.s_addr = INADDR_ANY;
    //localAddr.sin_port = 1099;
    //int nret = ::bind(fsock, (sockaddr*)&localAddr, sizeof(localAddr));
    //if (nret < 0) {
    //    mCritical() << "TcpConnection::connectTo bind local port error" << get_uv_errmsg();
    //    myclose(fsock);
    //    return nullptr;
    //}

    int ecode = connect(fsock, (sockaddr*)&addr.addr4(), addr.isIP4()?sizeof(addr.addr4()):sizeof(addr.addr6()));
    if (ecode < 0) {
        mCritical() << "TcpConnection::connectTo connect" << addr.description().c_str() << "error" << get_uv_errmsg();
        return nullptr;
    }
    auto conn = std::make_unique<TcpConnection>(thread, fsock);
    conn->setPeerAddr(addr);
    conn->attach();
    return conn;
}

void TcpConnection::attach() {
    SockUtil::setNoBlocked(_sock, true);
    if (!_peerAddr.isIP4() && !_peerAddr.isIP6()) {
        _peerAddr = INetAddress::getPeerAddress(_sock);
    }
    _selfAddr = INetAddress::getSelfAddress(_sock);

    _thread->addEvent(_sock, _eventType, std::bind(&TcpConnection::onEvent, this, std::placeholders::_1, std::placeholders::_2));
}

void TcpConnection::write(const char * buf, size_t size)
{
    size_t remain = size;
    //if (_thread->isCurrentThread() && 
    //    _writeBuf.readableBytes() == 0) {
    //    int n = ::send(_sock, buf, size, 0);
    //    if (n >= 0) {
    //        remain -= n;

    //    }
    //    else {
    //        int ecode = get_uv_error();
    //        if (ecode == UV_EAGAIN) {
    //            // write to buf.
    //        }
    //        else {
    //            mWarning() << "TcpConnection::write error:" << get_uv_errmsg();
    //            close();
    //            return;
    //        }
    //    }
    //}

    if (remain) {
        std::lock_guard<std::mutex> lock(_writeBufMutex);
        _writeBuf.append(buf, size);
        _eventType |= EventType::Write;
        _thread->modifyEvent(_sock, _eventType);
    }
}

void TcpConnection::writeInThread(const char * buf, size_t size)
{
}

void TcpConnection::close()
{
    assert(_thread->isCurrentThread());
    //mDebug() << "TcpConnection::close()" << this->description().c_str();
    if (_closing) {
        return;
    }
    _closing = true;
    _thread->removeEvents(_sock);
    //if (_thread->isCurrentThread()) {
    //    _thread->removeEvents(_sock);
    //}
    //else {
    //    bool done = false;;
    //    std::mutex closeMutex;
    //    std::condition_variable cvClose;
    //    std::unique_lock<std::mutex> lk(closeMutex);
    //    _thread->dispatch([this, &done, &cvClose, &closeMutex]() {
    //        _thread->removeEvents(_sock);
    //        {
    //            std::lock_guard<std::mutex> lk2(closeMutex);
    //            done = true;
    //        }
    //        cvClose.notify_one();
    //        }, true);
    //    cvClose.wait(lk, [&done]() {return done; });
    //}
    SOCKET sock = _sock;
    if (_connectionCb) {
        _connectionCb(*this, ConnectEvent::Closed);
    }
    myclose(sock); // this may be deleted after cb.
}

void TcpConnection::reset() {
    assert(_thread->isCurrentThread());
    mDebug() << "TcpConnection::reset()" << this->description().c_str();
    if (_closing) {
        return;
    }
    _closing = true;
    _thread->removeEvents(_sock);
    SOCKET sock = _sock;
    if (_connectionCb) {
        _connectionCb(*this, ConnectEvent::Closed);
    }
    struct linger sl;
    sl.l_onoff = 1;		/* non-zero value enables linger option in kernel */
    sl.l_linger = 0;	/* timeout interval in seconds */
    setsockopt(sock, SOL_SOCKET, SO_LINGER, (char*)&sl, sizeof(sl));
    myclose(sock); // this may be deleted after cb.

}

std::string TcpConnection::description() {
    std::stringstream o;
    o << sock() << " " << selfAddress().description() << "--" << peerAddress().description();

    return o.str();;

}

void TcpConnection::closeAfterWrite() {
    std::unique_lock<std::mutex> lock(_writeBufMutex);
    if (_writeBuf.readableBytes() == 0) {
        close();
    }
    else {
        _closeAfterWrite = true;
    }
}

void TcpConnection::onEvent(SOCKET sock, int eventType) {
    if (eventType & EventType::Write) {
        if (!handleWrite(sock)) {
            return;
        }
    }
    if (eventType & EventType::Read) {
        if (!handleRead(sock)) {
            return;
        }
    }
    if (eventType & EventType::Hangup) {
        handleHangup(sock);
        return;
    }
    if (eventType & EventType::Error) {
        handleError(sock);
    }
}

bool TcpConnection::handleRead(SOCKET sock)
{
    int ecode = 0;
    int32_t n = _readBuf.readFd(sock, &ecode);
    if (n > 0) {
        return _messageCb(*this, &_readBuf);
    }
    else if (n == 0) {
        close();
        return false;
    }
    else {
        mWarning() << "TcpConnection read error" << ecode << get_uv_errmsg();
        close();
        return false;
    }
}

bool TcpConnection::handleWrite(SOCKET sock)
{
    std::unique_lock<std::mutex> lock(_writeBufMutex);

    int n = ::send(sock, _writeBuf.peek(), _writeBuf.readableBytes(), 0);
    if (n >= 0) {
        _writeBuf.retrieve(n);
        if (_writeBuf.readableBytes() == 0) {
            _eventType &= ~EventType::Write;
            _thread->modifyEvent(_sock, _eventType);
            if (_writedcb) {
                _writedcb(*this);
                //_thread->dispatch([this]() {
                //});
            }
            if (_closeAfterWrite) {
                lock.unlock();
                close();
                return false;
            }
        }
        return true;
    }
    else {
        mWarning() << "TcpConnection::handleWrite error:" << get_uv_errmsg();
        lock.unlock();
        close();
        return false;
    }
}

void TcpConnection::handleHangup(SOCKET sock)
{
    mWarning() << "TcpConnection handleHangup" << sock;
    close();
}

void TcpConnection::handleError(SOCKET sock)
{
    int ecode = SockUtil::getSockError(sock);
    const char *eMsg = uv_strerror(ecode);
    mWarning() << "TcpConnection handleError:" << sock << ecode << eMsg;
    close();
}
