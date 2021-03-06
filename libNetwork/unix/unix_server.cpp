#include "unix_server.h"
#include <thread>
#include <memory>
#include <string>
#include <iostream>

#ifdef linux
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include <memory.h>

typedef int SOCKET;
#endif

unix_server::~unix_server()
{
    s_baccept_thread = false;

    //closesocket(m_socket);   //关闭套接字
    //WSACleanup();            //释放套接字资源;
}
int unix_server::init_server()
{
    int    listenfd;
    struct sockaddr_in     servaddr;

    if ((m_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        //printf("create socket error: %s(errno: %d)\n", strerror(errno), errno);
        return sock_error::createSocket_error;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(m_port);

    if (bind(listenfd, (struct sockaddr*) & servaddr, sizeof(servaddr)) == -1) {
        //printf("bind socket error: %s(errno: %d)\n", strerror(errno), errno);
        return sock_error::bind_error;
    }

    if (listen(listenfd, 10) == -1) {
        //printf("listen socket error: %s(errno: %d)\n", strerror(errno), errno);
        return sock_error::listen_error;
    }

    std::shared_ptr<std::thread> t(new std::thread([&] {
        while (s_baccept_thread)
        {
#ifdef _WIN32
            SOCKET sClient = INVALID_SOCKET;
#elif linux
            SOCKET sClient = -1;
#endif // _WIN32

            sockaddr_in addrClient;
            socklen_t addrClientlen = sizeof(addrClient);
            sClient = accept(m_socket, (sockaddr *) &addrClient, &addrClientlen);
            std::cout << "accept:" << sClient << std::endl;
#ifdef _WIN32
            if (INVALID_SOCKET == sClient)
#elif linux
            if(-1 == sClient)
#endif
            {

                break;
            }
            if (s_baccept_thread)
                process_accept(sClient, m_process);
        }
    }));

    t->detach();

    return sock_error::success;
}
void unix_server::process_accept(mSOCKET clientSock, process pro)
{
    std::shared_ptr<std::thread> s(new std::thread([&] {
        process tmpPro = pro;
        SOCKET tmpClient = clientSock;
        char* buf = new char[BUF_SIZE + 1];
        memset(buf, 0, BUF_SIZE + 1);
        int r = 0;
        std::string result;
        do
        {
            r = recv(tmpClient, buf, BUF_SIZE, 0);
            result += buf;
            std::cout << "recv size:" << r << std::endl;
        } while (r >= BUF_SIZE);

        if (tmpPro)
            tmpPro(result, clientSock);
        //send(tmpClient, result.c_str(), (int)result.length(), 0);

        //closesocket(tmpClient);
        //std::cout << result << std::endl;
    }));

    s->detach();
}