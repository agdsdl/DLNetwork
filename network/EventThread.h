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
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <list>
#include <map>
#include <unordered_map>
#include <assert.h>
#include "PipeWrap.h"
#include "TimerManager.h"

namespace DLNetwork {

enum EventType :int {
	Hangup = 1 << 0,
	Error = 1 << 1,
	Read = 1 << 2,
	Write = 1 << 3,
	ALL = Read | Write | Error | Hangup,
};
using EventHandleFun = std::function<void(int, int)>;

class EventThread
{
	using TASK_FUN = std::function<void(void)>;
	//using TIMER_FUN = std::function<int(void)>; // return next trigger timeout in ms. return 0 means don't trigger again.

	struct Event
	{
		int fd;
		int eventType;
		EventHandleFun callback;
	};

public:
	friend class EventThreadPool;
	//static EventThread* fromCurrentThread() {
	//	return new EventThread(true);
	//}
	~EventThread() {};
	void addEvent(int fd, int type, EventHandleFun&& callback);
	void modifyEvent(int fd, int type);
	void removeEvents(int fd);
	int eventCount() { return _event_map.size(); }
	void dispatch(TASK_FUN&& task, bool insertFront = false, bool tryNoQueue = false);
	Timer* addTimerInLoop(unsigned int ms, Timer::TIMER_FUN task, void* arg = NULL);
	bool delTimerInLoop(Timer* t);
	void delay(unsigned int ms, Timer::TIMER_FUN&& task, void* arg = NULL);
	bool isCurrentThread() {
		auto id = std::this_thread::get_id();
		return _selfThreadid == id;
	}

protected:
	//uint64_t processExpireTasks();
	void loopOnce();
	void runloop();
	void onPipeEvent();

	std::unordered_map<int, Event > _event_map;
	//std::multimap<uint64_t, TIMER_FUN> _delayTask;
	TimerManager _timerMan;
	std::vector<struct pollfd> _pollfds;
	std::thread _thread;
	std::thread::id _selfThreadid;
	std::mutex _taskMutex;
	std::list<TASK_FUN> _taskQueue;
	PipeWrap _pipe;
	bool _threadCancel;

private:
	EventThread(bool fromCurrentThread = false);
};

class EventThreadPool
{
public:
	EventThreadPool() {}
	~EventThreadPool() {}
	static EventThreadPool& instance();

	void init(int poolSize = 4);
	EventThread* getIdlestThread();
	EventThread* debugThread() {
		return _debugThread;
	}
private:
	std::vector<EventThread*> _threads;
	EventThread* _debugThread = nullptr;
};

} // ns DLNetwork