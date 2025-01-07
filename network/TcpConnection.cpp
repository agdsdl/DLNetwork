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
#include "cppdefer.h"
#include "StringUtil.h"
#include "SSLWrapper.h"

using namespace DLNetwork;

DLNetwork::MyOut& operator<<(DLNetwork::MyOut& o, DLNetwork::TcpConnection& c) {
    o << c.sock() << c.selfAddress().description() << "--" << c.peerAddress().description();
    return o;
}

#ifdef ENABLE_OPENSSL
using AlpnProtos = std::vector<uint8_t>;

const AlpnProtos alpnProtos{
    2, 'h', '2',
    8, 'h', 't', 't', 'p', '/', '1', '.', '1'
};

static int alpnCallback(SSL* ssl, const unsigned char** out, unsigned char* outlen, const unsigned char* _in, unsigned int inlen, void* arg)
{
    const AlpnProtos* protos = (AlpnProtos*)arg;

    if (SSL_select_next_proto((unsigned char**)out, outlen, &(*protos)[0], (unsigned int)protos->size(), _in, inlen) != OPENSSL_NPN_NEGOTIATED) {
        return SSL_TLSEXT_ERR_NOACK;
    }
    return SSL_TLSEXT_ERR_OK;
}
#endif // ENABLE_OPENSSL

TcpConnection::TcpConnection(EventThread *thread, SOCKET sock):_thread(thread),_closing(false),_eventType(EventType::Read),_sock(sock)
{
}


TcpConnection::~TcpConnection()
{
    mDebug() << "TcpConnection::~TcpConnection()" << ptr2string(this) << this->description().c_str();
    close();
}

#ifdef ENABLE_OPENSSL
void DLNetwork::TcpConnection::enableTls(std::string certFile, std::string keyFile, bool supportH2) {
    _ssl = std::make_unique<SSLWrapper>();
    bool ret;
    if (_clientMode) {
        ret = _ssl->initAsClient();
    } else {
        ret = _ssl->init(certFile, keyFile, supportH2);
    }
    if (!ret) {
        mCritical() << "Failed to initialize SSL";
        _ssl.reset();
        return;
    }

    _ssl->setOnEncData([this](const char* data, size_t len) {
        mDebug() << "TcpConnection::enableTls OnEncData" << len;
        DLNetwork::Buffer newBuf;
        newBuf.append(data, len);
        std::lock_guard<std::mutex> lock(_writeBufMutex);
        _writeBuf.push_back(std::move(newBuf));
        _eventType |= EventType::Write;
        _thread->modifyEvent(_sock, _eventType);
    });

    _ssl->setOnDecData([this](const char* data, size_t len) {
        mDebug() << "TcpConnection::enableTls OnDecData" << len;
        if (_messageCb) {
            Buffer buf;
            buf.append(data, len);
            _messageCb(shared_from_this(), &buf);
        }
    });

    if (_clientMode) {
        _ssl->startHandshake();
    }
    
}
#endif // ENABLE_OPENSSL

TcpConnection::Ptr TcpConnection::createClient(EventThread* thread, INetAddress addr) {
    if (!addr.isIP4() && !addr.isIP6()) {
        mCritical() << "TcpConnection::connectTo badAddr!" << addr.description().c_str();
        return nullptr;
    }
    SOCKET sock = socket(addr.isIP6() ? AF_INET6 : AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        mCritical() << "TcpConnection::connectTo create socket failed:" << get_uv_errmsg();
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

    auto conn = create(thread, sock);
    conn->setPeerAddr(addr);
    conn->attach();
    conn->_clientMode = true;
    return conn;
}

void TcpConnection::startConnect() {
    int ecode = connect(_sock, (sockaddr*)&_peerAddr.addr4(), _peerAddr.isIP4()?sizeof(_peerAddr.addr4()):sizeof(_peerAddr.addr6()));
    if (ecode < 0) {
        mCritical() << "TcpConnection::connectTo connect" << _peerAddr.description().c_str() << "error" << get_uv_errmsg();
        return;
    }
    _eventType = EventType::Write;
    attach();
}

void TcpConnection::attach() {
    SockUtil::setNoBlocked(_sock, true);
    if (!_peerAddr.isIP4() && !_peerAddr.isIP6()) {
        _peerAddr = INetAddress::getPeerAddress(_sock);
    }
    _selfAddr = INetAddress::getSelfAddress(_sock);

    _thread->addEvent(_sock, _eventType, std::bind(&TcpConnection::onEvent, this, std::placeholders::_1, std::placeholders::_2));
}

void TcpConnection::writeInner(const char* buf, size_t size)
{
    if (size) {
        if (_ssl) {
            _ssl->sendUnencrypted(buf, size);
        } else {
            DLNetwork::Buffer newBuf;
            newBuf.append(buf, size);
            
            {
                std::lock_guard<std::mutex> lock(_writeBufMutex);
                _writeBuf.push_back(std::move(newBuf));
            }
            
            _eventType |= EventType::Write;
            _thread->modifyEvent(_sock, _eventType);
        }
    }
}

void TcpConnection::write(const char * buf, size_t size)
{
    if (_closing) {
        mWarning() << "TcpConnection::write when closing" << size;
        return;
    }
    writeInner(buf, size);
}

void TcpConnection::writeInThread(const char * buf, size_t size)
{
}

void TcpConnection::close()
{
    //assert(_thread->isCurrentThread());
    if (_closing) {
        return;
    }
    _closing = true;
    //mDebug() << "TcpConnection::close()" << this->description().c_str();
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
        _connectionCb(shared_from_this(), ConnectEvent::Closed);
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
        _connectionCb(shared_from_this(), ConnectEvent::Closed);
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
    if (_writeBuf.size() == 0) {
        lock.unlock();
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
    if (_closing) {
        return false;
    }

    int ecode = 0;
    int32_t n = _readBuf.readFd(sock, &ecode);
    if (n > 0) {
        if (_ssl) {
            int ret = _ssl->onRecvEncrypted(_readBuf.peek(), _readBuf.readableBytes());
            if (ret > 0) {
                _readBuf.retrieve(ret);
                return true;
            }
            else {
                mWarning() << "TcpConnection::handleRead SSL error:" << ret;
                close();
                return false;
            }
        } else {
            if (_messageCb) {
                return _messageCb(shared_from_this(), &_readBuf);
            }
        }
        return false;
    }
    else if (n == 0) {
        close();
        return false;
    }
    else {
        mWarning() << "TcpConnection read" << sock << "error:" << ecode << get_uv_errmsg();
        close();
        return false;
    }
}

bool TcpConnection::handleWrite(SOCKET sock)
{
    if (_closing) {
        return false;
    }
    if (!_connected) {
        assert(_clientMode);
        _connected = true;
        _eventType = EventType::Read;
        _thread->modifyEvent(_sock, _eventType);
        if (_connectionCb) {
            _connectionCb(shared_from_this(), ConnectEvent::Established);
        }
    }

    return realSend();
}

bool DLNetwork::TcpConnection::realSend()
{
    decltype(_writeBuf) writeBufTmp;
    {
        std::lock_guard<std::mutex> lock(_writeBufMutex);
        if (!_writeBuf.empty()) {
            writeBufTmp.swap(_writeBuf);
        }
    }

    while (!writeBufTmp.empty()) {
        auto &buf = writeBufTmp.front();
        // 发送数据（在锁外进行）
        int n = ::send(_sock, buf.peek(), buf.readableBytes(), 0);
        if (n >= 0) {
            if (n == buf.readableBytes()) {
                writeBufTmp.pop_front();
            } else {
                buf.retrieve(n);
                break;
            }
        } else {
            mWarning() << "TcpConnection::handleWrite error:" << get_uv_errmsg();
            close();
            return false;
        }
    }
    // 回滚未发送完毕的数据
    if (!writeBufTmp.empty()) {
        // 有剩余数据
        std::lock_guard<std::mutex> lock(_writeBufMutex);
        writeBufTmp.swap(_writeBuf);
        _writeBuf.insert(_writeBuf.end(), writeBufTmp.begin(), writeBufTmp.end());
    }
    else{
        _eventType &= ~EventType::Write;
        _thread->modifyEvent(_sock, _eventType);
        
        if (_writedcb) {
            _writedcb(shared_from_this());
        }
        
        if (_closeAfterWrite) {
            close();
            return false;
        }
    }
    return true;
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
