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
#include "DispatchQueue.h"
#include "ThreadName.h"
#include <atomic>

using namespace DLNetwork;

DispatchQueue::DispatchQueue():_stop(false),_thread(nullptr)
{
	start();
}


DispatchQueue::~DispatchQueue()
{
	stop();
}

DispatchQueue * DispatchQueue::globalQueue()
{
	static DispatchQueue _queue;
	//static std::once_flag once;
	//std::call_once(once, []() {
	//	_queue.start();
	//});

	return &_queue;;
}

void DispatchQueue::start()
{
    static std::atomic<int> n(1);
    char buf[64];
    snprintf(buf, sizeof(buf), "DispatchQueue %d", n.load(std::memory_order_acquire));
    n++;
	_thread = new std::thread(&DispatchQueue::runloop, this);
    setThreadName(_thread, buf);
	_selfThreadid = _thread->get_id();
}

void DispatchQueue::stop()
{
	_stop = true;
	if (_thread)
	{
		_thread->join();
		delete _thread;
		_thread = nullptr;
	}
}

void DispatchQueue::dispatch(const TASK_FUN & task)
{
	std::unique_lock<std::mutex> lock(_mutex);
	_taskqueue.push(task);
	lock.unlock();
	_cv.notify_all();
}

void DispatchQueue::dispatch(TASK_FUN &&task)
{
	std::unique_lock<std::mutex> lock(_mutex);
	_taskqueue.emplace(task);
	lock.unlock();
	_cv.notify_all();
}

void DispatchQueue::runloop()
{
	while (!_stop)
	{
		std::unique_lock<std::mutex> lk(_mutex);
		_cv.wait_for(lk, std::chrono::duration<int, std::milli>(10), [this] {return _taskqueue.size() > 0; });
		if (_stop) break;

		if (_taskqueue.size() <= 0) continue;
		TASK_FUN task = std::move(_taskqueue.front());
		_taskqueue.pop();
		lk.unlock();

		task();
	}
}
