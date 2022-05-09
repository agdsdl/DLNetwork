#include <string>

namespace DLNetwork {
std::string base64_encode(unsigned char* bytes_to_encode, unsigned int in_len);
std::string base64_decode(std::string const& s);


size_t base64_encode(char* target, const void* source, size_t bytes);
}