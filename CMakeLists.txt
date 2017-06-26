PROJECT(simple_server)

CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

INCLUDE_DIRECTORIES(BEFORE ./ ./src/include)

add_executable(simple_server simple_server.cpp src/core/tcp_event_server.cpp src/server.cpp)