#include "client.h"
#include "../common/fingerprint.h"
#include <iostream>
#include <poll.h>
#include <unistd.h>

constexpr int POLL_TIMEOUT = 10;
constexpr unsigned PFDS_COUNT = 2;
constexpr unsigned TIMER_PFDS_IDX = 0;
constexpr unsigned SERVER_SOCK_PFDS_IDX = 1;
constexpr unsigned long MSG_TO_SERVER_INTERVAL_MS = 5'000; // TODO ustawienie interwału 30 ms

Client::Client(ClientConfig conf)
    : config(std::move(conf)), serverSock(addrinfo{0, AF_INET6, SOCK_DGRAM, 0, 0, nullptr, nullptr, nullptr},
                                          config.gameServer, config.serverPort, false),
      messageToServer{{0}, 0, false}, sessionId(utils::timestampToUll(utils::getCurrentTimestamp())),
      turnDirection(TurnDirection::straight) {
    timerFd = utils::createArmedTimer(MSG_TO_SERVER_INTERVAL_MS * NS_IN_MS);

    // TODO TCP gui
}

void Client::readMessageFromServer() {
}

void Client::sendMessageToServer() {
    std::cout << "DEBUG about to send data to server..." << std::endl;
    ssize_t numbytes = sendto(serverSock.getFd(), messageToServer.data, messageToServer.size, 0,
                              serverSock.getAddrInfo().ai_addr, serverSock.getAddrInfo().ai_addrlen);
    if (numbytes == -1) {
        perror("sendto failed.");
        return;
    }

    std::cout << "DEBUG packet to server sent" << std::endl;

    messageToServer.ready = false;
}

void Client::enqueueMessageToServer() {
    std::cerr << "Enqueue message" << std::endl;
    // TODO event no
    ClientToServerMessage msg(sessionId, turnDirection, 2137, config.playerName);
    messageToServer.size = msg.encode(messageToServer.data);
    messageToServer.ready = true;
}

[[noreturn]] void Client::run() {
    std::cout << "Starting client main loop." << std::endl;

    pollfd pfds[PFDS_COUNT]; // TODO add GUI sock fd
    pfds[TIMER_PFDS_IDX].fd = timerFd;
    pfds[TIMER_PFDS_IDX].events = POLLIN;
    pfds[SERVER_SOCK_PFDS_IDX].fd = serverSock.getFd();
    pfds[SERVER_SOCK_PFDS_IDX].events = POLLIN;

    while (true) {
        int numEvents = poll(pfds, PFDS_COUNT, POLL_TIMEOUT);
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
                enqueueMessageToServer();
            }
        } else {
            if (pfds[SERVER_SOCK_PFDS_IDX].revents & POLLIN) {
                readMessageFromServer();
            }
            if (messageToServer.ready && pfds[SERVER_SOCK_PFDS_IDX].revents & POLLOUT) {
                sendMessageToServer();
            }
        }

        pfds[SERVER_SOCK_PFDS_IDX].events = POLLIN;
        if (messageToServer.ready) {
            pfds[SERVER_SOCK_PFDS_IDX].events |= POLLOUT;
        }
    }
}
