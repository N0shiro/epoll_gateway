# epoll_gateway

一个基于 `epoll`、非阻塞套接字、线程池和 JSON 解析的 Linux C++ TCP 网关示例项目。

## 项目简介

本项目实现了一个简单的 TCP 网关服务，监听 `8080` 端口，使用 `epoll` 管理多客户端连接，将消息处理任务分发到线程池，并通过 `nlohmann/json` 解析客户端发送的 JSON 数据。

当前业务逻辑刻意保持简洁：从请求中提取设备 ID 和温度字段，当温度超过阈值时输出高温告警信息。

## 技术栈

- C++14
- Linux `epoll`
- POSIX Socket API
- 线程池
- CMake
- `nlohmann/json`

## 功能特性

- 非阻塞 TCP 服务端
- 基于 `epoll` 的事件循环
- 多客户端连接处理
- 基于线程池的任务分发
- JSON 报文解析
- 简单的温度告警规则
- 使用异常处理避免非法 JSON 导致程序崩溃

## 构建方式

本项目面向 Linux 环境。`epoll` 以及 `sys/epoll.h`、`arpa/inet.h`、`unistd.h` 等头文件不适用于标准 Windows 工具链。

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## 运行方式

构建完成后，在 `build` 目录或项目根目录中运行：

```bash
./epoll_gateway
```

程序默认监听 `8080` 端口。

## 示例报文

```json
{"id":"sensor_01","temperature":85}
```

当前示例逻辑会执行以下处理：

- 从 `id` 字段读取设备编号
- 从 `temperature` 字段读取温度值
- 当温度大于 `80` 时输出告警信息

## 项目结构

- `main.cpp`：程序入口
- `src/TcpServer.cpp`：套接字初始化、`epoll` 事件循环、客户端事件处理
- `include/TcpServer.h`：服务端接口定义
- `include/ThreadPool.h`：线程池定义
- `src/ThreadPool.cpp`：线程池源文件占位
- `include/json.hpp`：内置的 `nlohmann/json` 单头文件库

## 设计说明

- 服务端套接字与客户端套接字都被设置为非阻塞模式。
- 使用 `epoll_wait` 统一监听服务端和客户端文件描述符上的事件。
- 客户端数据处理交给线程池执行，主事件循环只负责 I/O 就绪分发。
- JSON 解析使用 `try/catch` 包裹，避免异常输入直接导致进程退出。

## 当前局限

- 仅支持 Linux
- 更偏向教学和学习用途，未达到生产可用标准
- 暂未实现协议拆包/粘包处理，默认一次读取就能拿到完整 JSON
- 暂未实现优雅退出和信号处理
- 连接生命周期管理和错误恢复能力较弱

## 第三方依赖

- [`nlohmann/json`](https://github.com/nlohmann/json)，已作为 `include/json.hpp` 收录到仓库中
- 许可证：MIT

## 后续可改进方向

- 增加基于长度前缀或分隔符的报文边界处理
- 为 JSON 解析和告警逻辑补充单元测试
- 将网络层和业务逻辑进一步解耦
- 用分级日志替代直接输出 `std::cout`
- 支持通过配置项指定端口和线程数

## 许可证

本仓库使用 MIT License，详见 `LICENSE` 文件。
