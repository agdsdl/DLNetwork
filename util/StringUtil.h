#ifndef __STRING_UTIL_H__
#define __STRING_UTIL_H__
#include <string>
#include <vector>

namespace DLNetwork {
class StringUtil
{
private:
    StringUtil() = delete;
    ~StringUtil() = delete;
    StringUtil(const StringUtil& rhs) = delete;
    StringUtil& operator=(const StringUtil& rhs) = delete;

public:
    static void split(const std::string& str, std::vector<std::string>& v, const char* delimiter = "|");
    //根据delimiter指定的字符串，将str切割成两半
    static void cut(const std::string& str, std::vector<std::string>& v, const char* delimiter = "|");
    static std::string& replace(std::string& str, const std::string& toReplaced, const std::string& newStr);

    static void trimLeft(std::string& str, char trimmed = ' ');
    static void trimRight(std::string& str, char trimmed = ' ');
    static void trim(std::string& str, char trimmed = ' ');
};

std::string hexmem(const void* buf, size_t len);
std::string hexdump(const void* buf, size_t len);

std::string BinToHex(const std::string& strBin, bool bIsUpper);
std::string BinToHex(const uint8_t* buf, size_t len, bool bIsUpper);
std::string HexToBin(const std::string& strHex);

} //DLNetwork
#define strstartswith(p, prefix)	(strncmp(p, prefix, strlen(prefix))? 0 : 1)
#define strendswith(p, suffix)		(strncmp(p+strlen(p)-strlen(suffix), suffix, strlen(suffix))? 0 : 1)

#endif //!__STRING_UTIL_H__