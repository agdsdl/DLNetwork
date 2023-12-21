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
#include "MyHttp2Stream.h"
#include "StringUtil.h"
#include "cppdefer.h"
#include <sstream>
#include <string>
#include <MyLog.h>
#include "MyHttp2Session.h"

using namespace DLNetwork;

MyHttp2Stream::MyHttp2Stream(uint32_t streamId, MyHttp2Session* session, uint32_t initWindowSize, uint32_t maxFrameSize) 
    : _streamId(streamId), _session(session), _initWindowSize(initWindowSize), _maxFrameSize(maxFrameSize), _closed(false) {
    mInfo() << "MyHttp2Stream create id" << _streamId << ptr2string(this);
}

MyHttp2Stream::~MyHttp2Stream() {
    //stop();
    mInfo() << "~MyHttp2Stream id" << _streamId << ptr2string(this);
}

void MyHttp2Stream::stop()
{
    if (!_closed) {
        _closed = true;
        if (_closedHandler) {
            _closedHandler(shared_from_this());
        }

        _session->onStreamEnd(shared_from_this());
    }

    //for (auto& h : _closeHandlers) {
    //    h(shared_from_this());
    //}
    //_closeHandlers.clear();
}

void MyHttp2Stream::response(std::string content) {
    response(200, content);
}

void MyHttp2Stream::response(int code, std::string content) {
    HeaderVector headers;
    size_t headerSize;
    headers.emplace_back(H2HeaderStatus, std::to_string(code));
    headers.emplace_back("content-type", "text/html; charset=utf-8");
    headers.emplace_back("content-length", std::to_string(content.length()));
    headers.emplace_back("access-control-allow-origin", "*");
    headers.emplace_back("access-control-max-age", "7200");
    sendHeaders(headers, false);
    sendData((const uint8_t*)content.c_str(), content.size(), true);
}

void MyHttp2Stream::responseFile(std::string content, std::string fname, std::string contentType) {
    HeaderVector headers;
    size_t headerSize;
    headers.emplace_back(H2HeaderStatus, "200");
    if (contentType.empty()) {
        headers.emplace_back("content-type", "application/octet-stream");
    }
    else {
        headers.emplace_back("content-type", contentType);
    }
    headers.emplace_back("content-length", std::to_string(content.length()));
    if (!fname.empty()) {
        headers.emplace_back("content-disposition", std::string("attachment; filename=\"") + fname + "\"");
    }
    sendHeaders(headers, false);
    sendData((const uint8_t*)content.c_str(), content.size(), true);
}

void MyHttp2Stream::responseFile(std::string filePath) {
    auto lastSep = std::find_if(std::reverse_iterator(filePath.end()), std::reverse_iterator(filePath.begin()), [](auto& i) {
        return i == '/' || i == '\\';
        });

    std::string name;
    if (lastSep != std::reverse_iterator(filePath.begin())) {
        name = filePath.substr(lastSep.base() - filePath.begin());
    }
    else {
        name = filePath;
    }

    FILE* fp = fopen(filePath.c_str(), "rb");
    if (!fp) {
        response(400, std::string("can't open file: ") + filePath);
        return;
    }

    beginFile(name, "");
    char buf[1024];
    while (!feof(fp)) {
        int n = fread(buf, 1, sizeof(buf), fp);
        if (n > 0) {
            writeFile(std::string(buf, n));
        }
    }
    endFile();
    fclose(fp);
}

void MyHttp2Stream::beginFile(std::string fname, std::string contentType) {
    HeaderVector headers;
    size_t headerSize;
    headers.emplace_back(H2HeaderStatus, "200");
    if (contentType.empty()) {
        headers.emplace_back("content-type", "application/octet-stream");
    }
    else {
        headers.emplace_back("content-type", contentType);
    }
    if (!fname.empty()) {
        headers.emplace_back("content-disposition", std::string("attachment; filename=\"") + fname + "\"");
    }
    headers.emplace_back("access-control-allow-origin", "*");
    headers.emplace_back("access-control-max-age", "7200");
    sendHeaders(headers, false);
}

void MyHttp2Stream::writeFile(std::string content) {
    if (_closed) {
        return;
    }
    sendData((const uint8_t*)content.c_str(), content.size(), false);
}

void MyHttp2Stream::endFile() {
    std::string content;
    sendData((const uint8_t*)content.c_str(), content.size(), true);
}

EventThread* MyHttp2Stream::thread() { return _session->thread(); }

std::string MyHttp2Stream::description() {
    return _session->description();
}


void MyHttp2Stream::onHeadersFrame(HeaderVector& headers, bool isEnd)
{
    HTTP::Request request;
    //int contentLength = 0;
    for (auto& header : headers)     {
        if (header.first == ":method") {
            request.method = HTTP::method_from_string(header.second);
        }
        else if (header.first == ":path") {
            request.resource = header.second;
            HTTP::urlParamParse(request.resource, request.path, request.urlParams);
        }
        else if (header.first == ":scheme") {
            //request.scheme = header.second;
        }
        else if (header.first == ":authority") {
            //request.authority = header.second;
        }
        else if (header.first == ":version") {
            request.version = HTTP::version_from_string(header.second);
        }
        //else if (header.first == "content-length") {
        //    contentLength = std::stoi(header.second);
        //}
        else {
            request.headers[header.first] = header.second;
        }
    }

    if (isEnd) {
        if (_handler) {
            _handler(request, shared_from_this());
        }
    }
    else {
        _request = request;
    }
/*
for (auto& header : headers) {
    if (header.first == ":method") {
        method = header.second;
    }
    else if (header.first == ":path") {
        path = header.second;
    }
    else if (header.first == ":scheme") {
        scheme = header.second;
    }
    else if (header.first == ":authority") {
        authority = header.second;
    }
    else if (header.first == "content-length") {
        contentLength = std::stoi(header.second);
    }
    else if (header.first == "content-type") {
        contentType = header.second;
    }
    else if (header.first == "user-agent") {
        userAgent = header.second;
    }
    else if (header.first == "accept") {
        accept = header.second;
    }
    else if (header.first == "accept-encoding") {
        acceptEncoding = header.second;
    }
    else if (header.first == "accept-language") {
        acceptLanguage = header.second;
    }
    else if (header.first == "cookie") {
        cookie = header.second;
    }
    else if (header.first == "referer") {
        referer = header.second;
    }
    else if (header.first == "origin") {
        origin = header.second;
    }
    else if (header.first == "upgrade-insecure-requests") {
        upgradeInsecureRequests = header.second;
    }
    else if (header.first == "cache-control") {
        cacheControl = header.second;
    }
    else if (header.first == "pragma") {
        pragma = header.second;
    }
    else if (header.first == "if-modified-since") {
        ifModifiedSince = header.second;
    }
    else if (header.first == "if-none-match") {
        ifNoneMatch = header.second;
    }
    else if (header.first == "range") {
        range = header.second;
    }
    else if (header.first == "x-requested-with") {
        xRequestedWith = header.second;
    }
    else if (header.first == "x-forwarded-for") {
        xForwardedFor = header.second;
    }
    else if (header.first == "x-real-ip") {
        xRealIp = header.second;
    }
    else if (header.first == "x-forwarded-proto") {
        xForwardedProto = header.second;
    }
}
*/
}

void MyHttp2Stream::onDataFrame(const uint8_t* data, size_t size, bool isEnd)
{
    _request.body.append((const char*)data, size);
    if (isEnd) {
        if (_handler) {
            _handler(_request, shared_from_this());
        }
    }
}

void MyHttp2Stream::onRstStreamFrame(uint32_t reason)
{
    mInfo() << "onRstStreamFrame" << _streamId << "reason:" << reason;
    stop();
    //for (auto& onclose : _closeHandlers) {
    //    onclose(shared_from_this());
    //}
}

int MyHttp2Stream::sendHeaders(const HeaderVector& headers, bool endStream)
{
    HeadersFrame frame;
    frame.setStreamId(_streamId);
    frame.addFlags(H2_FRAME_FLAG_END_HEADERS);
    if (endStream) {
        frame.addFlags(H2_FRAME_FLAG_END_STREAM);
    }
    size_t headerSize = 0;
    for (auto const& kv : headers) {
        headerSize += kv.first.size() + kv.second.size();
    }

    frame.setHeaders(std::move(headers), headerSize);
    auto ret = _session->sendH2Frame(&frame);

    return ret;
}

int MyHttp2Stream::sendData(const uint8_t* data, size_t size, bool endStream)
{
    //TODO: flow control
    size_t remain = size;
    int ret = 0;
    do {
        size_t n = remain > _maxFrameSize ? _maxFrameSize : remain;
        DataFrame frame;
        frame.setStreamId(_streamId);
        frame.setData(data, n);
        if (endStream && (n == remain)) {
            frame.addFlags(H2_FRAME_FLAG_END_STREAM);
        }
        ret = _session->sendH2Frame(&frame);
        if (ret < 0) {
            break;
        }
        remain -= n;
        data += n;
    } while (remain > 0);
    if (endStream) {
        mInfo() << "MyHttp2Stream::sendData and end stream." << _streamId;
        stop();
    }
    return ret;
}
