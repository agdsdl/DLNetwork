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

#include <queue>
#include <mutex>
#include <functional>
#include <thread>
#include <condition_variable>

namespace DLNetwork {
class DispatchQueue
{
	using TASK_FUN = std::function<void(void)>;
public:
	DispatchQueue();
	virtual ~DispatchQueue();
	static DispatchQueue* globalQueue();

	void start();
	void stop();
	void dispatch(const TASK_FUN& task);
	// dispatch and move
	void dispatch(TASK_FUN&& task);
	bool isCurrentThread() {
		auto id = std::this_thread::get_id();
		return _selfThreadid == id;
	}

private:
	void runloop();

	std::thread* _thread;
	std::thread::id _selfThreadid;
	std::mutex _mutex;
	std::condition_variable _cv;
	std::queue<TASK_FUN> _taskqueue;
	bool _stop;
};
} //DLNetwork

#define DELETE_THIS_LATER DispatchQueue::globalQueue()->aync([this]() {delete this; })
