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
#include "MyHttpServer.h"
#include "EventThread.h"
#include "NetEndian.h"
#include "uv_errno.h"
#include "StringUtil.h"
#include "MyLog.h"
#include <sstream>

using namespace DLNetwork;

//_MyHttpServer::_MyHttpServer()
//{
//}
//
//
//_MyHttpServer::~_MyHttpServer()
//{
//}
//
//bool _MyHttpServer::start(const char *ip, int port, const char* certFile, const char* keyFile)
//{
//}

//void _MyHttpServer::onConnection(std::unique_ptr<TcpConnection>&& conn)
//{
    //pConn->setConnectCallback(std::bind(&_MyHttpServer::onConnectionChange, this, std::placeholders::_1, std::placeholders::_2));
    //pConn->setOnMessage(std::bind(&_MyHttpServer::onMessage, this, std::placeholders::_1, std::placeholders::_2));
    //pConn->setOnWriteDone(std::bind(&_MyHttpServer::onWriteDone, this, std::placeholders::_1));
//}

//void _MyHttpServer::onConnectionChange(TcpConnection & conn, ConnectEvent e)
//{
//    if (e == ConnectEvent::Closed) {
//        _conn_session[&conn]->onConnectionChange(conn, e);
//        _conn_session.erase(&conn);
//    }
//}

//bool _MyHttpServer::onMessage(TcpConnection & conn, DLNetwork::Buffer *buf)
//{
//    assert(false);
//    std::string inbuf;
//    //先把所有数据都取出来
//    inbuf.append(buf->peek(), buf->readableBytes());
//    //因为一个http包头的数据至少\r\n\r\n，所以大于4个字符
//    //小于等于4个字符，说明数据未收完，退出，等待网络底层接着收取
//    if (inbuf.length() <= 4)
//        return true;
//
//    //我们收到的GET请求数据包一般格式如下：
//    /*
//    GET /register.do?p={%22username%22:%20%2213917043329%22,%20%22nickname%22:%20%22balloon%22,%20%22password%22:%20%22123%22} HTTP/1.1\r\n
//    Host: 120.55.94.78:12345\r\n
//    Connection: keep-alive\r\n
//    Upgrade-Insecure-Requests: 1\r\n
//    User-Agent: Mozilla/5.0 (Windows NT 6.1; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/65.0.3325.146 Safari/537.36\r\n
//    Accept-Encoding: gzip, deflate\r\n
//    Accept-Language: zh-CN, zh; q=0.9, en; q=0.8\r\n
//    \r\n
//    */
//    //检查是否以\r\n\r\n结束，如果不是说明包头不完整，退出
//    std::string end = inbuf.substr(inbuf.length() - 4);
//    if (end != "\r\n\r\n")
//        return true;
//    //超过2048个字符，且不含\r\n\r\n，我们认为是非法请求
//    else if (inbuf.length() >= 2048) {
//        conn.close();
//        return false;
//    }
//
//    //以\r\n分割每一行
//    std::vector<std::string> lines;
//    StringUtil::split(inbuf, lines, "\r\n");
//    if (lines.size() < 1 || lines[0].empty()) {
//        conn.close();
//        return false;
//    }
//
//    std::vector<std::string> chunk;
//    StringUtil::split(lines[0], chunk, " ");
//    //chunk中至少有三个字符串：GET+url+HTTP版本号
//    if (chunk.size() < 3) {
//        conn.close();
//        return false;
//    }
//    std::string method = chunk[0];
//    //mInfo() << "_MyHttpServer::onMessage" << "url" << chunk[1].c_str();
//    //inbuf = /register.do?p={%22username%22:%20%2213917043329%22,%20%22nickname%22:%20%22balloon%22,%20%22password%22:%20%22123%22}
//    std::vector<std::string> part;
//    //通过?分割成前后两端，前面是url，后面是参数
//    StringUtil::split(chunk[1], part, "?");
//
//    std::string url = part[0];
//    std::map<std::string, std::string> urlParams;
//    if (part.size() > 1) {
//        char *sp;
//        char *orip = (char*)malloc(part[1].size()+1);
//        char* p = orip;
//        strcpy(p, part[1].c_str());
//        do {
//            sp = strchr(p, '=');
//            if (!sp) {
//                break;
//            }
//            *sp = NULL;
//            char *key = p;
//            p = sp + 1;
//            char* val = p;
//            sp = strchr(p, '&');
//            if (sp) {
//                *sp = NULL;
//                p = sp + 1;
//            }
//            urlParams.emplace(key, val);
//        } while (sp);
//        free(orip);
//    }
//    std::string version = chunk[2];
//
//    if (_handler) {
//        MyHttpSession* sess = _conn_session[&conn].get();
//        _handler(url, urlParams, *sess);
//    }
//    return false;
//}
//
//void _MyHttpServer::onWriteDone(TcpConnection & conn)
//{
//}

//void _MyHttpServer::closedHandler(std::shared_ptr<MyHttpSession> sess) {
//}
//
//void _MyHttpServer::urlHanlder(HTTP::Request& request, std::shared_ptr<MyHttpSession> sess) {
//}
