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
    mDebug() << "TcpConnection::~TcpConnection()" << this << this->description().c_str();
    close();
}
#ifdef ENABLE_OPENSSL
void DLNetwork::TcpConnection::enableTls(std::string certFile, std::string keyFile, bool supportH2) {
    _enableTls = true;
    _certFile = certFile;
    _keyFile = keyFile;

    /* 以SSL V2 和V3 标准兼容方式产生一个SSL_CTX ，即SSL Content Text */
    _ctx = SSL_CTX_new(TLS_server_method());
    /*
    也可以用SSLv2_server_method() 或SSLv3_server_method() 单独表示V2 或V3标准
    */
    if (_ctx == NULL) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    /* 载入用户的数字证书， 此证书用来发送给客户端。证书里包含有公钥*/
    if (SSL_CTX_use_certificate_file(_ctx, _certFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    /* 载入用户私钥*/
    if (SSL_CTX_use_PrivateKey_file(_ctx, _keyFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }
    /* 检查用户私钥是否正确*/
    if (!SSL_CTX_check_private_key(_ctx)) {
        ERR_print_errors_fp(stdout);
        exit(1);
    }

    SSL_CTX_set_verify(_ctx, SSL_VERIFY_NONE, NULL);
    SSL_CTX_set_options(_ctx, SSL_OP_NO_RENEGOTIATION);
    if (supportH2) {
        SSL_CTX_set_alpn_select_cb(_ctx, alpnCallback, (void*)&alpnProtos);
    }

    _ssl = SSL_new(_ctx);
    _rbio = BIO_new(BIO_s_mem());
    _wbio = BIO_new(BIO_s_mem());

    SSL_set_bio(_ssl, _rbio, _wbio);
    SSL_set_accept_state(_ssl); // The server uses SSL_set_accept_state
    //size_t pendingW = BIO_ctrl_pending(_wbio);
    //size_t pendingR = BIO_ctrl_pending(_rbio);
}
#endif // ENABLE_OPENSSL

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

void TcpConnection::writeInner(const char* buf, size_t size)
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

void TcpConnection::write(const char * buf, size_t size)
{
#ifdef ENABLE_OPENSSL
    if (_enableTls) {
        do {
            int r = SSL_write(_ssl, buf, size);
            if (r <= 0) {
                int ecode = SSL_get_error(_ssl, r);
                if (ecode == SSL_ERROR_WANT_READ) {
                    mWarning() << "SSL_ERROR_WANT_READ when send";
                    handleSSLRead();
                }
                else if (ecode == SSL_ERROR_WANT_WRITE) {
                    mWarning() << "SSL_ERROR_WANT_WRITE when send";
                }
                else {
                    mWarning() << "SSL_write error:" << ecode;
                    ERR_print_errors_fp(stdout);
                }
            }
            else {
                break;
            }
        } while (true);
        handleSSLRead();
    }
    else {
        writeInner(buf, size);
    }
#else
    writeInner(buf, size);
#endif // ENABLE_OPENSSL
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
#ifdef ENABLE_OPENSSL
    if (_enableTls) {
        if (_ssl) {
            SSL_shutdown(_ssl);
            SSL_free(_ssl);
            _ssl = nullptr;
        }
        if (_ctx) {
            SSL_CTX_free(_ctx);
            _ctx = nullptr;
        }
    }
#endif //ENABLE_OPENSSL
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
#ifdef ENABLE_OPENSSL
        if (_enableTls) {
            // Here we get the data from the client suppose it's in the variable buf
// and write it to the connection reader BIO.
        //std::string inbuf;
        //先把所有数据都取出来
        //_readTlsBuf.append(_readBuf.peek(), _readBuf.readableBytes());
            DEFER(_readBuf.retrieveAll(););

            int w = BIO_write(_rbio, _readBuf.peek(), _readBuf.readableBytes());
            assert(w == _readBuf.readableBytes());
            //size_t pendingW = BIO_ctrl_pending(_wbio);
            //size_t pendingR = BIO_ctrl_pending(_rbio);
            //mInfo() << "onmsg" << description() << "pendingW:" << pendingW << " pendingR:" << pendingR;
            //ERR_print_errors_fp(stdout);

            if (!SSL_is_init_finished(_ssl)) {
                int r = SSL_do_handshake(_ssl);
                if (r < 1) {
                    int ecode = SSL_get_error(_ssl, r);
                    if (ecode == SSL_ERROR_WANT_READ) {
                        //mWarning() << "SSL_ERROR_WANT_READ when SSL_do_handshake";
                        handleSSLRead();
                    }
                    else if (ecode == SSL_ERROR_WANT_WRITE) {
                        //mWarning() << "SSL_ERROR_WANT_WRITE when SSL_do_handshake";
                    }
                    else {
                        mWarning() << "SSL_do_handshake error:" << ecode;
                        ERR_print_errors_fp(stdout);
                    }

                    return true;
                }
                else {
                    mInfo() << "SSL handshake finished";
                }
            }
            else {
                //mInfo() << "SSL handshake finished2";
            }
            char buf2[4096];

            int ret = 0;
            do {
                ret = SSL_read(_ssl, buf2, sizeof(buf2));
                if (ret <= 0) {
                    int err = SSL_get_error(_ssl, ret);
                    if (err == SSL_ERROR_WANT_READ) {
                        //mWarning() << "SSL_ERROR_WANT_READ when SSL_read";
                        handleSSLRead();
                    }
                    else if (err == SSL_ERROR_WANT_WRITE) {
                        //mWarning() << "SSL_ERROR_WANT_WRITE when SSL_read";
                    }
                    break;
                }
                else {
                    _readTlsBuf.append(buf2, ret);
                }
            } while (ret > 0);

            return _messageCb(*this, &_readTlsBuf);
        }
        else {
            return _messageCb(*this, &_readBuf);
        }
#else
        return _messageCb(*this, &_readBuf);
#endif // ENABLE_OPENSSL
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

#ifdef ENABLE_OPENSSL
void TcpConnection::handleSSLRead()
{
    int read = 0;
    do {
        char buf[4096];
        read = BIO_read(_wbio, buf, sizeof(buf));
        if (read > 0) {
            writeInner(buf, read);
        }
        else {
            //TODO: BIO_should_retry
        }
    } while (read > 0);
    //do {
    //    size_t pendingW = BIO_ctrl_pending(_wbio);
    //    size_t pendingR = BIO_ctrl_pending(_rbio);
    //    mInfo() << description() << "pendingW:" << pendingW << " pendingR:" << pendingR;
    //    if (pendingW > 0) {
    //        char* buf = new char[pendingW];
    //        int read = BIO_read(_wbio, buf, pendingW);
    //        assert(read == pendingW);
    //        writeInner(buf, pendingW);
    //        delete[] buf;
    //    }
    //} while (read != pendingW);
}
#endif // ENABLE_OPENSSL