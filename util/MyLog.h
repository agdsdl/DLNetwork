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
class MyLog
{
public:
	template<typename T>
	static void output(T&& s) {
		std::cout << s;
		fflush(stdout);
	}
};

class MyOut {
public:
	MyOut(const char* file, int line, char level) {
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

		_ss << tmp << " [" << level << "] [" << file << ":" << line << "] [" << tid << "] ";
	}
	~MyOut() {
		_ss << "\n";
		MyLog::output(_ss.str());
	}
	template<typename T>
	MyOut& operator<<(T&& t) {
		_ss << t << ' ';
		return *this;
	}
private:
	std::stringstream _ss;
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

#define mDebug() DLNetwork::MyOut(__FILENAME__, __LINE__, 'D')
#define mInfo() DLNetwork::MyOut(__FILENAME__, __LINE__, 'I')
#define mWarning() DLNetwork::MyOut(__FILENAME__, __LINE__, 'W')
#define mCritical() DLNetwork::MyOut(__FILENAME__, __LINE__, 'E')
