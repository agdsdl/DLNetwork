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
#include "MyHttp2Session.h"
#include "StringUtil.h"
#include "cppdefer.h"
#include <sstream>
#include <string>
#include <MyLog.h>

using namespace DLNetwork;

const char HTTP2_PREFACE[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

MyHttp2Session::MyHttp2Session(std::unique_ptr<TcpConnection>&& conn) :_conn(std::move(conn)), _closed(false) {
}

MyHttp2Session::~MyHttp2Session() {
    stop();
    mInfo() << "~MyHttp2Session" << this;
}

void MyHttp2Session::stop()
{
    if (_closeTimer) {
        thread()->delTimer(_closeTimer);
        _closeTimer = nullptr;
    }

    if (_conn) {
        _conn->close();
    }
}


void MyHttp2Session::takeoverConn() {
    _conn->setConnectCallback(std::bind(&MyHttp2Session::onConnectionChange, this, std::placeholders::_1, std::placeholders::_2));
    _conn->setOnMessage(std::bind(&MyHttp2Session::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    _conn->setOnWriteDone(std::bind(&MyHttp2Session::onWriteDone, this, std::placeholders::_1));
    _conn->attach();
}

void MyHttp2Session::onConnectionChange(TcpConnection& conn, ConnectEvent e) {
    if (e == ConnectEvent::Closed) {
        _closed = true;
        for (auto& onclose: _closeHandlers) {
            stop();
            onclose(shared_from_this());
        }
        //if (_closedHandler) {
        //    _closedHandler(shared_from_this());
        //}
        for (auto& s : _streams) {
            s.second->stop();
        }
    }
}

bool MyHttp2Session::onMessage(TcpConnection& conn, DLNetwork::Buffer* buf) {
    if (_state == 0) {
        if (buf->readableBytes() < sizeof(HTTP2_PREFACE)-1) {
            return true; // wait full preface
        }
        std::string inbuf;
        inbuf.append(buf->peek(), sizeof(HTTP2_PREFACE) - 1);
        if (inbuf == HTTP2_PREFACE) {
            _state = 1;
        }
        buf->retrieve(sizeof(HTTP2_PREFACE) - 1);
    }
    if (_state == 0) {
        return true;
    }
    //assert(_state != 0);
    while (buf->readableBytes() > 0) {
        if (buf->readableBytes() < H2_FRAME_HEADER_SIZE) {
            return true;
        }
        _frameHeader.decode((uint8_t*)buf->peek(), H2_FRAME_HEADER_SIZE);
        if (buf->readableBytes() < _frameHeader.getLength()+ H2_FRAME_HEADER_SIZE) {
            return true; // wait full frame
        }
        H2Frame* frame = parseFrame(_frameHeader, (uint8_t*)buf->peek()+ H2_FRAME_HEADER_SIZE);
        buf->retrieve(frame->calcFrameSize());
        //mInfo() << "MyHttp2Session::onMessage gotFrame id" << frame->getStreamId() << "type" << (int)frame->type();
        if (_frameHeader.getType() == H2FrameType::SETTINGS) {
            if (_frameHeader.getStreamId() != 0) {
                connectionError(H2Error::PROTOCOL_ERROR);
                return false; // abort conn
            }
            //TODO: apply setting
            SettingsFrame* settingsFrame = dynamic_cast<SettingsFrame*>(frame);
            if (!settingsFrame->isAck()) {

                SettingsFrame mySetting;
                mySetting.setStreamId(0);
                ParamVector params = {
                    {MAX_CONCURRENT_STREAMS, 128},
                    {INITIAL_WINDOW_SIZE, 64 * 1024},
                    {MAX_FRAME_SIZE, 16 * 1024*1024},
                    //{ENABLE_CONNECT_PROTOCOL, 0},
                };
                mySetting.setParams(params);
                sendH2Frame(&mySetting);

                applySettings(settingsFrame);
                SettingsFrame replay;
                replay.setStreamId(frame->getStreamId());
                replay.setAck(true);
                sendH2Frame(&replay);

                if (_state == 1) {
                    _state = 2; // conn ok
                }
            }
        }
        else {
            if (_state == 1) {
                mWarning() << "MyHttp2Session::onMessage, need setting but type=" << _frameHeader.getType();
                return false; // abort conn
            }

            switch (_frameHeader.getType()) {
            case H2FrameType::GOAWAY:
                break;
            case H2FrameType::PING:
            {
                PingFrame* frame2 = dynamic_cast<PingFrame*>(frame);
                if (!frame2->isAck()) {
                    frame2->setAck(true);
                    sendH2Frame(frame2);
                }
            }
            break;
            case H2FrameType::WINDOW_UPDATE:
            {
                WindowUpdateFrame* frame2 = dynamic_cast<WindowUpdateFrame*>(frame);
                updateWindowSize(frame2->getWindowSizeIncrement());
            }
            break;
            }
        }

        if (frame->getStreamId() == 0) {
            continue;
        }

        std::shared_ptr<MyHttp2Stream> stream = getOrGenStream(frame->getStreamId());
        _lastStreamId = frame->getStreamId();

        if (_state == 2) {
            switch (_frameHeader.getType()) {
            case H2FrameType::HEADERS:
            {
                HeadersFrame* headersFrame = dynamic_cast<HeadersFrame*>(frame);
                if (!headersFrame->hasEndHeaders()) {
                    _wantContinueFrame = true;
                    _continueEndStream = headersFrame->hasEndStream();
                    _headersBuf.append(headersFrame->getBlock(), headersFrame->getBlockSize());
                }
                else {
                    _wantContinueFrame = false;
                    parseHeaders(headersFrame->getBlock(), headersFrame->getBlockSize(), headersFrame->hasEndStream(), stream);
                }
            }
            break;
            case H2FrameType::CONTINUATION:
            {
                ContinuationFrame* frame2 = dynamic_cast<ContinuationFrame*>(frame);
                if (_wantContinueFrame) {
                    _headersBuf.append(frame2->getBlock(), frame2->getBlockSize());
                    if (frame2->hasEndHeaders()) {
                        parseHeaders(_headersBuf, _continueEndStream, stream);
                        _wantContinueFrame = false;
                    }
                }
                else {
                    connectionError(H2Error::PROTOCOL_ERROR);
                }
            }
            break;
            case H2FrameType::DATA:
            {
                DataFrame* frame2 = dynamic_cast<DataFrame*>(frame);
                stream->onDataFrame((uint8_t*)frame2->data(), frame2->size(), frame2->hasEndStream());
            }
            break;
            case H2FrameType::GOAWAY:
                break;
            case H2FrameType::RST_STREAM:
            {
                RSTStreamFrame* frame2 = dynamic_cast<RSTStreamFrame*>(frame);
                stream->onRstStreamFrame(frame2->getErrorCode());
                closeStream(stream);
            }
            break;
            case H2FrameType::WINDOW_UPDATE:
            {
                WindowUpdateFrame *frame2 = dynamic_cast<WindowUpdateFrame*>(frame);
                stream->updateWindowSize(frame2->getWindowSizeIncrement());
            }
            break;

            }
        }
    }
    //DEFER(buf->retrieveAll(););

    return true;
}

void MyHttp2Session::applySettings(SettingsFrame* frame)
{
    const ParamVector &params = frame->getParams();
    for (auto& kv : params) {
        //mInfo() << "applySettings, id=" << kv.first << ", value=" << kv.second;
        switch (kv.first) {
        case HEADER_TABLE_SIZE:
            _hpack.setMaxTableSize(kv.second);
            break;
        case INITIAL_WINDOW_SIZE:
            if (kv.second > H2_MAX_WINDOW_SIZE) {
                // RFC 7540, 6.5.2
                connectionError(H2Error::FLOW_CONTROL_ERROR);
                return;
            }
            break;
        case MAX_FRAME_SIZE:
            if (kv.second < H2_DEFAULT_FRAME_SIZE || kv.second > H2_MAX_FRAME_SIZE) {
                // RFC 7540, 6.5.2
                connectionError(H2Error::PROTOCOL_ERROR);
                return;
            }
            setMaxFrameSize(kv.second);
            break;
        case MAX_CONCURRENT_STREAMS:
            break;
        case ENABLE_PUSH:
            if (kv.second != 0 && kv.second != 1) {
                // RFC 7540, 6.5.2
                connectionError(H2Error::PROTOCOL_ERROR);
                return;
            }
            break;
        case ENABLE_CONNECT_PROTOCOL:
            break;
        }
    }
}

std::shared_ptr<MyHttp2Stream> MyHttp2Session::getOrGenStream(uint32_t streamId)
{
    std::shared_ptr<MyHttp2Stream> stream;
    auto iter = _streams.find(streamId);
    if (iter != _streams.end()) {
        stream = iter->second;
    }
    else {
        stream = std::make_shared<MyHttp2Stream>(streamId, this, _initialWindowSize, _maxFrameSize);
        _streams[streamId] = stream;
        stream->setUrlHandler(_handler);
        stream->addClosedHandler(std::bind(&MyHttp2Session::onStreamEnd, this, std::placeholders::_1));
    }

    return stream;
}

void MyHttp2Session::closeStream(std::shared_ptr<MyHttp2Stream> stream)
{
    mInfo() << "MyHttp2Session::closeStream" << stream->streamId();
    _streams.erase(stream->streamId());
    if (_streams.size() == 0) {
        // idle to close.
    }
}

void MyHttp2Session::onStreamEnd(std::shared_ptr<MyHttp2Stream> stream)
{
    mInfo() << "MyHttp2Session::onStreamEnd" << stream->streamId();
    std::weak_ptr<MyHttp2Session> weakThis(shared_from_this());
    thread()->dispatch([weakThis, stream]() {
        if (auto strongThis = weakThis.lock()) {
            strongThis->closeStream(stream);
        }
    });
}

void MyHttp2Session::onWriteDone(TcpConnection& conn) {
}

void MyHttp2Session::send(const char* buf, size_t size)
{
    _conn->write(buf, size);
}

H2Frame* MyHttp2Session::parseFrame(const FrameHeader& hdr, const uint8_t* payload)
{
    H2Frame* frame = nullptr;
    switch (_frameHeader.getType()) {
    case H2FrameType::DATA:
        frame = &_dataFrame;
        break;

    case H2FrameType::HEADERS:
        frame = &_headersFrame;
        break;

    case H2FrameType::PRIORITY:
        frame = &_priFrame;
        break;

    case H2FrameType::RST_STREAM:
        frame = &_rstFrame;
        break;

    case H2FrameType::SETTINGS:
        frame = &_setFrame;
        break;

    case H2FrameType::PUSH_PROMISE:
        frame = &_pushpFrame;
        break;

    case H2FrameType::PING:
        frame = &_pingFrame;
        break;

    case H2FrameType::GOAWAY:
        frame = &_goawayFrame;
        break;

    case H2FrameType::WINDOW_UPDATE:
        frame = &_wndUpFrame;
        break;

    case H2FrameType::CONTINUATION:
        frame = &_contFrame;
        break;

    default:
        mWarning() << "MyHttp2Session::handleFrame, invalid frame, type=" << frame->type();
        break;
    }

    H2Error err = frame->decode(hdr, payload);
    if (err != H2Error::NOERR) {
        mWarning() << "MyHttp2Session::handleFrame, parse failed!" << (int)err;
    }
    return frame;
}

int MyHttp2Session::sendH2Frame(H2Frame* frame)
{
    if (frame->type() == H2FrameType::HEADERS) {
        HeadersFrame* headers = dynamic_cast<HeadersFrame*>(frame);
        return sendHeadersFrame(headers);
    }
    else if (frame->type() == H2FrameType::DATA) {
        // flow ctrl
    }
    else if (frame->type() == H2FrameType::WINDOW_UPDATE && frame->getStreamId() != 0) {
        // flow ctrl
    }

    size_t payloadSize = frame->calcPayloadSize();
    size_t frameSize = payloadSize + H2_FRAME_HEADER_SIZE;

    std::unique_ptr<char[]> buf = std::make_unique<char[]>(frameSize);
    int ret = frame->encode((uint8_t*)buf.get(), frameSize);
    if (ret < 0) {
        mWarning() << "sendH2Frame, failed to encode frame" << ret;
        return -2;
    }
    assert(ret == (int)frameSize);
    this->send(buf.get(), ret);

    return 0;
}

int MyHttp2Session::sendHeadersFrame(HeadersFrame* frame)
{
    h2_priority_t pri;
    frame->setPriority(pri);
    size_t len1 = H2_FRAME_HEADER_SIZE + (frame->hasPriority() ? H2_PRIORITY_PAYLOAD_SIZE : 0);
    auto& headers = frame->getHeaders();
    size_t hdrSize = frame->getHeadersSize();

    size_t hpackSize = hdrSize * 3 / 2;
    size_t frameSize = len1 + hpackSize;

    //Buffer buf(frameSize);
    std::unique_ptr<char[]> buf = std::make_unique<char[]>(frameSize);
    int ret = _hpack.encode(headers, (uint8_t*)buf.get() + len1, hpackSize);
    if (ret < 0) {
        return -1;
    }
    size_t bsize = ret;
    ret = frame->encode((uint8_t*)buf.get(), len1, bsize);
    assert(ret == (int)len1);
    this->send(buf.get(), len1 + bsize);
    return 0;
}

void MyHttp2Session::parseHeaders(const uint8_t* buf, size_t size, bool endStream, std::shared_ptr<MyHttp2Stream>& stream)
{
    HeaderVector headers;
    _hpack.decode(buf, size, headers);
    //for (auto& header : headers) {
    //    mDebug() << "MyHttp2Session::parseHeaders, " << header.first << ": " << header.second;
    //}
    stream->onHeadersFrame(headers, endStream);
}

void MyHttp2Session::connectionError(H2Error err)
{
    mWarning() << "MyHttp2Session::connectionError" << (int32_t)err;
    GoawayFrame frame;
    frame.setErrorCode(uint32_t(err));
    frame.setStreamId(0);
    frame.setLastStreamId(_lastStreamId);
    sendH2Frame(&frame);
    _conn->closeAfterWrite();
}

void MyHttp2Session::refreshCloseTimer() {
    std::weak_ptr<MyHttp2Session> weakThis(shared_from_this());
    thread()->dispatch([weakThis]() {
        if (auto strongThis = weakThis.lock()) {
            if (strongThis->_closeTimer) {
                strongThis->thread()->delTimerInLoop(strongThis->_closeTimer);
                strongThis->_closeTimer = nullptr;
            }
            strongThis->_closeTimer = strongThis->thread()->addTimerInLoop(30000, [weakThis](void*) {
                if (auto strongThis = weakThis.lock()) {
                    strongThis->connection().closeAfterWrite();
                }
                return 0;
                });
        }
    });
}
