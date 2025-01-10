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
#include "H2Frame.h"
#include "HPacker.h"
#include "MyHttp2Stream.h"

namespace DLNetwork {
template<typename T>
class _MyHttpServer;

class MyHttp2Session : public Session {
public:
    friend class _MyHttpServer<MyHttp2Session>;
    friend class MyHttp2Stream;
    
    typedef DLNetwork::MyHttp2Stream CallbackSession;
    enum {supportH2 = true};
    
    typedef std::function<void(HTTP::Request& request, std::shared_ptr<CallbackSession> sess)> UrlHandler;
    typedef std::function<void(std::shared_ptr<MyHttp2Session> sess)> ClosedHandler;
    MyHttp2Session(EventThread* thread):Session(thread), _closed(false){}
    ~MyHttp2Session();
    
    void stop();
    void setClosedHandler(ClosedHandler handler) {
        _closedHandler = handler;
    }
    std::string description() {
        return _conn->description();
    }
    int sendH2Frame(H2Frame* frame);

    // 实现Session的虚函数
    void onClosed() override;
    bool onMessage(DLNetwork::Buffer* buf) override;
    void onWriteDone() override;

    std::string version;
    std::string method;
    std::string uri;
    std::map<std::string, std::string> urlParams;
private:
    void setUrlHandler(UrlHandler handler) {
        _handler = handler;
    }
    void closeStream(std::shared_ptr<MyHttp2Stream> stream);
    void onStreamEnd(std::shared_ptr<MyHttp2Stream> stream);
    void refreshCloseTimer();

    void send(const char* buf, size_t size);
    int sendHeadersFrame(HeadersFrame* frame);
    void applySettings(SettingsFrame* frame);
    void setInitialWindowsSize(uint32_t size) {
        _initialWindowSize = size;
        // _curWindowSize = size;
    }
    void updateWindowSize(uint32_t delta) {}
    void setMaxFrameSize(uint32_t size) {
        _maxFrameSize = size;
    }
    void setMaxHeaderListSize(uint32_t size){
        _maxHeaderListSize = size;
    }
    H2Frame* parseFrame(const FrameHeader& hdr, const uint8_t* payload);
    void parseHeaders(Buffer& buf, bool endStream, std::shared_ptr<MyHttp2Stream>& stream) {
        parseHeaders((const uint8_t*)buf.peek(), buf.readableBytes(), endStream, stream);
        buf.retrieveAll();
    }
    void parseHeaders(const uint8_t* buf, size_t size, bool endStream, std::shared_ptr<MyHttp2Stream>& stream);
    std::shared_ptr<MyHttp2Stream> getOrGenStream(uint32_t streamId);
    void connectionError(H2Error err);
    void healthCheck();

    bool _closed;
    UrlHandler _handler;
    ClosedHandler _closedHandler;
    Timer* _closeTimer = nullptr;

    std::unordered_map<uint32_t, std::shared_ptr<MyHttp2Stream>> _streams;
    Buffer _headersBuf;
    hpack::HPacker _hpack;
    int _state = 0;
    uint32_t _lastStreamId = 0;
    uint32_t _initialWindowSize = 64*1024;
    uint32_t _maxFrameSize = 16*1024;
    uint32_t _maxHeaderListSize;
    //uint32_t _curWindowSize;
    bool _wantContinueFrame = false;
    bool _continueEndStream = false;

    FrameHeader _frameHeader;
    DataFrame _dataFrame;
    HeadersFrame _headersFrame;
    PriorityFrame _priFrame;
    RSTStreamFrame _rstFrame;
    SettingsFrame _setFrame;
    PushPromiseFrame _pushpFrame;
    PingFrame _pingFrame;
    GoawayFrame _goawayFrame;
    WindowUpdateFrame _wndUpFrame;
    ContinuationFrame _contFrame;
};

} //DLNetwork