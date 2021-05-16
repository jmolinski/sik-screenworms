#include "server.h"
#include "../common/fingerprint.h"
#include "../common/messages.h"
#include <iostream>
#include <poll.h>
#include <unistd.h>

constexpr int POLL_TIMEOUT = 10;
constexpr unsigned TIMER_PFDS_IDX = 0;
constexpr unsigned SOCKET_PFDS_IDX = 1;
constexpr size_t INCOMING_MSG_BUFFER_SIZE = 100;

Server::Server(ServerConfig parsedConfig)
    : config(parsedConfig),
      sock(addrinfo{AI_PASSIVE, AF_INET6, SOCK_DGRAM, 0, 0, nullptr, nullptr, nullptr}, "", config.port, true) {
    timerFd = utils::createArmedTimer(10 * NS_IN_SECOND); // TODO ustawienie tury

    // TODO create game controller, create mq
}

void Server::readMessageFromClient() {
    std::cout << "DEBUG about to read data from client..." << std::endl;

    static unsigned char buf[INCOMING_MSG_BUFFER_SIZE];

    sockaddr_storage their_addr{};
    socklen_t addr_len = sizeof their_addr;
    ssize_t numbytes = recvfrom(sock.getFd(), buf, INCOMING_MSG_BUFFER_SIZE - 1, 0, (sockaddr *)&their_addr, &addr_len);
    if (numbytes == -1) {
        perror("Error in recvfrom. Skipping this datagram processing.");
        return;
    }

    std::string fingerprint = fingerprintNetuser((sockaddr *)&their_addr);
    std::cout << "DEBUG got packet from " << fingerprint << std::endl;

    try {
        ClientToServerMessage m(buf, static_cast<size_t>(numbytes));
        // TODO do something with the message
    } catch (...) {
        std::cerr << "Error in client message decoding. Skipping this datagram processing." << std::endl;
        return;
    }
}

void Server::sendMessageToClient() {
    std::cerr << "Data can be written (i'm scared)" << std::endl;
}

[[noreturn]] void Server::run() {
    std::cout << "Starting server main loop." << std::endl;

    struct pollfd pfds[2];
    pfds[TIMER_PFDS_IDX].fd = timerFd;
    pfds[TIMER_PFDS_IDX].events = POLLIN;
    pfds[SOCKET_PFDS_IDX].fd = sock.getFd();
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
