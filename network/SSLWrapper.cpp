#include "SSLWrapper.h"
#include "MyLog.h"

namespace DLNetwork {

#ifdef ENABLE_OPENSSL
using AlpnProtos = std::vector<uint8_t>;

const AlpnProtos alpnProtos{
    2, 'h', '2',
    8, 'h', 't', 't', 'p', '/', '1', '.', '1'
};

static int alpnCallback(SSL* ssl, const unsigned char** out, unsigned char* outlen, 
                       const unsigned char* _in, unsigned int inlen, void* arg) {
    const AlpnProtos* protos = (AlpnProtos*)arg;
    if (SSL_select_next_proto((unsigned char**)out, outlen, 
        &(*protos)[0], (unsigned int)protos->size(), _in, inlen) != OPENSSL_NPN_NEGOTIATED) {
        return SSL_TLSEXT_ERR_NOACK;
    }
    return SSL_TLSEXT_ERR_OK;
}
#endif

void SSLWrapper::globalInit() {
#ifdef ENABLE_OPENSSL
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();
#endif
}

void SSLWrapper::globalFini() {
#ifdef ENABLE_OPENSSL
    ERR_free_strings();
    EVP_cleanup();
#endif
}

SSLWrapper::SSLWrapper() {
}

SSLWrapper::~SSLWrapper() {
    shutdown();
}

bool SSLWrapper::init(const std::string& certFile, const std::string& keyFile, bool supportH2) {
#ifdef ENABLE_OPENSSL
    _mode = Mode::Server;
    _ctx = SSL_CTX_new(TLS_server_method());
    if (!_ctx) {
        mCritical() << "SSL_CTX_new failed";
        return false;
    }

    if (SSL_CTX_use_certificate_file(_ctx, certFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
        mCritical() << "SSL_CTX_use_certificate_file failed";
        return false;
    }

    if (SSL_CTX_use_PrivateKey_file(_ctx, keyFile.c_str(), SSL_FILETYPE_PEM) <= 0) {
        mCritical() << "SSL_CTX_use_PrivateKey_file failed";
        return false;
    }

    if (!SSL_CTX_check_private_key(_ctx)) {
        mCritical() << "SSL_CTX_check_private_key failed";
        return false;
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
    SSL_set_accept_state(_ssl);
    
    return true;
#else
    return false;
#endif
}

bool SSLWrapper::initAsClient(const std::string& hostName, const std::string& certFile) {
#ifdef ENABLE_OPENSSL
    _mode = Mode::Client;
    _ctx = SSL_CTX_new(TLS_client_method());
    if (!_ctx) {
        mCritical() << "SSL_CTX_new failed";
        return false;
    }

    // 设置客户端选项
    SSL_CTX_set_verify(_ctx, SSL_VERIFY_NONE, NULL);
    SSL_CTX_set_min_proto_version(_ctx, TLS1_2_VERSION);
    SSL_CTX_set_max_proto_version(_ctx, TLS1_3_VERSION);
    SSL_CTX_set_options(_ctx, SSL_OP_NO_RENEGOTIATION);

    if (!certFile.empty()) {
        SSL_CTX_load_verify_locations(_ctx, certFile.c_str(), NULL);
    }

    _ssl = SSL_new(_ctx);
    if (!_ssl) {
        mCritical() << "SSL_new failed";
        return false;
    }

    // 设置SNI
    if (!hostName.empty()) {
        //if (!SSL_set_tlsext_host_name(_ssl, hostName.c_str())) {
        //    mWarning() << "Failed to set SNI hostname";
        //    ERR_print_errors_fp(stderr);
        //} else {
        //    mInfo() << "Set SNI hostname:" << hostName;
        //}
    }

    _rbio = BIO_new(BIO_s_mem());
    _wbio = BIO_new(BIO_s_mem());
    if (!_rbio || !_wbio) {
        mCritical() << "BIO_new failed";
        return false;
    }
    
    SSL_set_bio(_ssl, _rbio, _wbio);
    SSL_set_connect_state(_ssl);  // 客户端使用connect state
    
    // 初始化完成后立即开始握手
    // doHandshake();
    // handleOut();  // 确保发送第一个Client Hello
    
    return true;
#else
    return false;
#endif
}

int SSLWrapper::onRecvEncrypted(const char* data, size_t len) {
#ifdef ENABLE_OPENSSL
    int needRead = len;
    do
    {
        // 将收到的加密数据写入BIO
        int w = BIO_write(_rbio, data, needRead);
        if (w <= 0) {
            mWarning() << "BIO_write failed";
            return w;
        }
        else {
            data += w;
            needRead -= w;
        }

        if (!SSL_is_init_finished(_ssl)) {
            doHandshake();
        }

        if (_handshakeFinished) {
            // 读取解密后的数据
            char buf[4096];
            int ret;
            do {
                ret = SSL_read(_ssl, buf, sizeof(buf));
                if (ret > 0) {
                    if (_onDecryptedData) {
                        _onDecryptedData(buf, ret);
                    }
                } else {
                    handleSSLError(ret);
                    break;
                }
            } while (ret > 0);
        }
    } while (needRead > 0);
    
    handleOut();
    return len - needRead;
#endif
}

int SSLWrapper::sendUnencrypted(const char* data, size_t len) {
#ifdef ENABLE_OPENSSL
    if (!_handshakeFinished) {
        // 握手未完成，将数据存入缓存
        _sendBuffer.push(std::string(data, len));
        mWarning() << "SSL handshake not finished, data cached";
        doHandshake();
        return len;  // 返回完整长度表示数据已被接受（虽然是缓存）
    }

    // 原有的发送逻辑
    int needWrite = len;
    do {
        int written = SSL_write(_ssl, data, needWrite);
        if (written <= 0) {
            handleSSLError(written);
        }
        else {
            data += written;
            needWrite -= written;
        }
    } while (needWrite > 0);
    
    handleOut();
    return len - needWrite;
#endif
}

bool SSLWrapper::doHandshake() {
#ifdef ENABLE_OPENSSL
    if (_handshakeFinished) {
        return true;
    }

    int ret = SSL_do_handshake(_ssl);
    if (ret < 1) {
        handleSSLError(ret);
        // 客户端需要主动发起握手
        if (_mode == Mode::Client) {
            handleOut();
        }
        return false;
    }
    else {
        mInfo() << "SSL handshake finished";
        _handshakeFinished = true;
        
        // 握手完成后，发送缓存的数据
        while (!_sendBuffer.empty()) {
            const std::string& data = _sendBuffer.front();
            sendUnencrypted(data.c_str(), data.length());
            _sendBuffer.pop();
        }

        // 客户端验证服务器证书
        if (_mode == Mode::Client) {
            X509* cert = SSL_get_peer_certificate(_ssl);
            if (cert) {
                X509_free(cert);
            } else {
                mWarning() << "No server certificate";
            }
        }
        return true;
    }
#else
    return false;
#endif
}

bool SSLWrapper::isHandshakeFinished() const {
    return _handshakeFinished;
}

bool SSLWrapper::handleSSLError(int ret) {
#ifdef ENABLE_OPENSSL
    int err = SSL_get_error(_ssl, ret);
    switch (err) {
        case SSL_ERROR_WANT_READ:
            handleOut();
            return false;
        case SSL_ERROR_WANT_WRITE:
            mWarning() << "SSL_ERROR_WANT_WRITE";
            return true;
        case SSL_ERROR_ZERO_RETURN:
            mWarning() << "SSL connection closed";
            return false;
        default:
            mWarning() << "SSL error:" << err;
            ERR_print_errors_fp(stderr);
            return false;
    }
#else
    return false;
#endif
}

void SSLWrapper::handleOut() {
#ifdef ENABLE_OPENSSL
    char buf[4096];
    int read;
    while ((read = BIO_read(_wbio, buf, sizeof(buf))) > 0) {
        if (_onEncryptedData) {
            _onEncryptedData(buf, read);
        }
    }
#endif
}

void SSLWrapper::shutdown() {
#ifdef ENABLE_OPENSSL
    if (_ssl) {
        SSL_shutdown(_ssl);
        SSL_free(_ssl);
        _ssl = nullptr;
    }
    if (_ctx) {
        SSL_CTX_free(_ctx);
        _ctx = nullptr;
    }
#endif
}

bool SSLWrapper::startHandshake() {
    if (_mode != Mode::Client || _handshakeFinished || SSL_is_init_finished(_ssl)) {
        return false;
    }
    return doHandshake();
}

} // namespace DLNetwork 