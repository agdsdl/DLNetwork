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
#include <mutex>
#include "platform.h"
#include "IOUtil.h"

namespace DLNetwork {

class DailyFileLog
{
public:
    DailyFileLog(std::string baseFileName, std::string ext, int maxFiles) : _baseFileName(baseFileName), _ext(ext), _maxFiles(maxFiles) {
        time_t now = time(NULL);
        struct tm tm;
        _localtime_s(&tm, &now);
        _curFileName = calcFileName(_baseFileName, tm);
        _fp = openFile(_curFileName.c_str(), "ab");
        _nextRotateTime = nextRotateTime(tm);
    }
    ~DailyFileLog() {
        if (_fp) {
            fclose(_fp);
        }
    }
	void write(std::string s) {
        bool rotated = false;
        std::lock_guard<std::mutex> lock(_mutex);
        time_t now = time(NULL);
        if (now >= _nextRotateTime) {
            rotated = true;
            struct tm tm;
            _localtime_s(&tm, &now);
            _curFileName = calcFileName(_baseFileName, tm);
            if (_fp) {
                fclose(_fp);
            }
            _fp = openFile(_curFileName.c_str(), "ab");
            _nextRotateTime = nextRotateTime(tm);
        }
        if (_fp) {
            if (s.length() != fwrite(s.c_str(), 1, s.length(), _fp)) {
                std::cout << "write log file failed!" << std::endl;
            }
        }
        flush();
        if (rotated && _maxFiles > 0) {
            deleteOld();
        }
	}
	void flush() {
        if (_fp) {
            fflush(_fp);
        }
	}

protected:
    inline int _localtime_s(struct tm* buf, const time_t* timer) {
#ifdef _WIN32
        return ::localtime_s(buf, timer);
#else
        return ::localtime_r(timer, buf) != NULL;
#endif
    }

    std::string calcFileName(std::string& baseName, tm& time) {
        char buf[64] = { NULL };
        strftime(buf, sizeof(buf), "%Y-%m-%d", &time);
        return baseName + "_" + buf + _ext;
    }
	time_t nextRotateTime() {
        time_t now = time(NULL);
        struct tm tm;
        _localtime_s(&tm, &now);
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        return mktime(&tm) + 24 * 60 * 60;
    }
    time_t nextRotateTime(tm& tm) {
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        return mktime(&tm) + 24 * 60 * 60;
    }
    void deleteOld() {
        time_t now = time(NULL);
        struct tm tm;
        _localtime_s(&tm, &now);
        tm.tm_hour = 0;
        tm.tm_min = 0;
        tm.tm_sec = 0;
        time_t today = mktime(&tm);
        time_t old = today - _maxFiles * 24 * 60 * 60;
        for (int i = 0; i < 10; i++) {
            _localtime_s(&tm, &old);
            std::string fileName = calcFileName(_baseFileName, tm);
            if (true) {
                remove(fileName.c_str());
            }
            old -= 24 * 60 * 60;
        }
    }

protected:
    time_t _nextRotateTime;
    std::string _baseFileName;
    std::string _ext;
    std::string _curFileName;
    int _maxFiles;
    FILE* _fp;
    std::mutex _mutex;
};
} //DLNetwork
