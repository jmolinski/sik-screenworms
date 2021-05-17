#include "server.h"
#include <iostream>
#include <poll.h>
#include <unistd.h>

constexpr int POLL_TIMEOUT = 10;
constexpr unsigned PFDS_COUNT = 3;
constexpr unsigned TURN_TIMER_PFDS_IDX = 0;
constexpr unsigned EXPIRATION_TIMER_PFDS_IDX = 1;
constexpr unsigned SOCKET_PFDS_IDX = 2;
constexpr size_t INCOMING_MSG_BUFFER_SIZE = 100;
constexpr unsigned CONNECTION_EXPIRATION_TIME_SEC = 2;

Server::Server(ServerConfig parsedConfig)
    : config(parsedConfig),
      sock(addrinfo{AI_PASSIVE, AF_INET6, SOCK_DGRAM, 0, 0, nullptr, nullptr, nullptr}, "", config.port, true),
      gameManager(config.rngSeed, config.turningSpeed, config.boardWidth, config.boardHeight) {
    turnTimerFd = utils::createArmedTimer(10 * NS_IN_SECOND); // TODO ustawienie tury
    expirationTimerFd = utils::createArmedTimer(CONNECTION_EXPIRATION_TIME_SEC * NS_IN_SECOND);
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

    std::string fingerprint = utils::fingerprintNetuser((sockaddr *)&their_addr);
    lastCommunicationTs.insert_or_assign(fingerprint, utils::getCurrentTimestamp());
    if (clientAddrs.find(fingerprint) == clientAddrs.end()) {
        // TODO add addr to map
    }
    std::cout << "DEBUG got packet from " << fingerprint << std::endl;

    try {
        ClientToServerMessage m(buf, static_cast<size_t>(numbytes));
        gameManager.handleMessage(fingerprint, m);
    } catch (const EncoderDecoderError &e) {
        std::cerr << "Error in client message decoding. Skipping this datagram processing." << std::endl;
        return;
    }
}

void Server::sendMessageToClient() {
    std::cerr << "Data can be written (i'm scared)" << std::endl; // TODO
}

void Server::cleanupObsoleteConnections() {
    std::cerr << "conn cleanup" << std::endl; // TODO
}

[[noreturn]] void Server::run() {
    std::cout << "Starting server main loop." << std::endl;

    pollfd pfds[PFDS_COUNT];
    pfds[TURN_TIMER_PFDS_IDX].fd = turnTimerFd;
    pfds[TURN_TIMER_PFDS_IDX].events = POLLIN;
    pfds[EXPIRATION_TIMER_PFDS_IDX].fd = expirationTimerFd;
    pfds[EXPIRATION_TIMER_PFDS_IDX].events = POLLIN;
    pfds[SOCKET_PFDS_IDX].fd = sock.getFd();
    pfds[SOCKET_PFDS_IDX].events = POLLIN;

    while (true) {
        int numEvents = poll(pfds, PFDS_COUNT, POLL_TIMEOUT);
        if (numEvents == -1) {
            perror(nullptr);
        } else if (numEvents == 0) {
            continue;
        }

        uint64_t exp;
        if (pfds[TURN_TIMER_PFDS_IDX].revents & POLLIN) {
            if (read(turnTimerFd, &exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
                std::cerr << "Timer read: problem reading turn timer" << std::endl;
            } else {
                gameManager.runTurn();
            }
        } else if (pfds[EXPIRATION_TIMER_PFDS_IDX].revents & POLLIN) {
            if (read(expirationTimerFd, &exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
                std::cerr << "Timer read: problem reading conenction expiration timer" << std::endl;
            } else {
                cleanupObsoleteConnections();
            }
        } else {
            if (pfds[SOCKET_PFDS_IDX].revents & POLLIN) {
                readMessageFromClient();
            }
            if (!gameManager.mqManager.isEmpty() && pfds[SOCKET_PFDS_IDX].revents & POLLOUT) {
                sendMessageToClient();
            }
        }

        pfds[SOCKET_PFDS_IDX].events = POLLIN;
        if (!gameManager.mqManager.isEmpty()) {
            pfds[SOCKET_PFDS_IDX].events |= POLLOUT;
        }
    }
}
