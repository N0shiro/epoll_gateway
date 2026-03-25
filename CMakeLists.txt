cmake_minimum_required(VERSION 3.10)

project(epoll_gateway LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

aux_source_directory(src SRC_LIST)

find_package(Threads REQUIRED)

add_executable(epoll_gateway main.cpp ${SRC_LIST})

target_include_directories(epoll_gateway PRIVATE include)
target_link_libraries(epoll_gateway PRIVATE Threads::Threads)
