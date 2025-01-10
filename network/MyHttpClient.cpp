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
#include "MyHttpClient.h"
#include "EventThread.h"
#include "NetEndian.h"
#include "uv_errno.h"
#include "StringUtil.h"
#include "MyLog.h"
#include <sstream>
#include "TcpConnection.h"
#include "cppdefer.h"

using namespace DLNetwork;

bool MyHttpClient::extractHostPortURI(const std::string& url, std::string& protocol, std::string& host, std::string& port, std::string& uri) {
    std::string protocolSeparator = "://";
    std::string portSeparator = ":";
    std::string uriSeparator = "/";

    std::size_t protocolPos = url.find(protocolSeparator);
    if (protocolPos == std::string::npos) {
        return false;
    }
    protocol = url.substr(0, protocolPos);
    std::size_t hostStart = protocolPos + protocolSeparator.length();
    std::size_t hostEnd = url.find_first_of(":/", hostStart);

    if (hostEnd == std::string::npos) {
        host = url.substr(hostStart);
        // 根据协议设置默认端口
        if (protocol == "https") {
            port = "443";
        }
        else if (protocol == "http") {
            port = "80";
        }
        else if (protocol == "ws") {
            port = "80";
        }
        else if (protocol == "wss") {
            port = "443";
        }
        else {
            return false;
        }
        uri = "/";
        return true;
    }

    host = url.substr(hostStart, hostEnd - hostStart);

    if (url[hostEnd] == ':') {
        std::size_t portStart = hostEnd + 1;
        std::size_t portEnd = url.find(uriSeparator, portStart);
        port = url.substr(portStart, portEnd - portStart);

        if (portEnd != std::string::npos) {
            uri = url.substr(portEnd);
        }
        else {
            uri = "/";
        }
    }
    else {
        port = (protocol == "https") ? "443" : "80"; // 根据协议设置默认端口
        uri = url.substr(hostEnd);
    }

    if (uri.empty()) {
        uri = "/";
    }
}

void DLNetwork::MyHttpClient::connect(std::string host, int port, bool tls, std::string certFile)
{
    EventThread* thread = EventThreadPool::instance().getIdlestThread();
    INetAddress address = INetAddress::fromDomainPort(host.c_str(), port);
    _connection = TcpConnection::createClient(thread, address);
    _tls = tls;
    _host = host;
    _port = port;
    _certFile = certFile;
    _connection->setOnMessage(std::bind(&MyHttpClient::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    _connection->setOnWriteDone(std::bind(&MyHttpClient::onWriteDone, this, std::placeholders::_1));
    _connection->setConnectCallback(std::bind(&MyHttpClient::onConnectionChange, this, std::placeholders::_1, std::placeholders::_2));
	_connection->startConnect();

}

void DLNetwork::MyHttpClient::sendRequest(std::string method, std::string uri, std::string body, std::string contentType)
{
    //std::string protocol, host, port, uri;
    //if (!extractHostPortURI(url, protocol, host, port, uri)) {
    //    return;
    //}
    std::string host = _host + ":" + std::to_string(_port);
    std::map<std::string, std::string> headers;
    headers.emplace("Host", host);
    headers.emplace("Connection", "keep-alive");
    headers.emplace("User-Agent", "UA");
    headers.emplace("accept", "*/*");
    if (!body.empty()) {
        headers.emplace("content-type", contentType);
        headers.emplace("Content-Length", std::to_string(body.size()));
    }

    HTTP::Request request(HTTP::method_from_string(method), uri, headers);
    std::string data = request.serialize();

    _connection->write(data.c_str(), data.size());
}

void DLNetwork::MyHttpClient::close()
{
    _connection->close();
}

void DLNetwork::MyHttpClient::onConnectionChange(TcpConnection::Ptr conn, ConnectEvent e)
{
    if (e == ConnectEvent::Established) {
        if (_tls) {
            _connection->enableTlsClient(_host, _certFile);
        }
        if (_onConnected) {
            _onConnected(*this);
        }
    }
    else if (e == ConnectEvent::Closed) {
        _closed = true;

        if (_onClose) {
            _onClose(*this);
        }
        _onClose = nullptr;
    }
}

bool DLNetwork::MyHttpClient::onMessage(TcpConnection::Ptr conn, DLNetwork::Buffer* buf)
{
    std::string inbuf;
    //先把所有数据都取出来
    inbuf.append(buf->peek(), buf->readableBytes());

    if (inbuf.length() <= 4)
        return true;

    HTTP::Response resp;
    HTTP::Response::ErrorCode ecode = resp.deserialize(inbuf.c_str(), inbuf.length());
    if (ecode == HTTP::Response::ErrorCode::ResponseInsufficent) {
        return true;
    }
    else if (ecode != HTTP::Response::ErrorCode::OK) {
        conn->close();
        return false;
    }

    DEFER(buf->retrieveAll(););

    if (_onResponse) {
        _onResponse(*this, std::move(resp));
    }

    return true;
}

void DLNetwork::MyHttpClient::onWriteDone(TcpConnection::Ptr conn)
{
}
