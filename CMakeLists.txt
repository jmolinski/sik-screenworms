cmake_minimum_required(VERSION 3.19)
project(sik_networms)

set(CMAKE_CXX_FLAGS "-g -Wall -Wextra -Werror -Wpedantic -O2 -std=c++17 -Wshadow -Wformat=2 -Wfloat-equal -Wconversion -Wlogical-op -Wshift-overflow=2 -Wduplicated-cond -Wcast-qual -Wcast-align -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-protector")
set(CMAKE_CXX_STANDARD 17)

#  -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_FORTIFY_SOURCE=2

add_executable(screen-worms-server
        server_src/main.cpp
        common/getopt_wrapper.cpp
        common/misc.cpp
        common/udp_socket.cpp
        common/interval_timer.cpp
        server_src/server.cpp
        server_src/game.cpp
        server_src/server_config.cpp
        common/messages.cpp
        common/fingerprint.cpp
        )

add_executable(screen-worms-client
        client_src/main.cpp
        common/getopt_wrapper.cpp
        common/misc.cpp
        common/udp_socket.cpp
        common/interval_timer.cpp
        client_src/client_config.cpp
        client_src/client.cpp
        common/messages.cpp
        common/fingerprint.cpp
        )

install(TARGETS DESTINATION .)
