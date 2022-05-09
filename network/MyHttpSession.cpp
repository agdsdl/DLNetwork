#include "MyHttpSession.h"
#include "StringUtil.h"
#include <sstream>

using namespace DLNetwork;

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
    os << "HTTP/1.1 " << code << " " << HTTP::Response::respCode2string(code) << "\r\nContent-Type: text/html\r\nContent-Length: "
        << content.length() << "\r\n\r\n"
        << content;

    return os.str();
}

void MyHttpSession::response(std::string content) {
    auto resp = makeupResponse(200, content);
    _conn->write(resp.c_str(), resp.size());
}

void MyHttpSession::response(int code, std::string content) {
    auto resp = makeupResponse(code, content);
    _conn->write(resp.c_str(), resp.size());
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
    resp.headers["Content-Length"] = content.size();

    std::string str = resp.serialize();
    _conn->write(str.c_str(), str.size());
    _conn->write(content.c_str(), content.size());
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
    resp.headers["Transfer-Encoding"] = "chunked";

    std::string str = resp.serialize();
    _conn->write(str.c_str(), str.size());
    refreshCloseTimer();
}

void MyHttpSession::writeFile(std::string content) {
    std::stringstream stream;
    stream << std::hex << content.size() << "\r\n" << content << "\r\n";
    _conn->write(stream.str().c_str(), stream.str().size());
    refreshCloseTimer();
}

void MyHttpSession::endFile() {
    const char* end = "0\r\n\r\n";
    _conn->write(end, 5);
    _conn->closeAfterWrite();
}

void MyHttpSession::takeoverConn() {
    _conn->setConnectCallback(std::bind(&MyHttpSession::onConnectionChange, this, std::placeholders::_1, std::placeholders::_2));
    _conn->setOnMessage(std::bind(&MyHttpSession::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    _conn->setOnWriteDone(std::bind(&MyHttpSession::onWriteDone, this, std::placeholders::_1));
    _conn->attach();
}

void MyHttpSession::onConnectionChange(TcpConnection& conn, ConnectEvent e) {
    if (e == ConnectEvent::Closed) {
        _closed = true;
        for (auto& onclose: _closeHandlers) {
            onclose(shared_from_this());
        }
        //if (_closedHandler) {
        //    _closedHandler(shared_from_this());
        //}
    }
}

bool MyHttpSession::onMessage(TcpConnection& conn, DLNetwork::Buffer* buf) {
    std::string inbuf;
    //先把所有数据都取出来
    inbuf.append(buf->peek(), buf->readableBytes());
    //因为一个http包头的数据至少\r\n\r\n，所以大于4个字符
    //小于等于4个字符，说明数据未收完，退出，等待网络底层接着收取
    if (inbuf.length() <= 4)
        return true;

    HTTP::Request req;
    HTTP::Request::ErrorCode ecode = req.deserialize(inbuf.c_str(), inbuf.length());
    if (ecode != HTTP::Request::ErrorCode::OK) {
        conn.close();
        return false;
    }

    if (_handler) {
        _handler(req, shared_from_this());
    }

    bool close = false;
    if (req.version == HTTP::Version::HTTP_1_0) {
        conn.closeAfterWrite();
        close = true;
    }
    if (req.version == HTTP::Version::HTTP_1_1) {
        auto iter = req.headers.find("Connection");
        if (iter != req.headers.end() && iter->second == "close") {
            conn.closeAfterWrite();
            close = true;
        }
    }

    if (!close) {
        refreshCloseTimer();
    }
    buf->retrieveAll();
    return true;
}

void MyHttpSession::onWriteDone(TcpConnection& conn) {
}

void MyHttpSession::refreshCloseTimer() {
    thread()->dispatch([this]() {
        if (_closeTimer) {
            connection().getThread()->delTimerInLoop(_closeTimer);
        }
        std::weak_ptr<MyHttpSession> weakThis(shared_from_this());
        _closeTimer = connection().getThread()->addTimerInLoop(30000, [weakThis](void*) {
            if (auto strongThis = weakThis.lock()) {
                strongThis->connection().closeAfterWrite();
            }
            return 0;
            });

        });
}