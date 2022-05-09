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
#include <endian.h>
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

