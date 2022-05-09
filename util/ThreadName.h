#pragma once
#include <thread>

namespace DLNetwork {
void setThreadName(std::thread* thread, const char* threadName);
void setThreadName(const char* threadName);
} // DLNetwork