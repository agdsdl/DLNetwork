#pragma once
#include <string>
#include <map>

namespace DLNetwork {
bool getUriParams(const char* uri, std::string& streamType, std::string& id, std::map<std::string, std::string>& params, std::string& subPath);
}