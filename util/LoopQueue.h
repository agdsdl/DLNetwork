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