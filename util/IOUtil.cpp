#include "IOUtil.h"
#include <string.h>
#include <thread>
#include <chrono>
#include "platform.h"

using namespace DLNetwork;

// Return true if path exists (file or directory)
bool DLNetwork::isPathExists(const std::string& filename)
{
#ifdef _WIN32
#    ifdef _WCHAR_
    auto attribs = ::GetFileAttributesW(filename.c_str());
#    else
    auto attribs = ::GetFileAttributesA(filename.c_str());
#    endif
    return attribs != INVALID_FILE_ATTRIBUTES;
#else // common linux/unix all have the stat system call
    struct stat buffer;
    return (::stat(filename.c_str(), &buffer) == 0);
#endif
}

// Return directory name from given path or empty string
// "abc/file" => "abc"
// "abc/" => "abc"
// "abc" => ""
// "abc///" => "abc//"
std::string DLNetwork::dirName(const std::string& path)
{
    auto pos = path.find_last_of(DLNETWORK_PATH_SEP);
    return pos != std::string::npos ? path.substr(0, pos) : std::string{};
}

// return true on success
static bool mkdir(const std::string& path)
{
#ifdef _WIN32
#    ifdef _WCHAR_
    return ::_wmkdir(path.c_str()) == 0;
#    else
    return ::_mkdir(path.c_str()) == 0;
#    endif
#else
    return ::mkdir(path.c_str(), mode_t(0755)) == 0;
#endif
}

// create the given directory - and all directories leading to it
// return true on success or if the directory already exists
bool DLNetwork::createDir(const std::string& path)
{
    if (isPathExists(path)) {
        return true;
    }

    if (path.empty()) {
        return false;
    }

    size_t search_offset = 0;
    do {
        auto token_pos = path.find_first_of(DLNETWORK_PATH_SEP, search_offset);
        // treat the entire path as a folder if no folder separator not found
        if (token_pos == std::string::npos) {
            token_pos = path.size();
        }

        auto subdir = path.substr(0, token_pos);

        if (!subdir.empty() && !isPathExists(subdir) && !mkdir(subdir)) {
            return false; // return error if failed creating dir
        }
        search_offset = token_pos + 1;
    } while (search_offset < path.size());

    return true;
}

size_t DLNetwork::getFileSize(FILE* f)
{
    if (f == nullptr) {
        return 0;
    }
#if defined(_WIN32) && !defined(__CYGWIN__)
    int fd = ::_fileno(f);
#    if defined(_WIN64) // 64 bits
    __int64 ret = ::_filelengthi64(fd);
    if (ret >= 0) {
        return static_cast<size_t>(ret);
    }

#    else // windows 32 bits
    long ret = ::_filelength(fd);
    if (ret >= 0) {
        return static_cast<size_t>(ret);
    }
#    endif

#else // unix
    // OpenBSD and AIX doesn't compile with :: before the fileno(..)
#    if defined(__OpenBSD__) || defined(_AIX)
    int fd = fileno(f);
#    else
    int fd = ::fileno(f);
#    endif
    // 64 bits(but not in osx or cygwin, where fstat64 is deprecated)
#    if (defined(__sun) || defined(_AIX)) && (defined(__LP64__) || defined(_LP64))
    struct stat64 st;
    if (::fstat64(fd, &st) == 0) {
        return static_cast<size_t>(st.st_size);
    }
#    else // other unix or linux  or cygwin
    struct stat st;
    if (::fstat(fd, &st) == 0) {
        return static_cast<size_t>(st.st_size);
    }
#    endif
#endif
    return 0; // will not be reached.
}

size_t DLNetwork::getFileSize(const char* filePath) {
    FILE* fp = fopen(filePath, "rb");
    if (!fp) {
        return 0;
    }
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    fclose(fp);
    return size;
}

bool DLNetwork::isFileExist(const char* filePath) {
    FILE* fp = fopen(filePath, "rb");
    if (!fp) {
        return false;
    }
    fclose(fp);
    return true;
}

FILE* DLNetwork::openFile(const std::string& path, const char* mode)
{
    for (int tries = 0; tries < 5; ++tries) {
        // create containing folder if not exists already.
        createDir(dirName(path));
        FILE* fp = fopen(path.c_str(), mode);
        if (fp) {
            return fp;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return nullptr;
}

