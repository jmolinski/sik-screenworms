cmake_minimum_required(VERSION 3.19)
project(sik_networms)

set(CMAKE_CXX_FLAGS "-g -Wall -Wextra -Werror -Wpedantic -O2 -std=c++17")
set(CMAKE_CXX_STANDARD 17)

add_executable(screen-worms-server
        server_src/main.cpp
        utils/getopt_wrapper.cpp
        utils/utils.cpp
        utils/udp_socket.cpp utils/udp_socket.h
        server_src/server.cpp server_src/server.h
        server_src/server_config.cpp server_src/server_config.h
        )

add_executable(screen-worms-client
        client_src/main.cpp
        utils/getopt_wrapper.cpp
        utils/utils.cpp
        utils/udp_socket.cpp utils/udp_socket.h
        client_src/client_config.cpp
        client_src/client_config.h
        client_src/client.cpp client_src/client.h
        )

install(TARGETS DESTINATION .)
