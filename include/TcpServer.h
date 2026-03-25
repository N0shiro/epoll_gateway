#ifndef TCPSERVER_H
#define TCPSERVER_H

#include <memory>

class ThreadPool;

class TcpServer {
public:
    explicit TcpServer(int port);
    ~TcpServer();

    void start();
    void setNonBlocking(int fd);

private:
    int port;
    int server_fd;
    int epoll_fd;
    std::unique_ptr<ThreadPool> thread_pool;
};

#endif
