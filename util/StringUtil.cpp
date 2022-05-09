#include "StringUtil.h"
#include <string.h>

using namespace DLNetwork;

void StringUtil::split(const std::string& str, std::vector<std::string>& v, const char* delimiter/* = "|"*/)
{
    if (delimiter == NULL || str.empty())
        return;

    std::string buf(str);
    size_t pos = std::string::npos;
    std::string substr;
    int delimiterlength = strlen(delimiter);
    while (true)
    {
        pos = buf.find(delimiter);
        if (pos != std::string::npos)
        {
            substr = buf.substr(0, pos);
            //if (!substr.empty())
                v.push_back(substr);

            buf = buf.substr(pos + delimiterlength);
        }
        else
        {
            //if (!buf.empty())
                v.push_back(buf);
            break;
        }
    }
}

void StringUtil::cut(const std::string& str, std::vector<std::string>& v, const char* delimiter/* = "|"*/)
{
    if (delimiter == NULL || str.empty())
        return;

    std::string buf(str);
    int delimiterlength = strlen(delimiter);
    size_t pos = buf.find(delimiter);
    if (pos == std::string::npos)
        return;

    std::string substr1 = buf.substr(0, pos);
    std::string substr2 = buf.substr(pos + delimiterlength);
    if (!substr1.empty())
        v.push_back(substr1);

    if (!substr2.empty())
        v.push_back(substr2);
}

std::string& StringUtil::replace(std::string& str, const std::string& toReplaced, const std::string& newStr)
{
    if (toReplaced.empty() || newStr.empty())
        return str;

    for (std::string::size_type pos = 0; pos != std::string::npos; pos += newStr.length())
    {
        pos = str.find(toReplaced, pos);
        if (pos != std::string::npos)
            str.replace(pos, toReplaced.length(), newStr);
        else
            break;
    }

    return str;
}

void StringUtil::trimLeft(std::string& str, char trimmed/* = ' '*/)
{
    std::string tmp = str;
    size_t length = tmp.length();
    for (size_t i = 0; i < length; ++i)
    {
        if (tmp[i] != trimmed)
        {
            str = tmp.substr(i);
            break;
        }
    }
}

void StringUtil::trimRight(std::string& str, char trimmed/* = ' '*/)
{
    std::string tmp = str;
    size_t length = tmp.length();
    for (size_t i = length - 1; i >= 0; --i)
    {
        if (tmp[i] != trimmed)
        {
            str = tmp.substr(0, i + 1);
            break;
        }
    }
}

void StringUtil::trim(std::string& str, char trimmed/* = ' '*/)
{
    trimLeft(str, trimmed);
    trimRight(str, trimmed);
}

std::string hexmem(const void* buf, size_t len) {
    std::string ret;
    char tmp[8];
    const uint8_t* data = (const uint8_t*)buf;
    for (size_t i = 0; i < len; ++i) {
        int sz = sprintf(tmp, "%.2x ", data[i]);
        ret.append(tmp, sz);
    }
    return ret;
}

bool is_safe(uint8_t b) {
    return b >= ' ' && b < 128;
}

std::string hexdump(const void* buf, size_t len) {
    std::string ret("\r\n");
    char tmp[8];
    const uint8_t* data = (const uint8_t*)buf;
    for (size_t i = 0; i < len; i += 16) {
        for (int j = 0; j < 16; ++j) {
            if (i + j < len) {
                int sz = snprintf(tmp, sizeof(tmp), "%.2x ", data[i + j]);
                ret.append(tmp, sz);
            }
            else {
                int sz = snprintf(tmp, sizeof(tmp), "   ");
                ret.append(tmp, sz);
            }
        }
        for (int j = 0; j < 16; ++j) {
            if (i + j < len) {
                ret += (is_safe(data[i + j]) ? data[i + j] : '.');
            }
            else {
                ret += (' ');
            }
        }
        ret += ('\n');
    }
    return ret;
}
