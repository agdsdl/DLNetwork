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
#include "EventThread.h"
#include <platform.h>
#include "sockutil.h"
#include <thread>
#include <chrono>
#include <algorithm>
#include <stdio.h>
#include <atomic>
#include <MyLog.h>
#include "uv_errno.h"
#include "ThreadName.h"

#define ToPoll(type) (type&EventType::Read ? POLLIN:0) | (type&EventType::Write ? POLLOUT:0) | (type&EventType::Error ? POLLERR:0) | (type&EventType::Hangup ? POLLHUP:0)
#define ToEventType(rev) (rev&POLLIN?EventType::Read:0) | (rev&POLLOUT?EventType::Write:0) | (rev&POLLERR?EventType::Error:0) |  (rev&POLLHUP?EventType::Hangup:0)

static inline uint64_t getCurrentMicrosecondEpoch() {
#if !defined(_WIN32)
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000LL + tv.tv_usec;
#else
	return  std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#endif
}

using namespace DLNetwork;

EventThread::EventThread() :_threadCancel(false) {
    static std::atomic<int> n(1);
    char buf[64];
    snprintf(buf, sizeof(buf), "EventThread %d", n.load(std::memory_order_acquire));
    n++;
	_thread = std::thread(std::bind(&EventThread::runloop, this));
	_selfThreadid = _thread.get_id();
    setThreadName(&_thread, buf);

	SockUtil::setNoBlocked(_pipe.readFD());
	SockUtil::setNoBlocked(_pipe.writeFD());
	//addEvent(_pipe.readFD(), EventType::Read, [this](int fd, int event) {onPipeEvent(); });
	Event ev{ _pipe.readFD(), EventType::Read, std::bind(&EventThread::onPipeEvent, this) };
	_event_map.emplace(_pipe.readFD(), ev);
}

void EventThread::addEvent(int fd, int type, EventHandleFun && callback)
{
	if (isCurrentThread()) {
		Event ev{ fd, type, callback };
		_event_map.emplace(fd, ev);
	}
	else {
		dispatch([this, fd, type, callback]() {
			addEvent(fd, type, std::move(const_cast<EventHandleFun &>(callback)));
		});
	}
}

void EventThread::modifyEvent(int fd, int type)
{
	if (isCurrentThread()) {
		auto it = _event_map.find(fd);
		if (it != _event_map.end()) {
			it->second.eventType = type;
		}
	}
	else {
		dispatch([this, fd, type]() {
			modifyEvent(fd, type);
		});
	}
}

void EventThread::removeEvents(int fd)
{
	if (isCurrentThread()) {
		_event_map.erase(fd);
	}
	else {
		dispatch([this, fd]() {
			removeEvents(fd);
		});
	}
}

void EventThread::dispatch(TASK_FUN &&task, bool insertFront, bool tryNoQueue) {
	if (tryNoQueue && isCurrentThread()) {
		return task();
	}

    {
	    std::lock_guard<std::mutex> lock(_taskMutex);
	    if (insertFront) {
		    _taskQueue.emplace_front(task);
	    }
	    else {
		    _taskQueue.emplace_back(task);
	    }
    }
	//写数据到管道,唤醒主线程
	_pipe.write("", 1);
	return;
}

Timer* EventThread::addTimerInLoop(unsigned int ms, Timer::TIMER_FUN task, void* arg) {
	assert(isCurrentThread());
	return _timerMan.addTimer(ms, std::move(task), arg);
}

bool EventThread::delTimerInLoop(Timer* t) {
	assert(isCurrentThread());
	return _timerMan.delTimer(t);
}

void EventThread::delay(unsigned int ms, Timer::TIMER_FUN && task, void* arg)
{
	dispatch([this, ms, task, arg]() {
		addTimerInLoop(ms, task, arg);
		//uint64_t now = getCurrentMicrosecondEpoch();
		//_delayTask.emplace(ms+now, task);
	}, true);
}

void EventThread::onPipeEvent() {
	char buf[1024];
	int err = 0;
	do {
		if (_pipe.read(buf, sizeof(buf)) > 0) {
			continue;
		}
		err = get_uv_error(true);
	} while (err != UV_EAGAIN);

	std::list<TASK_FUN> tmp;
	std::unique_lock<std::mutex> lk(_taskMutex);
	_taskQueue.swap(tmp);
	//TASK_FUN task = std::move(_taskQueue.front());
	//_taskQueue.pop_front();
	lk.unlock();

	for (auto& task : tmp) {
		task();
	}
}

//uint64_t EventThread::processExpireTasks()
//{
//	uint64_t now = getCurrentMicrosecondEpoch();
//	for (auto it = _delayTask.begin(); it != _delayTask.end() && it->first <= now; it = _delayTask.erase(it)) {
//		//已到期的任务
//		try {
//			auto next_delay = (it->second)();
//			if (next_delay) {
//				//可重复任务,更新时间截止线
//				_delayTask.emplace(next_delay + now, std::move(it->second));
//			}
//		}
//		catch (std::exception &ex) {
//			mCritical() << "EventThread processExpireTasks exception:" << ex.what();
//		}
//	}
//
//	if (_delayTask.size() != 0) {
//		return _delayTask.begin()->first - now;
//	}
//	else {
//		return 0;
//	}
//}

void EventThread::loopOnce()
{
	//uint64_t nextDelay = processExpireTasks();
	uint64_t nextDelay = _timerMan.processAllTimeout();

	if (_event_map.size() == 0) {
		return;
	}

	_pollfds.resize(_event_map.size());
	int i = 0;
	for (auto& evpair: _event_map) {
		_pollfds[i].fd = evpair.first;
		_pollfds[i].events = ToPoll(evpair.second.eventType);
		_pollfds[i].revents = 0;
		i++;
	}

	int ret = poll(_pollfds.data(), _pollfds.size(), nextDelay?nextDelay/1000:10000);
	if (ret > 0) {
		for (size_t i = 0; i < _pollfds.size(); i++) {
			if (_pollfds[i].revents != 0) {
				auto iter = _event_map.find(_pollfds[i].fd);
				if (iter != _event_map.end()) {
					iter->second.callback(_pollfds[i].fd, ToEventType(_pollfds[i].revents));
				}
			}
		}
	}
	else if (ret < 0) {
		//error
		mWarning() << "EventThread::loopOnce poll error:" << get_uv_errmsg();
	}
	else {
		//mWarning() << "EventThread::loopOnce poll error:" << myerrno;
		//timeout
	}
}

void EventThread::runloop()
{
	while (!_threadCancel) {
		if (_event_map.size() == 0) {
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			continue;
		}
		loopOnce();
	}
}

void EventThreadPool::init(int poolSize)
{
	_threads.reserve(poolSize);
	for (size_t i = 0; i < poolSize; i++) {
		EventThread *t = new EventThread();
		_threads.push_back(t);
	}
    _debugThread = new EventThread();
}

EventThreadPool& EventThreadPool::instance()
{
	static EventThreadPool _theadpool;
	return _theadpool;
}

EventThread * EventThreadPool::getIdlestThread()
{
	int minCount = _threads[0]->eventCount();
	EventThread *ret = _threads[0];
	for (auto t : _threads) {
		if (t->eventCount() < minCount) {
			minCount = t->eventCount();
			ret = t;
		}
	}
	return ret;
}
