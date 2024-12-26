#include "StringUtil.h"
#include <string.h>
#include "NetEndian.h"

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

bool StringUtil::isEndWith(std::string const& fullString, std::string const& ending) {
    if (fullString.length() >= ending.length()) {
        return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
    }
    else {
        return false;
    }
}

std::string DLNetwork::hexmem(const void* buf, size_t len) {
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

std::string DLNetwork::hexdump(const void* buf, size_t len) {
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

std::string DLNetwork::BinToHex(const std::string& strBin, bool bIsUpper)
{
    return BinToHex((const uint8_t *)strBin.data(), strBin.size(), bIsUpper);
}

std::string DLNetwork::BinToHex(const uint8_t* buf, size_t len, bool bIsUpper)
{
    std::string strHex;
    strHex.resize(len * 2);
    for (size_t i = 0; i < len; i++) {
        uint8_t cTemp = buf[i];
        for (size_t j = 0; j < 2; j++) {
            uint8_t cCur = (cTemp & 0x0f);
            if (cCur < 10) {
                cCur += '0';
            }
            else {
                cCur += ((bIsUpper ? 'A' : 'a') - 10);
            }
            strHex[2 * i + 1 - j] = cCur;
            cTemp >>= 4;
        }
    }

    return strHex;
}

std::string DLNetwork::HexToBin(const std::string& strHex)
{
    if (strHex.size() % 2 != 0) {
        return "";
    }

    std::string strBin;
    strBin.resize(strHex.size() / 2);
    for (size_t i = 0; i < strBin.size(); i++) {
        uint8_t cTemp = 0;
        for (size_t j = 0; j < 2; j++) {
            char cCur = strHex[2 * i + j];
            if (cCur >= '0' && cCur <= '9') {
                cTemp = (cTemp << 4) + (cCur - '0');
            }
            else if (cCur >= 'a' && cCur <= 'f') {
                cTemp = (cTemp << 4) + (cCur - 'a' + 10);
            }
            else if (cCur >= 'A' && cCur <= 'F') {
                cTemp = (cTemp << 4) + (cCur - 'A' + 10);
            }
            else {
                return "";
            }
        }
        strBin[i] = cTemp;
    }

    return strBin;
}

std::string DLNetwork::ptr2string(void* p) {
    if (!p) {
        return "0x0";
    }

#if !defined(BIG_ENDIAN)
    int n = 8;
    uint8_t* ptr = (uint8_t*)&p;
    while (ptr[--n] == 0);

    char tmp[2+16];
    int k = 2;
    for (int i = n; i >= 0; i--) {
        tmp[k++] = (ptr[i] >> 4) >= 10 ? (ptr[i] >> 4)+'a'-10 : (ptr[i] >> 4) + '0';
        tmp[k++] = (ptr[i]&0xF) >= 10 ? (ptr[i] & 0xF) +'a'-10 : (ptr[i] & 0xF) + '0';
    }
    int start = 0;
    if (tmp[2] == '0') {
        start = 1;
    }
    tmp[start] = '0';
    tmp[start + 1] = 'x';
    return std::string(tmp + start, 2 + (n+1) * 2 - start);
#else
    constexpr int n = sizeof(void*);
#if (n == 8)
    p = (void*)DLNetwork::hostToNetwork64((uint64_t)p);
#elif (n == 4)
    p = (void*)DLNetwork::hostToNetwork32((uint32_t)p);
#elif (n == 2)
    p = (void*)DLNetwork::hostToNetwork16((uint16_t)p);
#endif // (sizeof(void*) == 4)

    std::string str = DLNetwork::BinToHex((const uint8_t*)&p, sizeof(p), false);
    std::string::size_type pos = str.find_first_not_of('0');
    if (pos != std::string::npos) {
        str = str.substr(pos);
    }
    return std::string("0x") + str;
#endif
}
