#include "server.h"
#include <iostream>
#include <poll.h>
#include <unistd.h>

constexpr int POLL_TIMEOUT = 5;
constexpr unsigned PFDS_COUNT = 2;
constexpr unsigned TURN_TIMER_PFDS_IDX = 0;
constexpr unsigned SOCKET_PFDS_IDX = 1;
constexpr size_t INCOMING_MSG_BUFFER_SIZE = 100;
constexpr unsigned CONNECTION_EXPIRATION_TIME_SEC = 2; // TODO

Server::Server(ServerConfig parsedConfig)
    : config(parsedConfig),
      sock(addrinfo{AI_PASSIVE, AF_INET6, SOCK_DGRAM, 0, 0, nullptr, nullptr, nullptr}, "", config.port, true),
      gameManager(config.rngSeed, config.turningSpeed, config.boardWidth, config.boardHeight) {
    // turnTimerFd = utils::createArmedTimer(100 * NS_IN_MS); // TODO ustawienie tury
    long roundTime = NS_IN_SECOND / config.roundsPerSec;
    turnTimerFd = utils::createArmedTimer(roundTime); // TODO ustawienie tury
}

void Server::readMessageFromClient() {
    static unsigned char buf[INCOMING_MSG_BUFFER_SIZE];

    sockaddr_storage their_addr{};
    socklen_t addr_len = sizeof their_addr;
    ssize_t numbytes =
        recvfrom(sock.getFd(), buf, INCOMING_MSG_BUFFER_SIZE - 1, MSG_DONTWAIT, (sockaddr *)&their_addr, &addr_len);
    if (numbytes == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Error in recvfrom. Skipping this datagram processing.");
        }
        return;
    }

    std::string fingerprint = utils::fingerprintNetuser((sockaddr *)&their_addr);
    lastCommunicationTs.insert_or_assign(fingerprint, utils::getCurrentTimestamp());
    if (clientAddrs.find(fingerprint) == clientAddrs.end()) {
        clientAddrs.insert({fingerprint, {their_addr, addr_len}});
    }

    try {
        ClientToServerMessage m(buf, static_cast<size_t>(numbytes));
        gameManager.handleMessage(fingerprint, m);
    } catch (const EncoderDecoderError &e) {
        std::cerr << "Error in client message decoding. Skipping this datagram processing." << std::endl;
        return;
    }
}

void Server::sendMessageToClient() {
    auto [fingerprint, ackNumber, msg] = gameManager.mqManager.getNextMessage();

    static unsigned char dataBuffer[MAX_UDP_DATA_FIELD_SIZE];
    size_t payloadSize = msg.encode(dataBuffer);

    auto &clientAddrInfo = clientAddrs.find(fingerprint)->second;

    ssize_t numbytes = sendto(sock.getFd(), dataBuffer, payloadSize, MSG_DONTWAIT, (sockaddr *)&clientAddrInfo.first,
                              clientAddrInfo.second);
    if (numbytes == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("sendto failed.");
        }
        return;
    }

    gameManager.mqManager.ack(fingerprint, ackNumber);
}

void Server::cleanupObsoleteConnections() {
    // std::cerr << "conn cleanup" << std::endl; // TODO
}

[[noreturn]] void Server::run() {
    std::cout << "Starting server main loop." << std::endl;

    pollfd pfds[PFDS_COUNT];
    pfds[TURN_TIMER_PFDS_IDX].fd = turnTimerFd;
    pfds[TURN_TIMER_PFDS_IDX].events = POLLIN;
    pfds[SOCKET_PFDS_IDX].fd = sock.getFd();
    pfds[SOCKET_PFDS_IDX].events = POLLIN;

    while (true) {
        int numEvents = poll(pfds, PFDS_COUNT, POLL_TIMEOUT);
        if (numEvents == -1) {
            perror(nullptr);
        } else if (numEvents == 0) {
            continue;
        }

        if (pfds[TURN_TIMER_PFDS_IDX].revents & POLLIN) {
            uint64_t exp;
            if (read(turnTimerFd, &exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
                std::cerr << "Timer read: problem reading turn timer" << std::endl;
            } else {
                cleanupObsoleteConnections();
                gameManager.runTurn();
            }
        }
        if (pfds[SOCKET_PFDS_IDX].revents & POLLIN) {
            readMessageFromClient();
        }
        if (!gameManager.mqManager.isEmpty() && pfds[SOCKET_PFDS_IDX].revents & POLLOUT) {
            sendMessageToClient();
        }

        pfds[SOCKET_PFDS_IDX].events = POLLIN;
        if (!gameManager.mqManager.isEmpty()) {
            pfds[SOCKET_PFDS_IDX].events |= POLLOUT;
        }
    }
}
