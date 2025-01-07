#pragma once

#include <string>
#include <functional>
#include "Buffer.h"

#ifdef ENABLE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#endif

namespace DLNetwork {

class SSLWrapper {
public:
    SSLWrapper();
    ~SSLWrapper();

    static void globalInit();
    static void globalFini();

    enum class Mode {
        Server,
        Client
    };

    bool init(const std::string& certFile, const std::string& keyFile, bool supportH2 = true);
    bool initAsClient(const std::string& hostName = "");

    /**
     * 收到密文后，调用此函数解密
     * @param buffer 收到的密文数据
     */
    int onRecvEncrypted(const char* data, size_t len);

    /**
     * 需要加密明文调用此函数
     * @param buffer 需要加密的明文数据
     */
    int sendUnencrypted(const char* data, size_t len);

    /**
     * 设置解密后获取明文的回调
     * @param cb 回调对象
     */
    void setOnDecData(const std::function<void(const char* data, size_t len)> &cb){
        _onDecryptedData = cb;
    }

    /**
     * 设置加密后获取密文的回调
     * @param cb 回调对象
     */
    void setOnEncData(const std::function<void(const char* data, size_t len)> &cb){
        _onEncryptedData = cb;
    }

    // 客户端主动发起握手
    bool startHandshake() {
        if (_mode != Mode::Client || _handshakeFinished) {
            return false;
        }
        return doHandshake();
    }

    void shutdown();
private:
    bool doHandshake();    
    bool isHandshakeFinished() const;

    bool handleSSLError(int ret);
    void handleOut();

    Mode _mode = Mode::Server;
#ifdef ENABLE_OPENSSL
    SSL_CTX* _ctx = nullptr;
    SSL* _ssl = nullptr;
    BIO* _rbio = nullptr;
    BIO* _wbio = nullptr;
#endif

    Buffer _encryptedBuf;  // 加密后的数据
    Buffer _decryptedBuf;  // 解密后的数据
    bool _handshakeFinished = false;
    std::function<void(const char* data, size_t len)> _onEncryptedData;
    std::function<void(const char* data, size_t len)> _onDecryptedData;
};

} // namespace DLNetwork 