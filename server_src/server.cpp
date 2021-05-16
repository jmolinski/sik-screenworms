#include "server.h"
#include <iostream>
#include <poll.h>
#include <unistd.h>

constexpr int POLL_TIMEOUT = 10;
constexpr unsigned TIMER_PFDS_IDX = 0;
constexpr unsigned SOCKET_PFDS_IDX = 1;

Server::Server(ServerConfig parsedConfig) : config(parsedConfig) {
    addrinfo hints{AI_PASSIVE, AF_INET6, SOCK_DGRAM, 0, 0, nullptr, nullptr, nullptr};
    socket = UdpSocket(hints, "", config.port, true);
    timerFd = utils::createArmedTimer(2 * NS_IN_SECOND);  // TODO ustawienie tury

    // TODO create game controller, create mq
}

void Server::readMessageFromClient() {
    std::cerr << "Data can be read (i'm scared)" << std::endl;
}
void Server::sendMessageToClient() {
    std::cerr << "Data can be written (i'm scared)" << std::endl;
}

[[noreturn]] void Server::run() {
    std::cout << "Starting server main loop." << std::endl;

    struct pollfd pfds[2];
    pfds[TIMER_PFDS_IDX].fd = timerFd;
    pfds[TIMER_PFDS_IDX].events = POLLIN;
    pfds[SOCKET_PFDS_IDX].fd = socket.getFd();
    pfds[SOCKET_PFDS_IDX].events = POLLIN;

    while (true) {
        int numEvents = poll(pfds, 2, POLL_TIMEOUT);
        if (numEvents == -1) {
            perror(nullptr);
        } else if (numEvents == 0) {
            continue;
        }

        if (pfds[TIMER_PFDS_IDX].revents & POLLIN) {
            uint64_t exp;
            if (read(timerFd, &exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
                std::cerr << "Timer read: problem reading expirations count" << std::endl;
            } else {
                gameManager.runTurn();
            }
        } else {
            if (pfds[SOCKET_PFDS_IDX].revents & POLLIN) {
                readMessageFromClient();
            }
            if (!mqManager.isEmpty() && pfds[SOCKET_PFDS_IDX].revents & POLLOUT) {
                sendMessageToClient();
            }
        }

        pfds[SOCKET_PFDS_IDX].events = POLLIN;
        if (!mqManager.isEmpty()) {
            pfds[SOCKET_PFDS_IDX].events |= POLLOUT;
        }
    }
}
