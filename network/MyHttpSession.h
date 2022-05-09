#pragma once
#include <memory>
#include <map>
#include <unordered_map>
#include <set>
#include "TcpConnection.h"
#include "HttpSession.h"

namespace DLNetwork {
class MyHttpSession : public std::enable_shared_from_this<MyHttpSession> {
public:
    typedef std::function<void(HTTP::Request& request, std::shared_ptr<MyHttpSession> sess)> UrlHandler;
    typedef std::function<void(std::shared_ptr<MyHttpSession> sess)> ClosedHandler;
    MyHttpSession(std::unique_ptr<TcpConnection>&& conn) :_conn(std::move(conn)), _closed(false) {
    }
    ~MyHttpSession() {}

    void setUrlHandler(UrlHandler handler) {
        _handler = handler;
    }
    //void setClosedHandler(ClosedHandler handler) {
    //    _closedHandler = handler;
    //}
    void addClosedHandler(ClosedHandler&& handler) {
        _closeHandlers.push_back(std::move(handler));
    }

    std::string makeupResponse(int code, const std::string& content);

    void response(std::string content);
    void response(int code, std::string content);
    void responseFile(std::string content, std::string fname, std::string contentType);
    void responseFile(std::string filePath);
    void beginFile(std::string fname, std::string contentType);
    void writeFile(std::string content);
    void endFile();
    void closeAfterWrite() {
        _conn->closeAfterWrite();
    }
    TcpConnection& connection() { return *_conn.get(); }
    EventThread* thread() { return _conn->getThread(); }
    void takeoverConn();
    std::string description() {
        return _conn->description();
    }
    std::string version;
    std::string method;
    std::string uri;
    std::map<std::string, std::string> urlParams;
private:
    void onConnectionChange(TcpConnection& conn, ConnectEvent e);
    bool onMessage(TcpConnection& conn, DLNetwork::Buffer* buf);
    void onWriteDone(TcpConnection& conn);

    void refreshCloseTimer();

    std::unique_ptr<TcpConnection> _conn;
    bool _closed;
    UrlHandler _handler;
    //ClosedHandler _closedHandler;
    std::vector<ClosedHandler> _closeHandlers;
    Timer* _closeTimer = nullptr;
};
} //DLNetwork