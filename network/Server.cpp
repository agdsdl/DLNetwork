#include "Server.h"
#include "EventThread.h"
#include "MyLog.h"

namespace DLNetwork {

Server::Server() : _listenSock(INVALID_SOCKET), _thread(nullptr), _reusePort(true), _timer(nullptr) {
}

Server::~Server() {
    destroy();
}

void Server::destroy() {
    if (_listenSock != INVALID_SOCKET) {
        _thread->removeEvents(_listenSock);
        myclose(_listenSock);
        _listenSock = INVALID_SOCKET;
    }
    for (auto& pair : _conn_session) {
        if (!pair.second->thread()->isCurrentThread()) {
            auto session = pair.second;
            _thread->dispatch([session]() {
                session->destroy();
            }, false, true);
        }
    }
    _conn_session.clear();
    if (_timer) {
        _thread->delTimer(_timer);
        _timer = nullptr;
    }
}

void Server::onManager() {
    for (auto& pair : _conn_session) {
        if (!pair.second->thread()->isCurrentThread()) {
            auto session = pair.second;
            _thread->dispatch([session]() {
                session->onManager();
            }, false, true);
        }
        else {
            pair.second->onManager();
        }
    }
}

void Server::onConnectionChange(Connection::Ptr conn, ConnectEvent e) {
    if (e == ConnectEvent::Closed) {
        _conn_session[conn]->onConnectionChange(conn, e);
        _conn_session.erase(conn);
    }
}

} // namespace DLNetwork 