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

#include "platform.h"
#include <functional>
#include "EventThread.h"
#include "sockutil.h"
#include <RingBuffer.h>
#include <mutex>
#include <memory>

namespace DLNetwork {

class AsyncSocket;
using AsyncSocketEventHandler = std::function<void(AsyncSocket*, int)>;

class AsyncSocket : public std::enable_shared_from_this<AsyncSocket>
{
public:
	enum {
		DEFAULT_SEND_BUF_SIZE = 1 * 1024 * 1024,
	};

	AsyncSocket(SOCKET sock, EventThread* thread, size_t bufSize, size_t sockSize = 4 * 1024 * 1024);
	~AsyncSocket();

	SOCKET sock() { return _sock; }
	int notSendDataSize();
	int send(const char* buf, size_t size, bool shouldBlock = false);
	int recv(char* buf, size_t size) {
		return ::recv(_sock, buf, size, 0);
	}
	void setReadCallback(AsyncSocketEventHandler readCallback) {
		_thread->dispatch([this, readCallback]() {
			_readCallback = readCallback;
			}, false, true);
	}
	void setWriteCallback(AsyncSocketEventHandler writeCallback) {
		_thread->dispatch([this, writeCallback]() {
			_writeCallback = writeCallback;
			}, false, true);
	}
	void setErrorCallback(AsyncSocketEventHandler errorCallback) {
		_thread->dispatch([this, errorCallback]() {
			_errorCallback = errorCallback;
			}, false, true);
	}
	void setHangupCallback(AsyncSocketEventHandler hangupCallback) {
		_thread->dispatch([this, hangupCallback]() {
			_hangupCallback = hangupCallback;
			}, false, true);
	}
	void enableReadNotify(bool enable) {
		if (enable) {
			_eventType |= EventType::Read;
		}
		else {
			_eventType &= ~EventType::Read;
		}
		_thread->modifyEvent(_sock, _eventType);
	}
	void enableWriteNotify(bool enable) {
		if (enable) {
			_eventType |= EventType::Write;
		}
		else {
			_eventType &= ~EventType::Write;
		}
		_thread->modifyEvent(_sock, _eventType);
	}
	void enableErrorNotify(bool enable) {
		if (enable) {
			_eventType |= EventType::Error;
		}
		else {
			_eventType &= ~EventType::Error;
		}
		_thread->modifyEvent(_sock, _eventType);
	}
	void enableHangupNotify(bool enable) {
		if (enable) {
			_eventType |= EventType::Hangup;
		}
		else {
			_eventType &= ~EventType::Hangup;
		}
		_thread->modifyEvent(_sock, _eventType);
	}
	int flushBuf();

private:
	void onEvent(SOCKET sock, int eventType);
	void onWritable();
	size_t _bufSize = DEFAULT_SEND_BUF_SIZE;
	char* _sendBuf;
	RingBuffer _ringSendBuf;
	std::mutex _bufMutex;
	EventThread* _thread;
	SOCKET _sock;
	int _eventType;

	AsyncSocketEventHandler _readCallback;
	AsyncSocketEventHandler _writeCallback;
	AsyncSocketEventHandler _errorCallback;
	AsyncSocketEventHandler _hangupCallback;
};

}//ns DLNetwork