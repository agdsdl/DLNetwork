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

namespace DLNetwork {
class ScopeGuard {
public:
	template<class Callable>
	ScopeGuard(Callable&& fn) : fn_(std::forward<Callable>(fn)) {}
	ScopeGuard(ScopeGuard&& other) : fn_(std::move(other.fn_)) {
		other.fn_ = nullptr;
	}
	~ScopeGuard() {
		// must not throw
		if (fn_) fn_();
	}
	ScopeGuard(const ScopeGuard&) = delete;
	void operator=(const ScopeGuard&) = delete;
private:
	std::function<void()> fn_;
};
}
#ifndef CONCAT_
#define CONCAT_(a, b) a ## b
#endif // !CONCAT_

#ifndef CONCAT
#define CONCAT(a, b) CONCAT_(a,b)
#endif // !CONCAT

#ifndef DEFER
#define DEFER(fn) DLNetwork::ScopeGuard CONCAT(__defer__, __LINE__) = [&] ( ) { fn ; }
#else
#warnning DEFER aleady defined!
#endif // !DEFER

