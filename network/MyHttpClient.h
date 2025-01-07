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
#include <unordered_map>
#include <map>
#include <string>
#include "uv_errno.h"
#include "TcpServer.h"
#include "TcpConnection.h"
#include "MyLog.h"
#include "HttpSession.h"
#include <memory>
#include <string_view>

namespace DLNetwork {
class MyHttpClient
{
public:
    typedef std::function<void(MyHttpClient& cli, HTTP::Response&& response)> ResponseCallback;
    typedef std::function<void(MyHttpClient& cli)> CloseCallback;
    typedef std::function<void(MyHttpClient& cli)> ConnectedCallback;

    MyHttpClient() {

    }
    ~MyHttpClient() {

    }
    void setOnResponse(ResponseCallback cb) { _onResponse = std::move(cb); }
    void setOnConnected(ConnectedCallback cb) { _onConnected = std::move(cb); }
    void setOnClose(CloseCallback cb) { _onClose = std::move(cb); }
    //void startRequest(std::string method, std::string url, std::string body);
    static bool extractHostPortURI(const std::string& url, std::string& protocol, std::string& host, std::string& port, std::string& uri);
    void connect(std::string host, int port, bool tls,std::string certFile, std::string keyFile);
    void sendRequest(std::string method, std::string uri, std::string body, std::string contentType);
    void close();
private:
    void onConnectionChange(TcpConnection::Ptr conn, ConnectEvent e);
    bool onMessage(TcpConnection::Ptr conn, DLNetwork::Buffer* buf);
    void onWriteDone(TcpConnection::Ptr conn);

    EventThread* _thread = nullptr;

    std::string _host;
    int _port = 0;
    std::string _certFile;
    std::string _keyFile;
    TcpConnection::Ptr _connection;
    ResponseCallback _onResponse;
    CloseCallback _onClose;
    ConnectedCallback _onConnected;
    bool _closed = false;
};

} //DLNetwork
