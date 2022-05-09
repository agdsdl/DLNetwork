#include "MyTime.h"
#include <chrono>

 void DLNetwork::getCurrentMilisecondEpoch(long long& sec, long& mili) {
#if !defined(_WIN32)
	struct timeval tv;
	gettimeofday(&tv, NULL);
	sec = tv.tv_sec;
	mili = tv.tv_usec*1000LL;
#else
	 auto now = std::chrono::system_clock::now().time_since_epoch();
	 sec = std::chrono::duration_cast<std::chrono::seconds>(now).count();
	 mili = std::chrono::duration_cast<std::chrono::milliseconds>(now).count() - sec*1000;
#endif
}

