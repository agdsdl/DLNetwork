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
#include <sstream>
#include <thread>
#include <time.h>
#include "MyTime.h"
#include <iostream>
#include <string.h>
#include "platform.h"

#ifndef __FILENAME__
#ifdef WIN32
#define __FILENAME__ (strrchr(__FILE__, '\\') ? (strrchr(__FILE__, '\\') + 1):__FILE__)
#else
#define __FILENAME__ (strrchr(__FILE__, '/') ? (strrchr(__FILE__, '/') + 1):__FILE__)
#endif // WIN32
#endif // !__FILENAME__

namespace DLNetwork {
enum class MyLogLevel : int {
    Debug = 1,
    Info = 2,
    Warning = 3,
    Critical = 4,
};
extern MyLogLevel g_myLevel;
static void setMyLogLevel(MyLogLevel level) {
	g_myLevel = level;
}
static MyLogLevel getMyLogLevel() {
    return g_myLevel;
}
class MyLog
{
public:
	typedef void(*OutputFunc)(const std::string& s);

	static void output(std::string s) {
		if (_output) {
			_output(s);
		}
		else {
			std::cout << s;
			fflush(stdout);
		}
	}

	static void setOutputFunc(OutputFunc func) {
        _output = func;
    }
private:
	static OutputFunc _output;
};

class MyOut {
public:
	MyOut(const char* file, int line, char type, int level) : _level(level) {
		if (level < (int)getMyLogLevel()) {
            return;
        }
		long long sec;
		long mili;
		getCurrentMilisecondEpoch(sec, mili);
		time_t secT = sec;
		char tmp[64] = { NULL };
		int len = strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S.000", localtime(&secT));
		strcpy(tmp + len - 3, std::to_string(mili).substr(0, 3).c_str());
		int tid;
#ifdef WIN32
		tid = GetCurrentThreadId();
#else
		tid = syscall(SYS_gettid);
#endif // WIN32

		_ss << "[" << tmp << "] [" << type << "] [" << file << ":" << line << "] [" << tid << "] ";
	}
	~MyOut() {
		if (_level < (int)getMyLogLevel()) {
			return;
		}
		_ss << "\n";
		MyLog::output(_ss.str());
	}
	template<typename T>
	MyOut& operator<<(T&& t) {
		if (_level < (int)getMyLogLevel()) {
			return * this;
		}
		_ss << t << ' ';
		return *this;
	}
private:
	std::stringstream _ss;
	int _level;
};

class NullOut {
public:
	NullOut(const char* file, int line, char level) {
	}
	template<typename T>
	NullOut& operator<<(T&& t) {
	}
};
} //DLNetwork

//#pragma warning(disable:4172)
//inline MyOut __mylogger(const char* f, int l, char lev) {
//	MyOut out(f, l, lev);
//	return out;
//}
//#pragma warning(default:4172)
//
//#define mDebug() __mylogger(__FILE__, __LINE__, 'D')
//#define mInfo() __mylogger(__FILE__, __LINE__, 'I')
//#define mWarning() __mylogger(__FILE__, __LINE__, 'W')
//#define mCritical() __mylogger(__FILE__, __LINE__, 'E')

#define mDebug() DLNetwork::MyOut(__FILENAME__, __LINE__, 'D', 1)
#define mInfo() DLNetwork::MyOut(__FILENAME__, __LINE__, 'I', 2)
#define mWarning() DLNetwork::MyOut(__FILENAME__, __LINE__, 'W', 3)
#define mCritical() DLNetwork::MyOut(__FILENAME__, __LINE__, 'E', 4)
//#define mDebug() ((DLNetwork::getMyLogLevel()<=DLNetwork::MyLogLevel::Debug) ? DLNetwork::MyOut(__FILENAME__, __LINE__, 'D') : DLNetwork::NullOut(__FILENAME__, __LINE__, 'D'))
//#define mInfo() ((DLNetwork::getMyLogLevel()<=DLNetwork::MyLogLevel::Info) ? DLNetwork::MyOut(__FILENAME__, __LINE__, 'I') : DLNetwork::NullOut(__FILENAME__, __LINE__, 'I'))
//#define mWarning() ((DLNetwork::getMyLogLevel()<=DLNetwork::MyLogLevel::Warning) ? DLNetwork::MyOut(__FILENAME__, __LINE__, 'W') : DLNetwork::NullOut(__FILENAME__, __LINE__, 'W'))
//#define mCritical() ((DLNetwork::getMyLogLevel()<=DLNetwork::MyLogLevel::Critical) ? DLNetwork::MyOut(__FILENAME__, __LINE__, 'E') : DLNetwork::NullOut(__FILENAME__, __LINE__, 'E'))
