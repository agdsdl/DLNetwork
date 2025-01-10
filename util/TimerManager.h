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
#include <iostream>
#include <algorithm>
#include <queue>
#include <vector>
#include <chrono>
#include "platform.h"

namespace DLNetwork {

//禁止拷贝基类
class noncopyable {
protected:
    noncopyable() {}
    ~noncopyable() {}
private:
    //禁止拷贝
    noncopyable(const noncopyable &that) = delete;
    noncopyable(noncopyable &&that) = delete;
    noncopyable &operator=(const noncopyable &that) = delete;
    noncopyable &operator=(noncopyable &&that) = delete;
};

template<class R, class... ArgTypes>
class TaskCancelable : public noncopyable {
public:
    using Ptr = std::shared_ptr<TaskCancelable>;
    using func_type = std::function<R(ArgTypes...)>;

    ~TaskCancelable() = default;

    //template<typename FUNC>
    TaskCancelable(func_type &&task) {
        _strongTask = std::make_shared<func_type>(std::forward<func_type>(task));
        _weakTask = _strongTask;
    }

    void cancel() {
        _strongTask = nullptr;
    }

    operator bool() const {
        return _strongTask && *_strongTask;
    }

    void operator=(std::nullptr_t) {
        _strongTask = nullptr;
    }

    R operator()(ArgTypes ...args) const {
        auto strongTask = _weakTask.lock();
        if (strongTask && *strongTask) {
            return (*strongTask)(std::forward<ArgTypes>(args)...);
        }
        return defaultValue<R>();
    }

    template<typename T>
    static typename std::enable_if<std::is_void<T>::value, void>::type
    defaultValue() {}

    template<typename T>
    static typename std::enable_if<std::is_pointer<T>::value, T>::type
    defaultValue() {
        return nullptr;
    }

    template<typename T>
    static typename std::enable_if<std::is_integral<T>::value, T>::type
    defaultValue() {
        return 0;
    }

protected:
    std::weak_ptr<func_type> _weakTask;
    std::shared_ptr<func_type> _strongTask;
};

class Timer : public TaskCancelable<int, void*>
{
public:
    friend class TimerManager;
    using TIMER_FUN = std::function<int(void*)>; // return next trigger timeout in ms. return 0 means don't trigger again.

    Timer(unsigned long long expire, TIMER_FUN fun, void* args)
        : TaskCancelable<int, void*>(std::move(fun)), expire(expire), args(args) {
    }

    inline int active() { return (*this)(args); }

    inline unsigned long long getExpire() const { return expire; }

protected:
    TIMER_FUN fun;
    void* args;

    unsigned long long expire;
};


class TimerManager
{
public:
    TimerManager() {}

    Timer* addTimer(unsigned int timeout, Timer::TIMER_FUN fun, void* args = NULL) {
        unsigned long long now = getCurrentMillisecs();
        Timer* timer = new Timer(now + timeout, fun, args);
        _queue.push(timer);

        return timer;
    }

    void addTimer(Timer* timer) {
        _queue.push(timer);
    }

    bool delTimer(Timer* timer) {
        std::priority_queue<Timer*, std::vector<Timer*>, cmp> newqueue;

        bool found = false;
        while (!_queue.empty()) {
            Timer* top = _queue.top();
            _queue.pop();
            if (top != timer) {
                newqueue.push(top);
            }
            else {
                found = true;
            }
        }
        _queue = newqueue;
        if (found) {
            delete timer;
        }
        return found;
    }

    unsigned long long getRecentTimeout() {
        unsigned long long timeout = -1;
        if (_queue.empty())
            return timeout;

        unsigned long long now = getCurrentMillisecs();
        timeout = _queue.top()->getExpire() - now;
        if (timeout < 0)
            timeout = 0;

        return timeout;
    }

    unsigned long long  processAllTimeout() {
        unsigned long long now = getCurrentMillisecs();

        while (!_queue.empty()) {
            Timer* timer = _queue.top();
            if (timer->getExpire() <= now) {
                _queue.pop();
                int to = timer->active();
                if (to > 0) {
                    timer->expire = now + to;
                    _queue.push(timer);
                }
                else {
                    delete timer;
                }
            }
            else {
                return timer->getExpire() - now;
            }
        }
        return 0;
    }

    unsigned long long getCurrentMillisecs() {
#if !defined(_WIN32)
        struct timeval tv;
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000LL + tv.tv_usec / 1000;
#else
        return  std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
#endif
    }

private:

    struct cmp
    {
        bool operator()(Timer*& lhs, Timer*& rhs) const { return lhs->getExpire() > rhs->getExpire(); }
    };

    std::priority_queue<Timer*, std::vector<Timer*>, cmp> _queue;
};

} //DLNetwork