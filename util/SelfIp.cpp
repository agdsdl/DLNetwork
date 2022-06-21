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
#include "SelfIp.h"
#include "platform.h"
#include <iostream>
#include <string.h>

std::string DLNetwork::getSelfIp(const char *serverIp)
{
    std::string localIp;
    SOCKET fsock = 0;
    struct addrinfo hints, *res;
    do {
        //sockaddr_in serverAddr;
        memset(&hints, 0, sizeof(hints));
        hints.ai_flags = AI_ADDRCONFIG | AI_NUMERICSERV;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_family = AF_INET;
        hints.ai_protocol = IPPROTO_UDP;
        int ecode;
        ecode = getaddrinfo(serverIp, "5060", &hints, &res);
        if (ecode != 0) {
            const char *errMsg = gai_strerror(ecode);
            std::cout << "sip server ip error:" << serverIp << errMsg;
            break;
        }
        if (!res) {
            std::cout << "getaddrinfo got 0 addr of" << serverIp;
            break;
        }


        //if (inet_pton(AF_INET, serverIp, &serverAddr.sin_addr) <= 0) {
        //	mCritical() << "sip server ip error:" << serverIp << myerrno;
        //	break;
        //}
        fsock = socket(AF_INET, SOCK_DGRAM, 0);
        if (fsock < 0) {
            std::cout << "init sip sock error";
            break;
        }
        //serverAddr.sin_family = AF_INET;
        //serverAddr.sin_port = serverPort;
        ecode = connect(fsock, res->ai_addr, res->ai_addrlen);
        if (ecode < 0) {
            ecode = myerrno;
            std::cout << "try connect sip server error " << myerrno;
            break;
        }
        sockaddr_in localAddr;
        socklen_t addrLen = sizeof(sockaddr_in);
        if (0 != getsockname(fsock, (sockaddr*)&localAddr, &addrLen)) {
            std::cout << "get local addr error " << myerrno;
            break;
        }
        char szAddr[INET_ADDRSTRLEN];
        if (NULL == inet_ntop(AF_INET, &localAddr.sin_addr, szAddr, INET_ADDRSTRLEN)) {
            std::cout << "inet_ntop error " << myerrno;
            break;
        }
        localIp = szAddr;
        std::cout << "localip:" << szAddr;
    } while (false);

    if (fsock) {
        myclose(fsock);
    }
    if (res) {
        freeaddrinfo(res);
    }

    return localIp;
}
