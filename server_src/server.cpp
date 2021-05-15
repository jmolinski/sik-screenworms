#include "server.h"
#include <iostream>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

Server::Server(ServerConfig parsedConfig) : config(parsedConfig) {
    addrinfo hints{AI_PASSIVE, AF_INET6, SOCK_DGRAM, 0, 0, nullptr, nullptr, nullptr};
    socket = UdpSocket(hints, "", config.port, true);
    timerFd = utils::createArmedTimer(2 * NS_IN_SECOND);

    // TODO create game controller, create mq
}

Server::~Server() {
}

void Server::run() {
    std::cout << "entering run" << std::endl;
    while (true) {
        // TODO poll
        uint64_t exp;
        ssize_t s = read(timerFd, &exp, sizeof(uint64_t));
        if (s != sizeof(uint64_t)) {
            std::cerr << "read" << std::endl;
        } else {
            std::cerr << "alarm" << std::endl;
        }
    }
}
