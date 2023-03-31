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

#include <string>

#ifdef WIN32
#include <winsock2.h> 
#include <ws2tcpip.h>
#include <Iphlpapi.h>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment(lib,"Iphlpapi.lib")
#define ioctl ioctlsocket
#define poll WSAPoll

#define myerrno WSAGetLastError()
#define myclose closesocket
#define MYEINTR WSAEINTR

#else
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/types.h>
#include <netdb.h>
#ifdef __APPLE__
#include <machine/endian.h>
#else
#include <endian.h>
#endif
#include <sys/uio.h>
#include <sys/syscall.h>

#ifndef SOCKET
#define SOCKET int
#endif
#define myerrno errno
#define myclose ::close
#define MYEINTR EINTR

#ifndef SOCKET_ERROR
#define SOCKET_ERROR            (-1)
#endif // ! SOCKET_ERROR

#endif

