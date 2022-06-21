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
#include "AsyncSocket.h"
#include "MyLog.h"
#include "uv_errno.h"
#include <assert.h>
#include <thread>

using namespace DLNetwork;

AsyncSocket::AsyncSocket(SOCKET sock, EventThread * thread, size_t bufSize, size_t sockSize) :
    _sock(sock), _thread(thread), _eventType(0), _bufSize(bufSize), _sendBuf(new char[bufSize]), _ringSendBuf(_sendBuf, bufSize),
    _readCallback(nullptr), _writeCallback(nullptr), _errorCallback(nullptr), _hangupCallback(nullptr) {
    SockUtil::setNoBlocked(sock, true);
    int nSendBuf = sockSize;
    SockUtil::setSendBuf(sock, nSendBuf);
    //SockUtil::setRecvLowWaterMark(sock, 16);
    _thread->addEvent(sock, 0, std::bind(&AsyncSocket::onEvent, this, std::placeholders::_1, std::placeholders::_2));
}

AsyncSocket::~AsyncSocket() {
	mDebug() << "~AsyncSocket" << _sock;
	if (_sendBuf) {
		delete[] _sendBuf;
		_sendBuf = nullptr;
	}
	if (_sock) {
		_thread->removeEvents(_sock);
		myclose(_sock);
	}
}

int AsyncSocket::notSendDataSize()
{
	std::unique_lock<std::mutex> lock(_bufMutex);
	return _ringSendBuf.dataSize();
}

int AsyncSocket::send(const char * buf, size_t size, bool shouldBlock) {
    if (shouldBlock) {
        int ret = 0;
        int n = 0;
        while(n < size)
        {
            ret = ::send(_sock, buf+n, size-n, 0);
            if (ret < 0) {
                int ecode = get_uv_error();
                if (ecode == UV_EAGAIN) {
					std::this_thread::sleep_for(std::chrono::milliseconds(20));
                }
                else {
                    mWarning() << "AsyncSocket::send error:" << get_uv_errmsg();
                    return ret;
                }
            }
            else {
                n += ret;
            }
        }
        return n;
    }
    else {
        std::unique_lock<std::mutex> lock(_bufMutex);
        if (_ringSendBuf.remainSize() >= size) {
            _ringSendBuf.write(buf, size);
            this->enableWriteNotify(true);
            //std::weak_ptr<AsyncSocket> weakThis(this->shared_from_this());
            //_thread->dispatch([weakThis]() {
            //	if (auto strongThis = weakThis.lock()) {
            //		strongThis->flushBuf();
            //	}
            //});
            return size;
        }
        else {
            mCritical() << "AsyncSocket::send buf full!";
            return -99;
        }
    }

	//if (_ringSendBuf.dataSize() > 0) {
	//	std::unique_lock<std::mutex> lock(_bufMutex);
	//	if (_ringSendBuf.remainSize() > size) {
	//		_ringSendBuf.write(buf, size);
	//		this->enableWriteNotify(true);
	//		_thread->dispatch([this]() {
	//			flushBuf();
	//		});
	//		return size;
	//	}
	//	else {
	//		mCritical() << "AsyncSocket::send buf full!";
	//		return -1;
	//	}
	//}

	//int ret = ::send(_sock, buf, size, 0);
	//if (ret == -1) {
	//	int ecode = get_uv_error();
	//	if (ecode == UV_EAGAIN) {
	//		std::unique_lock<std::mutex> lock(_bufMutex);
	//		if (_ringSendBuf.remainSize() < size) {
	//			mCritical() << "AsyncSocket::send buf full!";
	//			return -1;
	//		}
	//		else {
	//			_ringSendBuf.write(buf, size);
	//			this->enableWriteNotify(true);
	//			_thread->dispatch([this]() {
	//				flushBuf();
	//			});
	//			return size;
	//		}
	//	}
	//	else {
	//		mCritical() << "AsyncSocket::send error:" << get_uv_errmsg();
	//		return -1;
	//	}
	//}
	//else {
	//	return ret;
	//}
}
void AsyncSocket::onEvent(SOCKET sock, int eventType) {
	if (_hangupCallback && eventType&EventType::Hangup) {
		_hangupCallback(this, EventType::Hangup);
	}
	if (_errorCallback && eventType&EventType::Error) {
		_errorCallback(this, EventType::Error);
	}
    if (_readCallback && eventType&EventType::Read) {
        _readCallback(this, EventType::Read);
    }
    if (eventType&EventType::Write) {
        onWritable();
        if (_writeCallback) {
            _writeCallback(this, EventType::Write);
        }
    }
}

void AsyncSocket::onWritable()
{
	flushBuf();
}

int AsyncSocket::flushBuf()
{
	int ret = 0;
	std::unique_lock<std::mutex> lock(_bufMutex);
	while (_ringSendBuf.dataSize() > 0) {
		int nSend;
		int size = 0;
		char *buf = _ringSendBuf.firstSegment(&size);
		assert(size > 0);
		nSend = ::send(_sock, buf, size, 0);
		if (nSend < 0) {
			int ecode = get_uv_error();
			if (ecode == UV_EAGAIN) {
				this->enableWriteNotify(true);
			}
			else {
				mCritical() << "AsyncSocket::flushBuf error:" << get_uv_errmsg();
				ret = -1;
			}
			break;
		}
		else {
			_ringSendBuf.consume(nSend);
			ret += nSend;
		}
	}
	if (_ringSendBuf.dataSize() == 0) {
		this->enableWriteNotify(false);
	}
	
	return ret;
}
