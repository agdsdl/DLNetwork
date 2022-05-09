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

