#include "TcpServer.h"

#include "ThreadPool.h"
#include "json.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace {

constexpr int kWorkerCount = 4;
constexpr int kMaxEvents = 1024;
constexpr int kBufferSize = 1024;

void closeClient(int epoll_fd, int client_fd) {
    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, nullptr);
    close(client_fd);
}

}  // namespace

TcpServer::TcpServer(int port)
    : port(port), server_fd(-1), epoll_fd(-1), thread_pool(std::make_unique<ThreadPool>(kWorkerCount)) {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "Failed to set socket options" << std::endl;
        close(server_fd);
        std::exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr {};
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, reinterpret_cast<struct sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        std::cerr << "Failed to bind socket" << std::endl;
        close(server_fd);
        std::exit(EXIT_FAILURE);
    }

    if (listen(server_fd, SOMAXCONN) == -1) {
        std::cerr << "Failed to listen on socket" << std::endl;
        close(server_fd);
        std::exit(EXIT_FAILURE);
    }

    setNonBlocking(server_fd);

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "Failed to create epoll instance" << std::endl;
        close(server_fd);
        std::exit(EXIT_FAILURE);
    }

    struct epoll_event event {};
    event.events = EPOLLIN;
    event.data.fd = server_fd;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &event) == -1) {
        std::cerr << "Failed to add server socket to epoll" << std::endl;
        close(server_fd);
        close(epoll_fd);
        std::exit(EXIT_FAILURE);
    }

    std::cout << "Server started on port " << port << std::endl;
}

void TcpServer::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Failed to get file flags" << std::endl;
        return;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Failed to set non-blocking mode" << std::endl;
    }
}

void TcpServer::start() {
    struct epoll_event events[kMaxEvents];

    while (true) {
        int nfds = epoll_wait(epoll_fd, events, kMaxEvents, -1);
        if (nfds == -1) {
            if (errno == EINTR) {
                continue;
            }

            std::cerr << "Failed to wait for epoll events" << std::endl;
            continue;
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_fd) {
                struct sockaddr_in client_addr {};
                socklen_t client_addr_len = sizeof(client_addr);

                while (true) {
                    int client_fd = accept(
                        server_fd,
                        reinterpret_cast<struct sockaddr*>(&client_addr),
                        &client_addr_len);

                    if (client_fd == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            break;
                        }

                        std::cerr << "Failed to accept client connection" << std::endl;
                        break;
                    }

                    setNonBlocking(client_fd);

                    struct epoll_event client_event {};
                    client_event.events = EPOLLIN;
                    client_event.data.fd = client_fd;

                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_event) == -1) {
                        std::cerr << "Failed to add client socket to epoll" << std::endl;
                        close(client_fd);
                    }
                }

                continue;
            }

            int active_client_fd = events[i].data.fd;
            thread_pool->enqueue([this, active_client_fd] {
                char buffer[kBufferSize];
                ssize_t bytes_read = read(active_client_fd, buffer, sizeof(buffer));

                if (bytes_read > 0) {
                    std::string data_str(buffer, bytes_read);
                    std::cout << "\n--- 收到新消息 ---" << std::endl;
                    std::cout << "原始报文: " << data_str << std::endl;

                    try {
                        auto j = nlohmann::json::parse(data_str);

                        if (!j.is_object()) {
                            std::cerr << "[格式警告] 收到的不是 JSON 对象: " << data_str << std::endl;
                            return;
                        }

                        std::string sensor_id = j.value("id", "unknown_device");
                        int temperature = j.value("temperature", 0);

                        std::cout << "[解析成功] 设备: " << sensor_id
                                  << " | 当前温度: " << temperature << "℃" << std::endl;

                        if (temperature > 80) {
                            std::cout << "[警报] 设备 " << sensor_id << " 温度超标" << std::endl;
                        }
                    } catch (const nlohmann::json::exception& e) {
                        std::cerr << "[解析失败] " << e.what() << std::endl;
                    }

                    return;
                }

                if (bytes_read == 0) {
                    std::cout << "Client disconnected" << std::endl;
                    closeClient(epoll_fd, active_client_fd);
                    return;
                }

                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    std::cerr << "Failed to read client data" << std::endl;
                    closeClient(epoll_fd, active_client_fd);
                }
            });
        }
    }
}

TcpServer::~TcpServer() {
    if (server_fd != -1) {
        close(server_fd);
    }

    if (epoll_fd != -1) {
        close(epoll_fd);
    }
}
