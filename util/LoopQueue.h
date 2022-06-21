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
#include <queue>
#include <mutex>

namespace DLNetwork {
using TASK_FUN = std::function<void(void)>;

class LoopQueue
{
public:
	LoopQueue() {};
	~LoopQueue() {};
	void aync(TASK_FUN&& task) {
		std::lock_guard<std::mutex> lock(_mutex);
		_taskqueue.emplace(task);
	}

	void runOnce() {
		std::vector<TASK_FUN> tasks;
		std::unique_lock<std::mutex> lk(_mutex);
		while (_taskqueue.size() > 0) {
			tasks.emplace_back(std::move(_taskqueue.front()));
			_taskqueue.pop();
		}
		lk.unlock();
		for (TASK_FUN& fun : tasks) {
			fun();
		}
	}
protected:
	std::mutex _mutex;
	std::queue<TASK_FUN> _taskqueue;
};

} // DLNetwork