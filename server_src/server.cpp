#include "server.h"
#include <iostream>
#include <poll.h>
#include <unistd.h>

constexpr int POLL_TIMEOUT = 5;
constexpr unsigned PFDS_COUNT = 2;
constexpr unsigned SOCKET_PFDS_IDX = 0;
constexpr unsigned TURN_TIMER_PFDS_IDX = 1;
constexpr size_t INCOMING_MSG_BUFFER_SIZE = 100;
constexpr unsigned CONNECTION_EXPIRATION_TIME_SEC = 2;

Server::Server(ServerConfig parsedConfig)
    : config(parsedConfig),
      sock(addrinfo{AI_PASSIVE, AF_INET6, SOCK_DGRAM, 0, 0, nullptr, nullptr, nullptr}, "", config.port, true),
      turnTimerFd(utils::createTimer()),
      gameManager(config.rngSeed, config.turningSpeed, config.boardWidth, config.boardHeight, turnTimerFd,
                  NS_IN_SECOND / config.roundsPerSec) {
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
    auto currentTime = utils::getCurrentTimestamp().time_since_epoch().count();
    std::vector<utils::fingerprint_t> connsToDrop;

    for (auto &lastConn : lastCommunicationTs) {
        auto lastConnTime = lastConn.second.time_since_epoch().count();
        if ((currentTime - lastConnTime) > (CONNECTION_EXPIRATION_TIME_SEC * US_IN_SECOND)) {
            gameManager.dropConnection(lastConn.first);
            clientAddrs.erase(lastConn.first);
            connsToDrop.push_back(lastConn.first);
        }
    }
    for (const auto &fprint : connsToDrop) {
        lastCommunicationTs.erase(fprint);
    }
}

[[noreturn]] void Server::run() {
    pollfd pfds[PFDS_COUNT];
    pfds[SOCKET_PFDS_IDX].fd = sock.getFd();
    pfds[SOCKET_PFDS_IDX].events = POLLIN;
    pfds[TURN_TIMER_PFDS_IDX].fd = turnTimerFd;
    pfds[TURN_TIMER_PFDS_IDX].events = POLLIN;

    while (true) {
        int numEvents = poll(pfds, PFDS_COUNT, POLL_TIMEOUT);
        if (numEvents == -1) {
            perror(nullptr);
        } else if (numEvents == 0) {
            continue;
        }

        if (pfds[TURN_TIMER_PFDS_IDX].revents & POLLIN) {
            utils::clearTimer(turnTimerFd);
            cleanupObsoleteConnections();
            gameManager.runTurn();
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
