#ifndef SIK_NETWORMS_MESSAGES_H
#define SIK_NETWORMS_MESSAGES_H

#include "misc.h"
#include <exception>
#include <string>
#include <utility>
#include <variant>
#include <vector>

constexpr size_t MAX_UDP_DATA_FIELD_SIZE = 550;
constexpr unsigned MAX_PLAYERS = 25;

class EncoderDecoderError : public std::exception {};
class CRC32MismatchError : public std::exception {};

enum struct TurnDirection : uint8_t { straight = 0, right = 1, left = 2 };

enum struct EventType : uint8_t { newGame = 0, pixel = 1, playerEliminated = 2, gameOver = 3, unknown = 4 };

struct ClientToServerMessage {
    static constexpr size_t minStructSize = 13, maxStructSize = 33;

    uint64_t sessionId;
    TurnDirection turnDirection;
    uint32_t nextExpectedEventNo;
    char playerName[MAX_PLAYER_NAME_LENGTH + 1];
    size_t playerNameSize;

    ClientToServerMessage(uint64_t, TurnDirection, uint32_t, const std::string &);
    ClientToServerMessage(const unsigned char *buffer, size_t size);
    size_t encode(unsigned char *buffer) const;
};

struct EventNewGame {
    static constexpr uint32_t maxPlayersFieldSize = MAX_PLAYERS * (MAX_PLAYER_NAME_LENGTH + 1);

    uint32_t maxx, maxy;
    unsigned char players[maxPlayersFieldSize];
    uint32_t playersFieldSize;

    uint32_t getSize() const {
        return sizeof(maxx) + sizeof(maxy) + playersFieldSize;
    }

    std::unordered_map<uint8_t, std::string> parsedPlayers() const;

    EventNewGame(uint32_t maxx, uint32_t maxy, const std::vector<std::string> &playerNames);
    EventNewGame(const unsigned char *buff, uint32_t size);
    uint32_t encode(unsigned char *buff) const;
};

struct EventPixel {
    uint8_t playerNumber;
    uint32_t x, y;

    uint32_t getSize() const {
        return sizeof(x) + sizeof(y) + sizeof(playerNumber);
    }

    EventPixel(uint8_t playerNumber, uint32_t x, uint32_t y) : playerNumber(playerNumber), x(x), y(y) {
    }
    EventPixel(const unsigned char *buff, size_t size);
    uint32_t encode(unsigned char *buff) const;
};

struct EventPlayerEliminated {
    uint8_t playerNumber;

    uint32_t getSize() const {
        return sizeof(playerNumber);
    }

    explicit EventPlayerEliminated(uint8_t playerNumber) : playerNumber(playerNumber) {
    }

    EventPlayerEliminated(const unsigned char *buff, size_t size) {
        if (size < 1) {
            throw EncoderDecoderError();
        }
        playerNumber = buff[0];
    }

    uint32_t encode(unsigned char *buff) const {
        buff[0] = playerNumber;
        return 1;
    }
};

struct EventGameOver {
    uint32_t getSize() const {
        return 0;
    }

    EventGameOver() = default;
    EventGameOver(const unsigned char *buff [[maybe_unused]], size_t size [[maybe_unused]]) {
    }

    uint32_t encode(unsigned char *buff [[maybe_unused]]) const {
        return 0;
    }
};

using GameEventVariant = std::variant<EventNewGame, EventPixel, EventPlayerEliminated, EventGameOver>;

struct Event {
    uint32_t eventNo;
    EventType eventType;
    GameEventVariant eventData;

    Event(uint32_t eventNo, EventType eventType, GameEventVariant eventVariant);
    Event(const unsigned char *buffer, size_t size, size_t *bytesUsed);
    uint32_t encode(unsigned char *buffer) const;

    uint32_t getEventDataSize() const;
    uint32_t getEncodedSize() const;
};

struct ServerToClientMessage {
    static constexpr size_t minStructSize = 4, maxStructSize = MAX_UDP_DATA_FIELD_SIZE;

    uint32_t gameId;
    std::vector<Event> events;

    ServerToClientMessage(uint32_t gameIdParam, std::vector<Event> pickedEvents)
        : gameId(gameIdParam), events(std::move(pickedEvents)) {
    }

    ServerToClientMessage(unsigned char *buffer, size_t size);
    size_t encode(unsigned char *buffer);

    static std::pair<uint32_t, ServerToClientMessage> fromEvents(uint32_t gameId, std::vector<Event>::const_iterator it,
                                                                 std::vector<Event>::const_iterator endIt);
};

std::string eventToMessageForGui(const Event &e, const std::unordered_map<uint8_t, std::string> &playerNames);

#endif // SIK_NETWORMS_MESSAGES_H
