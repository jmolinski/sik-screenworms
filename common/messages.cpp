#include "messages.h"
#include "crc32.h"
#include <cstring>
#include <iostream>

template <typename T>
static inline T binaryToNum(const unsigned char *buff) {
    T num;
    memcpy(&num, buff, sizeof(T));
    return num;
}

template <typename T>
static inline void numToBinary(T num, unsigned char *buff) {
    auto *numBPtr = reinterpret_cast<unsigned char *>(&num);
    for (size_t i = 0; i < sizeof(T); i++) {
        buff[i] = numBPtr[i];
    }
}

ClientToServerMessage::ClientToServerMessage(uint64_t sessionIdP, TurnDirection direction, uint32_t eventNo,
                                             const std::string &playerNameP)
    : sessionId(sessionIdP), turnDirection(direction), nextExpectedEventNo(eventNo) {
    playerNameSize = playerNameP.size();
    memset(playerName, 0, sizeof playerName);
    memcpy(playerName, playerNameP.c_str(), playerNameSize);
}

ClientToServerMessage::ClientToServerMessage(const unsigned char *buffer, size_t size) {
    if (size < minStructSize || size > maxStructSize) {
        throw EncoderDecoderError();
    }

    sessionId = be64toh(binaryToNum<uint64_t>(buffer));
    turnDirection = static_cast<TurnDirection>(buffer[8]);
    nextExpectedEventNo = be32toh(binaryToNum<uint32_t>(buffer + 9));
    playerNameSize = size - minStructSize;
    memset(playerName, 0, sizeof playerName);
    memcpy(playerName, buffer + minStructSize, playerNameSize);
    if (buffer[8] > static_cast<uint8_t>(TurnDirection::left) || !utils::isValidPlayerName(playerName)) {
        throw EncoderDecoderError();
    }
}

size_t ClientToServerMessage::encode(unsigned char *buffer) const {
    numToBinary(htobe64(sessionId), buffer);
    numToBinary(turnDirection, buffer + 8);
    numToBinary(htobe32(nextExpectedEventNo), buffer + 9);
    memcpy(buffer + minStructSize, playerName, playerNameSize);
    return minStructSize + playerNameSize;
}

Event::Event(uint32_t eventNo, EventType eventType, GameEventVariant eventVariant)
    : eventNo(eventNo), eventType(eventType), eventData(eventVariant) {
}

Event::Event(const unsigned char *buffer, size_t size, size_t *bytesUsed) : eventData(EventGameOver()) {
    uint32_t len = be32toh(binaryToNum<uint32_t>(buffer));
    eventNo = be32toh(binaryToNum<uint32_t>(buffer + 4));
    eventType = EventType(buffer[8]);
    const uint32_t eventHeaderSize = 9;
    *bytesUsed = len + sizeof(uint32_t);

    if (len + 4 > size) {
        throw EncoderDecoderError();
    }

    uint32_t crc32Got = be32toh(binaryToNum<uint32_t>(buffer + len));
    uint32_t crc32Expected = utils::crc32(buffer, len);

    if (crc32Got != crc32Expected) {
        throw CRC32MismatchError();
    }

    if (eventType == EventType::newGame) {
        eventData = EventNewGame(buffer + eventHeaderSize, len - eventHeaderSize);
    } else if (eventType == EventType::pixel) {
        eventData = EventPixel(buffer + eventHeaderSize, len - eventHeaderSize);
    } else if (eventType == EventType::playerEliminated) {
        eventData = EventPlayerEliminated(buffer + eventHeaderSize, len - eventHeaderSize);
    } else if (eventType == EventType::gameOver) {
        eventData = EventGameOver(buffer + eventHeaderSize, len - eventHeaderSize);
    } else {
        // Invalid eventType
        throw EncoderDecoderError();
    }
}

uint32_t Event::encode(unsigned char *buffer) const {
    const unsigned char *bufferFirstPos = buffer;
    uint32_t encodedSize = getEncodedSize();
    auto len = static_cast<uint32_t>(encodedSize - sizeof(uint32_t));
    numToBinary(htobe32(len), buffer);
    numToBinary(htobe32(eventNo), buffer + 4);
    buffer[8] = static_cast<uint8_t>(eventType);
    buffer += 9;

    if (eventType == EventType::newGame) {
        buffer += std::get<EventNewGame>(eventData).encode(buffer);
    } else if (eventType == EventType::pixel) {
        buffer += std::get<EventPixel>(eventData).encode(buffer);
    } else if (eventType == EventType::playerEliminated) {
        buffer += std::get<EventPlayerEliminated>(eventData).encode(buffer);
    } else if (eventType == EventType::gameOver) {
        buffer += std::get<EventGameOver>(eventData).encode(buffer);
    }

    uint32_t crc32 = utils::crc32(bufferFirstPos, len);
    numToBinary(htobe32(crc32), buffer);
    return encodedSize;
}

uint32_t Event::getEventDataSize() const {
    if (eventType == EventType::newGame) {
        return std::get<EventNewGame>(eventData).getSize();
    } else if (eventType == EventType::pixel) {
        return std::get<EventPixel>(eventData).getSize();
    } else if (eventType == EventType::playerEliminated) {
        return std::get<EventPlayerEliminated>(eventData).getSize();
    } else if (eventType == EventType::gameOver) {
        return std::get<EventGameOver>(eventData).getSize();
    }
    throw std::runtime_error("Unknown event type");
}

uint32_t Event::getEncodedSize() const {
    return getEventDataSize() + sizeof(uint32_t) + sizeof(eventNo) + sizeof(eventType) + sizeof(uint32_t);
}

EventNewGame::EventNewGame(uint32_t maxx, uint32_t maxy, const std::vector<std::string> &playerNames) {
    this->maxx = maxx;
    this->maxy = maxy;

    std::ostringstream os; // TODO ładniej
    for (const auto &pname : playerNames) {
        os << pname << ' ';
    }
    std::string spaceSeparatedNames = os.str();
    memcpy(players, spaceSeparatedNames.c_str(), spaceSeparatedNames.size());
    playersFieldSize = static_cast<uint32_t>(spaceSeparatedNames.size());

    for (unsigned i = 0; i < playersFieldSize; i++) {
        if (players[i] == ' ') {
            players[i] = '\0';
        }
    }
}

EventNewGame::EventNewGame(const unsigned char *buff, uint32_t size) {
    maxx = be32toh(binaryToNum<uint32_t>(buff));
    maxy = be32toh(binaryToNum<uint32_t>(buff + 4));
    playersFieldSize = size - 8;
    memcpy(players, buff + 8, playersFieldSize);
}

uint32_t EventNewGame::encode(unsigned char *buff) const {
    numToBinary(htobe32(maxx), buff);
    numToBinary(htobe32(maxy), buff + 4);
    memcpy(buff + 8, players, playersFieldSize);
    return 8 + playersFieldSize;
}

std::unordered_map<uint8_t, std::string> EventNewGame::parsedPlayers() const {
    static char buff[maxPlayersFieldSize];
    memcpy(buff, players, playersFieldSize);
    for (unsigned i = 0; i < playersFieldSize; i++) {
        if (buff[i] == ' ') {
            throw EncoderDecoderError();
        }
        if (buff[i] == '\0') {
            buff[i] = ' ';
        }
    }

    std::stringstream ss;
    ss.write(buff, playersFieldSize);
    std::string playerName;

    std::unordered_map<uint8_t, std::string> playerNames;
    while (ss >> playerName) {
        if (!utils::isValidPlayerName(playerName)) {
            throw EncoderDecoderError();
        }
        playerNames.insert({playerNames.size(), playerName});
    }

    return playerNames;
}

EventPixel::EventPixel(const unsigned char *buff, size_t size) {
    if (size < getSize()) {
        throw EncoderDecoderError();
    }

    playerNumber = buff[0];
    x = be32toh(binaryToNum<uint32_t>(buff + 1));
    y = be32toh(binaryToNum<uint32_t>(buff + 5));
}

uint32_t EventPixel::encode(unsigned char *buff) const {
    buff[0] = playerNumber;
    numToBinary(htobe32(x), buff + 1);
    numToBinary(htobe32(y), buff + 5);
    return 9;
}

ServerToClientMessage::ServerToClientMessage(unsigned char *buffer, size_t size) {
    if (size < minStructSize || size > maxStructSize) {
        throw EncoderDecoderError();
    }

    gameId = be32toh(binaryToNum<uint32_t>(buffer));

    size_t bytesRead = 4;
    while (size > bytesRead) {
        size_t eventSize = 0;
        try {
            Event event(buffer + bytesRead, size - bytesRead, &eventSize);
            events.push_back(event);
        } catch (const CRC32MismatchError &e) {
            // We stop decoding further data, but preserve the first events with correct checksum.
            return;
        }
        bytesRead += eventSize;
    }
}

size_t ServerToClientMessage::encode(unsigned char *buffer) {
    numToBinary(htobe32(gameId), buffer);

    uint32_t written = 4;
    for (const auto &event : events) {
        written += event.encode(buffer + written);
    }

    return written;
}

std::pair<uint32_t, ServerToClientMessage> ServerToClientMessage::fromEvents(uint32_t gameIdParam,
                                                                             std::vector<Event>::const_iterator it,
                                                                             std::vector<Event>::const_iterator endIt) {
    std::vector<Event> pickedEvents;
    size_t reservedSize = sizeof(gameId);

    while (it != endIt) {
        size_t eventSize = it->getEncodedSize();
        if (reservedSize + eventSize > MAX_UDP_DATA_FIELD_SIZE) {
            break;
        }
        reservedSize += eventSize;
        pickedEvents.push_back(*it);
        it++;
    }

    return {pickedEvents.size(), ServerToClientMessage(gameIdParam, pickedEvents)};
}

std::string eventToMessageForGui(const Event &e, const std::unordered_map<uint8_t, std::string> &playerNames) {
    std::ostringstream os;

    if (e.eventType == EventType::newGame) {
        const auto &event = std::get<EventNewGame>(e.eventData);
        os << "NEW_GAME " << event.maxx << ' ' << event.maxy;

        auto totalPlayers = static_cast<uint8_t>(playerNames.size());
        for (uint8_t i = 0; i < totalPlayers; i++) {
            os << ' ' << playerNames.find(i)->second;
        }
        os << '\n';
    } else if (e.eventType == EventType::pixel) {
        const auto &event = std::get<EventPixel>(e.eventData);
        os << "PIXEL " << event.x << ' ' << event.y << ' ' << playerNames.find(event.playerNumber)->second << '\n';
    } else if (e.eventType == EventType::playerEliminated) {
        const auto &event = std::get<EventPlayerEliminated>(e.eventData);
        os << "PLAYER_ELIMINATED " << playerNames.find(event.playerNumber)->second << '\n';
    }

    return os.str();
}
