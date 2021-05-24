cmake_minimum_required(VERSION 3.19)
project(sik_networms)

set(CMAKE_CXX_FLAGS "-g -Wall -Wextra -Werror -Wpedantic -O2 -std=c++17 -Wformat=2 -Wfloat-equal -Wconversion -Wshift-overflow=2 -Wduplicated-cond -Wcast-qual -Wcast-align -fsanitize=address -fsanitize=undefined -fno-sanitize-recover -fstack-protector")

#set(CMAKE_CXX_FLAGS "-O2 -std=c++17")

set(CMAKE_CXX_STANDARD 17)

#  -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC -D_FORTIFY_SOURCE=2

add_executable(screen-worms-server
        server_src/main.cpp
        common/getopt_wrapper.cpp
        common/misc.cpp
        common/sockets.cpp
        common/time_utils.cpp
        server_src/server.cpp
        server_src/game.cpp
        server_src/server_config.cpp
        common/messages.cpp
        common/fingerprint.cpp
        common/crc32.cpp
        common/rng.h
        )

add_executable(screen-worms-client
        client_src/main.cpp
        common/getopt_wrapper.cpp
        common/misc.cpp
        common/sockets.cpp
        common/time_utils.cpp
        client_src/client_config.cpp
        client_src/client.cpp
        common/messages.cpp
        common/fingerprint.cpp
        common/crc32.cpp
        common/rng.h
        )

install(TARGETS DESTINATION .)

enable_testing()

add_executable(test_worms
        tests/tests.cpp
        common/getopt_wrapper.cpp
        common/misc.cpp
        common/sockets.cpp
        common/time_utils.cpp
        server_src/server.cpp
        server_src/game.cpp
        server_src/server_config.cpp
        common/messages.cpp
        common/fingerprint.cpp
        common/crc32.cpp
        common/rng.h
        )

add_test(test_crc32 test_worms 1)
add_test(test_encode_decode_game_over test_worms 2)
add_test(test_encode_decode_player_eliminated test_worms 3)
add_test(test_encode_decode_pixel test_worms 4)
add_test(test_encode_decode_new_game test_worms 5)
add_test(test_encode_msg_for_client test_worms 6)
