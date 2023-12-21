#ifndef __kuma_utils_h__
#define __kuma_utils_h__


#include <string>

namespace DLNetwork {

    inline uint32_t decode_u32(const uint8_t* src)
    {
        return (src[0] << 24) | (src[1] << 16) | (src[2] << 8) | src[3];
    }

    inline uint32_t decode_u24(const uint8_t* src)
    {
        return (src[0] << 16) | (src[1] << 8) | src[2];
    }

    inline uint16_t decode_u16(const uint8_t* src)
    {
        return (src[0] << 8) | src[1];
    }

    inline void encode_u32(uint8_t* dst, uint32_t u)
    {
        dst[0] = u >> 24;
        dst[1] = u >> 16;
        dst[2] = u >> 8;
        dst[3] = u;
    }

    inline void encode_u24(uint8_t* dst, uint32_t u)
    {
        dst[0] = u >> 16;
        dst[1] = u >> 8;
        dst[2] = u;
    }

    inline void encode_u16(uint8_t* dst, uint32_t u)
    {
        dst[0] = u >> 8;
        dst[1] = u;
    }
}

#endif
