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

#include <platform.h>
#include <stdint.h>

namespace DLNetwork
{
    inline uint64_t hostToNetwork64(uint64_t host64)
    {
#ifdef WIN32
        return htonll(host64);
#else
        return htobe64(host64);
#endif
    }

    inline uint32_t hostToNetwork32(uint32_t host32)
    {
#ifdef WIN32
        return htonl(host32);
#else
        return htobe32(host32);
#endif
    }

    inline uint16_t hostToNetwork16(uint16_t host16)
    {
#ifdef WIN32		
        return htons(host16);
#else
        return htobe16(host16);
#endif
    }

    inline uint64_t networkToHost64(uint64_t net64)
    {
#ifdef WIN32
        return ntohll(net64);
#else
        return be64toh(net64);
#endif
    }

    inline uint32_t networkToHost32(uint32_t net32)
    {
#ifdef WIN32
        return ntohl(net32);
#else
        return be32toh(net32);
#endif
    }

    inline uint16_t networkToHost16(uint16_t net16)
    {
#ifdef WIN32
        return ntohs(net16);
#else
        return be16toh(net16);
#endif
    }

}// end namespace DLNetwork
