cmake_minimum_required(VERSION 3.19)
project(sik_networms)

set(CMAKE_CXX_FLAGS "-g -Wall -Wextra -Werror -Wpedantic -O2 -std=c++17")
set(CMAKE_CXX_STANDARD 17)

add_executable(screen-worms-server
        server_src/main.cpp
        utils/getopt_wrapper.cpp
        utils/utils.cpp
        )

add_executable(screen-worms-client
        client_src/main.cpp
        utils/getopt_wrapper.cpp
        utils/utils.cpp
        )

install(TARGETS DESTINATION .)
