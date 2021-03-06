#include "client.h"
#include "../common/fingerprint.h"
#include <iostream>
#include <poll.h>
#include <unistd.h>

constexpr int POLL_TIMEOUT = 5;
constexpr unsigned PFDS_COUNT = 3;
constexpr unsigned TIMER_PFDS_IDX = 0;
constexpr unsigned SERVER_SOCK_PFDS_IDX = 1;
constexpr unsigned GUI_SOCK_PFDS_IDX = 2;
constexpr unsigned long MSG_TO_SERVER_INTERVAL_MS = 30;
constexpr size_t INBOUND_SERVER_MESSAGE_BUFFER_SIZE = MAX_UDP_DATA_FIELD_SIZE + 10;

Client::Client(ClientConfig conf)
    : config(std::move(conf)), serverSock({0, AF_UNSPEC, SOCK_DGRAM, 0, 0, nullptr, nullptr, nullptr},
                                          config.gameServer, config.serverPort, false),
      guiSock({0, AF_UNSPEC, SOCK_STREAM, 0, 0, nullptr, nullptr, nullptr}, config.guiServer, config.guiPort),
      timerFd(utils::createTimer()), messageToServer{{0}, 0, false},
      sessionId(utils::timestampToUll(utils::getCurrentTimestamp())), turnDirection(TurnDirection::straight),
      gameId(-1) {
    utils::setIntervalTimer(timerFd, MSG_TO_SERVER_INTERVAL_MS * NS_IN_MS);
}

void Client::validateEvent(const Event &event) const {
    while (true) {
        if (event.eventType == EventType::newGame) {
            const auto &e = std::get<EventNewGame>(event.eventData);
            try {
                if (event.eventNo != 0 || e.parsedPlayers().size() < 2) {
                    break;
                }
            } catch (const EncoderDecoderError &e) {
                break;
            }
        } else if (event.eventType == EventType::pixel) {
            const auto &e = std::get<EventPixel>(event.eventData);
            if (e.playerNumber >= playersInGame || e.x >= maxx || e.y >= maxy) {
                break;
            }
        } else if (event.eventType == EventType::playerEliminated) {
            const auto &e = std::get<EventPlayerEliminated>(event.eventData);
            if (e.playerNumber >= playersInGame) {
                break;
            }
        }
        return;
    }
    std::cerr << "Received corrupted data from server with correct crc32. Failing \n" << std::endl;
    exit(EXIT_FAILURE);
}

void Client::readInEventsListOfCurrentGame(const ServerToClientMessage &msg) {
    for (const Event &event : msg.events) {
        validateEvent(event);
        if (events.find(event.eventNo) == events.end()) {
            events.insert({event.eventNo, event});
        }
    }
}

void Client::processMessageFromServerWithMismatchedGameId(const ServerToClientMessage &msg) {
    if (seenGameIds.find(msg.gameId) != seenGameIds.end()) {
        // A strayed packet describing one of the previous games.
        return;
    }

    // A packet describing a new game.
    gameId = msg.gameId;
    seenGameIds.insert({msg.gameId});
    playersInGame = 0;
    nextWantedEventNo = 0;
    nextEventToSendNo = 0;
    events.clear();

    // Try to find NEW_GAME event.
    for (const Event &event : msg.events) {
        if (event.eventType == EventType::newGame) {
            validateEvent(event);
            const auto &newGame = std::get<EventNewGame>(event.eventData);
            maxx = newGame.maxx;
            maxy = newGame.maxy;
            playerNames = newGame.parsedPlayers();
            playersInGame = static_cast<uint8_t>(playerNames.size());
            readInEventsListOfCurrentGame(msg);
            return;
        }
    }
}

void Client::processMessageFromServer(const ServerToClientMessage &msg) {
    if (msg.gameId != gameId) {
        processMessageFromServerWithMismatchedGameId(msg);
    } else {
        readInEventsListOfCurrentGame(msg);
    }

    while (events.find(nextWantedEventNo) != events.end()) {
        nextWantedEventNo++;
    }
    while (nextEventToSendNo < nextWantedEventNo) {
        guiSock.writeData(eventToMessageForGui(events.find(nextEventToSendNo)->second, playerNames));
        nextEventToSendNo++;
    }
}

void Client::readMessageFromServer() {
    static unsigned char buf[INBOUND_SERVER_MESSAGE_BUFFER_SIZE];

    errno = 0;
    ssize_t numbytes = recv(serverSock.getFd(), buf, INBOUND_SERVER_MESSAGE_BUFFER_SIZE, MSG_DONTWAIT);
    if (numbytes == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Error in recv. Skipping this datagram processing.");
        }
        return;
    }

    auto bytesRead = static_cast<size_t>(numbytes);
    if (bytesRead > MAX_UDP_DATA_FIELD_SIZE) {
        // Discarding an invalid datagram.
        return;
    }

    try {
        ServerToClientMessage m(buf, bytesRead);
        processMessageFromServer(m);
    } catch (const EncoderDecoderError &e) {
        std::cerr << "Error in server message decoding." << std::endl;
        exit(EXIT_FAILURE);
    }
}

void Client::sendMessageToServer() {
    ClientToServerMessage msg(sessionId, turnDirection, nextWantedEventNo, config.playerName);
    messageToServer.size = msg.encode(messageToServer.data);

    ssize_t numbytes = sendto(serverSock.getFd(), messageToServer.data, messageToServer.size, MSG_DONTWAIT,
                              serverSock.getAddrInfo().ai_addr, serverSock.getAddrInfo().ai_addrlen);
    if (numbytes == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("sendto failed.");
        }
        return;
    }

    messageToServer.ready = false;
}

void Client::enqueueMessageToServer() {
    messageToServer.ready = true;
}

void Client::readUpdateFromGui() {
    while (true) {
        auto [success, line] = guiSock.readline();
        if (!success) {
            return;
        }

        if (line == "LEFT_KEY_DOWN") {
            turnDirection = TurnDirection::left;
        } else if (line == "RIGHT_KEY_DOWN") {
            turnDirection = TurnDirection::right;
        } else if ((line == "LEFT_KEY_UP" && turnDirection == TurnDirection::left) ||
                   (line == "RIGHT_KEY_UP" && turnDirection == TurnDirection::right)) {
            turnDirection = TurnDirection::straight;
        }

        if (!guiSock.hasAvailableLine()) {
            break;
        }
    }
}

[[noreturn]] void Client::run() {
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
            utils::clearTimer(timerFd);
            enqueueMessageToServer();
        }
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
