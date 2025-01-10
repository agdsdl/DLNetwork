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
#include <memory>
#include <map>
#include <unordered_map>
#include <set>
#include "TcpConnection.h"
#include "HttpSession.h"
#include "Session.h"

namespace DLNetwork {
template<typename T>
class _MyHttpServer;

class MyHttpSession : public Session {
public:
    friend class _MyHttpServer<MyHttpSession>;

    typedef MyHttpSession CallbackSession;
    enum { supportH2 = false };

    typedef std::function<void(HTTP::Request& request, std::shared_ptr<CallbackSession> sess)> UrlHandler;
    typedef std::function<void(std::shared_ptr<MyHttpSession> sess)> ClosedHandler;
    MyHttpSession(EventThread* thread):Session(thread){}
    ~MyHttpSession();
    void stop();
    void setClosedHandler(ClosedHandler handler) {
        _closedHandler = handler;
    }

    void response(std::string content);
    void response(int code, std::string content);
    void responseFile(std::string content, std::string fname, std::string contentType);
    void responseFile(std::string filePath);
    void beginFile(std::string fname, std::string contentType);
    void writeFile(std::string content);
    void endFile();
    
    // 实现Session的虚函数
    void onClosed() override;
    bool onMessage(DLNetwork::Buffer* buf) override;
    void onWriteDone() override;

    std::string version;
    std::string method;
    std::string uri;
    std::map<std::string, std::string> urlParams;

private:
    std::string makeupResponse(int code, const std::string& content);
    void setUrlHandler(UrlHandler handler) {
        _handler = handler;
    }
    void refreshCloseTimer();
    void send(const char* buf, size_t size);

    bool _closed = false;
    UrlHandler _handler;
    ClosedHandler _closedHandler;
    Timer* _closeTimer = nullptr;
};

} //DLNetwork