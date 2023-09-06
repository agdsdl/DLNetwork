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
#include "H2Frame.h"
//#include "MyHttp2Session.h"

namespace DLNetwork {
    class MyHttp2Session;
class MyHttp2Stream : public std::enable_shared_from_this<MyHttp2Stream> {
public:
    MyHttp2Stream(uint32_t streamId, DLNetwork::MyHttp2Session* session, uint32_t initWindowSize, uint32_t maxFrameSize);
    ~MyHttp2Stream();
    typedef std::function<void(HTTP::Request& request, std::shared_ptr<MyHttp2Stream> sess)> UrlHandler;
    typedef std::function<void(std::shared_ptr<MyHttp2Stream> sess)> ClosedHandler;
    void stop();
    void setUrlHandler(UrlHandler handler) {
        _handler = handler;
    }

    void addClosedHandler(ClosedHandler&& handler) {
        _closeHandlers.push_back(std::move(handler));
    }

    void response(std::string content);
    void response(int code, std::string content);
    void responseFile(std::string content, std::string fname, std::string contentType);
    void responseFile(std::string filePath);
    void beginFile(std::string fname, std::string contentType);
    void writeFile(std::string content);
    void endFile();
    void closeAfterWrite() {
        //_conn->closeAfterWrite();
    }
    EventThread* thread();
    std::string description();
    uint32_t streamId() { return _streamId; }

    void updateWindowSize(uint32_t delta){}
    void onHeadersFrame(HeaderVector& headers, bool isEnd);
    void onDataFrame(const uint8_t* data, size_t size, bool isEnd);
    void onRstStreamFrame(uint32_t reason);

    std::string version;
    std::string method;
    std::string uri;
    std::map<std::string, std::string> urlParams;
private:
    HTTP::Request _request;
    uint32_t _streamId;
    MyHttp2Session* _session;
    void send(const char* buf, size_t size);
    int sendHeaders(const HeaderVector& headers, bool endStream);
    int sendData(const uint8_t* data, size_t size, bool endStream);
    bool _closed;
    UrlHandler _handler;
    //ClosedHandler _closedHandler;
    uint32_t _initWindowSize;
    uint32_t _maxFrameSize;
    std::vector<ClosedHandler> _closeHandlers;
};
} //DLNetwork