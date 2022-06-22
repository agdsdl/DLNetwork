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
#include <functional>
#include <string>

#include "TcpConnection.h"
#include "INetAddress.h"

namespace DLNetwork {
class TcpClient {
public:
	enum class State :int {
		disconnected = 0,
		connecting,
		connected
	};
	typedef std::function<void(TcpClient& cli, SOCKET sock, int ecode, std::string eMsg) > ErrorCallback;
	typedef std::function<void(TcpClient& cli, ConnectEvent e)> ConnectionCallback;
	typedef std::function<bool(TcpClient& cli, Buffer*)> MessageCallback;
	typedef std::function<void(TcpClient& cli)> WritedCallback;


	TcpClient(EventThread* loop, const INetAddress& serverAddr, const std::string& name);
	~TcpClient();
	bool startConnect();
	void write(const char* buf, size_t size);
	void close();

	void setConnectCallback(ConnectionCallback cb) {
		_connectionCb = cb;
	}
	void setOnMessage(MessageCallback cb) {
		_messageCb = cb;
	}
	void setOnWriteDone(WritedCallback cb) {
		_writedCb = cb;
	}

	void setOnConnectError(const ErrorCallback& cb) {
		_errorCb = cb;
	}

	const std::unique_ptr<TcpConnection>& connection() {
		return _conn;
	}
	SOCKET sock() {
		return _sock;
	}
	EventThread* thread() {
		return _thread;
	}
	const std::string& name() {
		return _name;
	}
	std::string description();
	uint32_t seq = 0;
	uint32_t wantSeq = 0;
private:
	void connectionCallback(TcpConnection& conn, ConnectEvent e);
	bool messageCallback(TcpConnection& conn, Buffer* buf);
	void writedCallback(TcpConnection& conn);

	void onEvent(SOCKET sock, int eventType);
	void handleRead(SOCKET sock);
	void handleWrite(SOCKET sock);
	void handleHangup(SOCKET sock);
	void handleError(SOCKET sock);

	std::unique_ptr<TcpConnection> _conn;
	ConnectionCallback _connectionCb;
	MessageCallback _messageCb;
	WritedCallback _writedCb;
	SOCKET _sock = -1;
	ErrorCallback _errorCb;
	EventThread* _thread;
	INetAddress _serverAddr;
	std::string _name;
	State _state = State::disconnected;
};

}
DLNetwork::MyOut& operator<<(DLNetwork::MyOut& o, DLNetwork::TcpClient& c);
