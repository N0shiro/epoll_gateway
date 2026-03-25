#include <iostream>

#include "TcpServer.h"

int main() {
    std::cout << "=== Epoll 网关启动 ===" << std::endl;

    TcpServer server(8080);
    server.start();

    return 0;
}
