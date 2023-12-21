#ifndef __STRING_UTIL_H__
#define __STRING_UTIL_H__
#include <string>
#include <vector>
#include <stdio.h>

// folder separator
#if !defined(DLNETWORK_PATH_SEP)
#    ifdef _WIN32
#        define DLNETWORK_PATH_SEP '\\/'
#    else
#        define DLNETWORK_PATH_SEP '/'
#    endif
#endif

namespace DLNetwork {
bool isPathExists(const std::string& filename);
std::string dirName(const std::string& path);
bool createDir(const std::string& path);
size_t getFileSize(FILE* f);
size_t getFileSize(const char* filePath);
bool isFileExist(const char* filePath);
FILE* openFile(const std::string& path, const char* mode);
}
#endif //!__STRING_UTIL_H__