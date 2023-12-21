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

class RotateFileLog
{
public:
    RotateFileLog(std::string baseFileName, std::string ext, int maxFiles, int maxSize) : _baseFileName(baseFileName), _ext(ext), _maxFiles(maxFiles), _maxSize(maxSize), _fp(nullptr) {
        std::string fileName = calcFileName(_baseFileName, 0);
        _curSize = getFileSize(fileName.c_str());
        if (_curSize >= _maxSize) {
            rotate();
        }
        else {
            _fp = openFile(fileName.c_str(), "ab");
            _curSize = 0;
        }
    }
    ~RotateFileLog() {
        if (_fp) {
            fclose(_fp);
        }
    }

	void write(std::string s) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_fp) {
            fwrite(s.c_str(), s.length(), 1, _fp);
            _curSize += s.length();
            if (_curSize >= _maxSize) {
                rotate();
            }
            else {
                flush();
            }
        }
	}
	void flush() {
        if (_fp) {
            fflush(_fp);
        }
	}

protected:
    std::string calcFileName(std::string& baseName, int index) {
        std::stringstream ss;
        ss << baseName << "_" << index << _ext;
        return ss.str();
    }
    // log.txt -> log.1.txt
    // log.1.txt -> log.2.txt
    // log.2.txt -> log.3.txt
    // log.3.txt -> delete
    bool rotate() {
        bool ret = true;
        //std::lock_guard<std::mutex> lock(_mutex);
        if (_fp) {
            fclose(_fp);
        }
        for (int i = _maxFiles - 1; i >= 0; i--) {
            std::string src = calcFileName(_baseFileName, i);
            if (!isFileExist(src.c_str())) {
                continue;
            }
            std::string dst = calcFileName(_baseFileName, i + 1);
            remove(dst.c_str());
            int ecode = rename(src.c_str(), dst.c_str());
            if (ecode != 0) {
                for (size_t j = 0; j < 10; j++) {
                    dst = src + "." + std::to_string(j);
                    remove(dst.c_str());
                    ecode = rename(src.c_str(), dst.c_str());
                    if (ecode == 0) {
                        break;
                    }
                }
                if (ecode != 0) {
                    ret = false;
                    printf("RotateFileLog rotate failed: %s i:%d\n", strerror(errno), i);
                    break; // cancel rotating to avoid lossing log.
                }
            }
        }
        _fp = openFile(calcFileName(_baseFileName, 0).c_str(), "ab");
        _curSize = 0;
        return ret;
    }
    void deleteOld() {
    }

protected:
    std::string _baseFileName;
    std::string _ext;
    int _maxFiles;
    int _maxSize;
    int _tmpMaxSize;
    size_t _curSize;
    FILE* _fp;
    std::mutex _mutex;
};
} //DLNetwork
