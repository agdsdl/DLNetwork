#include <cstdlib>
#include <ctime>
#include <thread>
#include <platform.h>
#include <string>
#include <iostream>
#include <sstream>
#include <csignal>
#include <set>
#include <memory>
#include <mutex>
#include <stdio.h>

#include "MyLog.h"
#include "EventThread.h"
#include "TcpServer.h"
#include "TcpClient.h"
#include "TcpConnection.h"
#include "TlsTest.h"
#include "UdpTest.h"
#include "TcpTest.h"
using namespace DLNetwork;

bool InitSocket()
{
#ifdef WIN32
    int Error;
    WORD VersionRequested;
    WSADATA WsaData;
    VersionRequested = MAKEWORD(2, 2);
    Error = WSAStartup(VersionRequested, &WsaData); //WinSock2
    if (Error != 0) {
        printf("Error:Start WinSock failed!\n");
        return false;
    }
    else {
        if (LOBYTE(WsaData.wVersion) != 2 || HIBYTE(WsaData.wHighVersion) != 2) {
            printf("Error:The version is WinSock2!\n");
            WSACleanup();
            return false;
        }
    }
#endif
    return true;
}



int main(int argc, char** argv) {
    InitSocket();
    setMyLogLevel(MyLogLevel::Debug);
    EventThreadPool::instance().init(1);

    // 取消注释以测试不同功能
    testTcp();  // 添加TCP测试选项
    testUdp();
    testTls();

#ifdef WIN32
    WSACleanup();
#endif

    return 0;
}
