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
#include "MyHttpSession.h"
#include "StringUtil.h"
#include "cppdefer.h"
#include <sstream>
#include <string>
#include <MyLog.h>


using namespace DLNetwork;

DLNetwork::MyHttpSession::~MyHttpSession() {
    stop();
    mInfo() << "~MyHttpSession" << ptr2string(this);
}

void DLNetwork::MyHttpSession::stop()
{
    if (_closeTimer) {
        thread()->delTimer(_closeTimer);
        _closeTimer = nullptr;
    }
    if (_conn) {
        _conn->close();
    }
}


//组装http协议应答包
/*
HTTP/1.1 200 OK\r\n
Content-Type: text/html\r\n
Date: Wed, 16 May 2018 10:06:10 GMT\r\n
Content-Length: 4864\r\n
\r\n\
{"code": 0, "msg": ok}
*/
std::string MyHttpSession::makeupResponse(int code, const std::string& content) {
    std::ostringstream os;
    os << "HTTP/1.1 " << code << " " << HTTP::Response::respCode2string(code) << "\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: "
        << std::to_string(content.length()) << "\r\n\r\n"
        << content;

    return os.str();
}

void MyHttpSession::response(std::string content) {
    auto resp = makeupResponse(200, content);
    send(resp.c_str(), resp.size());
}

void MyHttpSession::response(int code, std::string content) {
    auto resp = makeupResponse(code, content);
    send(resp.c_str(), resp.size());
}

void MyHttpSession::responseFile(std::string content, std::string fname, std::string contentType) {
    HTTP::Response resp;
    resp.version = HTTP::Version::HTTP_1_1;
    resp.responseCode = HTTP::Response::OK;
    if (contentType.empty()) {
        resp.headers["Content-Type"] = "application/octet-stream";
    }
    else {
        resp.headers["Content-Type"] = contentType;
    }
    if (!fname.empty()) {
        resp.headers["Content-Disposition"] = std::string("attachment; filename=\"") + fname + "\"";
    }
    resp.headers["Content-Length"] = std::to_string(content.size());

    std::string str = resp.serialize();
    send(str.c_str(), str.size());
    send(content.c_str(), content.size());
    _conn->closeAfterWrite();
}

void MyHttpSession::responseFile(std::string filePath) {
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

void MyHttpSession::beginFile(std::string fname, std::string contentType) {
    HTTP::Response resp;
    if (version == "HTTP/1.0")
    {
        resp.version = HTTP::Version::HTTP_1_0;
    }
    else if (version == "HTTP/1.1")
    {
        resp.version = HTTP::Version::HTTP_1_1;
        resp.headers["Access-Control-Allow-Methods"] = "no-cache";
        resp.headers["Access-Control-Expose-Headers"] = "X-Requested-With";
        resp.headers["Expires"] = "-1";
        resp.headers["Pragma"] = "no-cache";
    }
    else
    {
        resp.version = HTTP::Version::HTTP_1_1;
    }
    resp.responseCode = HTTP::Response::OK;
    if (contentType.empty()) {
        resp.headers["Content-Type"] = "application/octet-stream";
    }
    else {
        resp.headers["Content-Type"] = contentType;
    }
    if (!fname.empty()) {
        resp.headers["Content-Disposition"] = std::string("attachment; filename=\"") + fname + "\"";
    }
    resp.headers["Transfer-Encoding"] = "chunked";
    resp.headers["Access-Control-Allow-Origin"] = "*";
    std::string str = resp.serialize();
    send(str.c_str(), str.size());
    refreshCloseTimer();
}

void MyHttpSession::writeFile(std::string content) {
    std::stringstream stream;
    char szHex[16] = { 0 };
    sprintf(szHex, "%zx", content.size());
    stream << szHex << "\r\n" << content << "\r\n";
    send(stream.str().c_str(), stream.str().size());
    refreshCloseTimer();
}

void MyHttpSession::endFile() {
    const char* end = "0\r\n\r\n";
    send(end, 5);
    _conn->closeAfterWrite();
}

void MyHttpSession::onClosed() {
    _closed = true;
    if (_closeTimer) {
        thread()->delTimer(_closeTimer);
        _closeTimer = nullptr;
    }

    if (_closedHandler) {
        _closedHandler(std::static_pointer_cast<MyHttpSession>(shared_from_this()));
    }
    _closedHandler = nullptr;
}

bool MyHttpSession::onMessage(DLNetwork::Buffer* buf) {
    std::string inbuf;
    //先把所有数据都取出来
    inbuf.append(buf->peek(), buf->readableBytes());
    DEFER(buf->retrieveAll(););

    //因为一个http包头的数据至少\r\n\r\n，所以大于4个字符
    //小于等于4个字符，说明数据未收完，退出，等待网络底层接着收取
    if (inbuf.length() <= 4)
        return true;

    HTTP::Request req;
    HTTP::Request::ErrorCode ecode = req.deserialize(inbuf.c_str(), inbuf.length());
    if (ecode != HTTP::Request::ErrorCode::OK) {
        _conn->close();
        return false;
    }

    if (req.version == HTTP::Version::HTTP_1_0) {
        version = "HTTP/1.0";
    }
    if (req.version == HTTP::Version::HTTP_1_1) {
        version = "HTTP/1.1";
    }

    if (_handler) {
        _handler(req, std::static_pointer_cast<MyHttpSession>(shared_from_this()));
    }

    bool close = false;
    if (req.version == HTTP::Version::HTTP_1_0) {
        _conn->closeAfterWrite();
        close = true;
    }
    if (req.version == HTTP::Version::HTTP_1_1) {
        auto iter = req.headers.find("Connection");
        if (iter != req.headers.end() && iter->second == "close") {
            _conn->closeAfterWrite();
            close = true;
        }
    }

    if (!close) {
        refreshCloseTimer();
    }
    return true;
}

void MyHttpSession::onWriteDone() {
    // 可以为空实现
}

void MyHttpSession::refreshCloseTimer() {
    std::weak_ptr<MyHttpSession> weakThis(std::static_pointer_cast<MyHttpSession>(shared_from_this()));
    if (_closeTimer) {
        thread()->delTimer(_closeTimer);
        _closeTimer = nullptr;
    }
    _closeTimer = thread()->addTimer(30000, [weakThis](void*) {
        if (auto strongThis = weakThis.lock()) {
            strongThis->_conn->closeAfterWrite();
        }
        return 0;
    });

    // thread()->dispatch([weakThis]() {
    //     if (auto strongThis = weakThis.lock()) {
    //         if (strongThis->_closeTimer) {
    //             strongThis->thread()->delTimerInLoop(strongThis->_closeTimer);
    //             strongThis->_closeTimer = nullptr;
    //         }
    //         strongThis->_closeTimer = strongThis->thread()->addTimerInLoop(30000, [weakThis](void*) {
    //             if (auto strongThis = weakThis.lock()) {
    //                 strongThis->_conn->closeAfterWrite();
    //             }
    //             return 0;
    //             });
    //     }

    //     });
}

void DLNetwork::MyHttpSession::send(const char* buf, size_t size)
{
    Session::send(buf, size);
}
