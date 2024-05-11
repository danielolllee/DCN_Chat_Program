cmake_minimum_required(VERSION 3.26)
project(Chat)

set(CMAKE_CXX_STANDARD 17)

link_libraries(ws2_32)

add_executable(Server server.cpp)
add_executable(Client client.cpp)
