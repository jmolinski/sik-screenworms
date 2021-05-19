#include "client.h"
#include "../common/fingerprint.h"
#include <iostream>
#include <poll.h>
#include <unistd.h>

constexpr int POLL_TIMEOUT = 10;
constexpr unsigned PFDS_COUNT = 3;
constexpr unsigned TIMER_PFDS_IDX = 0;
constexpr unsigned SERVER_SOCK_PFDS_IDX = 1;
constexpr unsigned GUI_SOCK_PFDS_IDX = 2;
constexpr unsigned long MSG_TO_SERVER_INTERVAL_MS = 5'000; // TODO ustawienie interwału 30 ms

Client::Client(ClientConfig conf)
    : config(std::move(conf)), serverSock({0, AF_INET6, SOCK_DGRAM, 0, 0, nullptr, nullptr, nullptr}, config.gameServer,
                                          config.serverPort, false),
      guiSock({0, AF_UNSPEC, SOCK_STREAM, 0, 0, nullptr, nullptr, nullptr}, config.guiServer, config.guiPort),
      messageToServer{{0}, 0, false}, sessionId(utils::timestampToUll(utils::getCurrentTimestamp())),
      turnDirection(TurnDirection::straight) {
    serverSock.connectPeer(serverSock.getAddrInfo());
    timerFd = utils::createArmedTimer(MSG_TO_SERVER_INTERVAL_MS * NS_IN_MS);
}

void Client::readMessageFromServer() {
}

void Client::sendMessageToServer() {
    std::cout << "DEBUG about to send data to server..." << std::endl;
    ssize_t numbytes = sendto(serverSock.getFd(), messageToServer.data, messageToServer.size, MSG_DONTWAIT,
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
    ClientToServerMessage msg(sessionId, turnDirection, nextWantedEventNo, config.playerName);
    messageToServer.size = msg.encode(messageToServer.data);
    messageToServer.ready = true;
}

void Client::readUpdateFromGui() {
    auto [success, line] = guiSock.readline();
    if (!success) {
        return;
    }

    std::cout << "DEBUG msg from tcp: " << line << std::endl;

    if (line == "LEFT_KEY_DOWN") {
        turnDirection = TurnDirection::left;
    } else if (line == "RIGHT_KEY_DOWN") {
        turnDirection = TurnDirection::right;
    } else if (line == "LEFT_KEY_UP" || line == "RIGHT_KEY_UP") {
        turnDirection = TurnDirection::straight;
    }
}

[[noreturn]] void Client::run() {
    std::cout << "Starting client main loop." << std::endl;

    pollfd pfds[PFDS_COUNT];
    pfds[TIMER_PFDS_IDX].fd = timerFd;
    pfds[TIMER_PFDS_IDX].events = POLLIN;
    pfds[SERVER_SOCK_PFDS_IDX].fd = serverSock.getFd();
    pfds[SERVER_SOCK_PFDS_IDX].events = POLLIN;
    pfds[GUI_SOCK_PFDS_IDX].fd = guiSock.getFd();
    pfds[GUI_SOCK_PFDS_IDX].events = POLLIN;

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
            if (guiSock.hasAvailableLine() || pfds[GUI_SOCK_PFDS_IDX].revents & POLLIN) {
                readUpdateFromGui();
            }
            if (guiSock.hasPendingOutgoingData() && pfds[GUI_SOCK_PFDS_IDX].revents & POLLOUT) {
                guiSock.flushOutgoing();
            }
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
        pfds[GUI_SOCK_PFDS_IDX].events = POLLIN;
        if (guiSock.hasPendingOutgoingData()) {
            pfds[GUI_SOCK_PFDS_IDX].events |= POLLOUT;
        }
    }
}
