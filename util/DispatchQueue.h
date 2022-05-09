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
