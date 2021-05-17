#ifndef SIK_NETWORMS_GAME_H
#define SIK_NETWORMS_GAME_H

#include "../common/fingerprint.h"
#include "../common/messages.h"
#include "rng.h"
#include <map>
#include <set>
#include <vector>

constexpr unsigned MAX_PLAYERS = 27;

struct ClientOutgoingMsgQueue {
    uint32_t nextExpectedEventNo;
};

class MQManager {
    std::map<utils::fingerprint_t, ClientOutgoingMsgQueue> queues;

  public:
    bool isEmpty() {
        // TODO
        return true;
    }

    void schedule() {
    }

    void ack(const utils::fingerprint_t &fingerprint, uint32_t nextExpectedEventNo) {
        // TODO
    }

    void dropQueue(const utils::fingerprint_t &fingerprint) {
        queues.erase(fingerprint);
    }

    void addQueue(const utils::fingerprint_t &fingerprint, uint32_t nextExpectedEventNo) {
        queues.insert({fingerprint, {nextExpectedEventNo}});
    }
};

struct Watcher {
    utils::fingerprint_t fingerprint;
    uint64_t sessionId;
};

struct Player {
    utils::fingerprint_t fingerprint;
    std::string playerName;

    struct {
        double x, y;
    } coords;

    uint64_t sessionId;
    uint32_t nextExpectedEventNo;

    struct {
        uint16_t x, y;
    } pixel;

    uint16_t movementDirection;
    TurnDirection turnDirection;
    bool takesPartInCurrentGame;
    bool isDisconnected;
    bool isEliminated;
};

struct Event {};

class GameManager {
    uint32_t gameId;
    bool gameStarted;

    std::map<utils::fingerprint_t, Watcher> watchers;
    std::map<std::string, Player> players;

    using int_coords = std::pair<uint16_t, uint16_t>;
    std::set<int_coords> eatenPixels;

    std::vector<Event> events;

    RNG rng;
    uint16_t maxx, maxy;
    uint8_t turningSpeed;
    uint8_t alivePlayers;

    void eliminatePlayer(Player &);
    void emitPixelEvent(const Player &);
    void handleMessageFromWatcher(const utils::fingerprint_t &, const ClientToServerMessage &);
    void handleMessageFromPlayer(const utils::fingerprint_t &, const ClientToServerMessage &);

  public:
    GameManager(uint32_t rngSeed, uint8_t turningSpeed, uint16_t maxx, uint16_t maxy);
    void runTurn();
    void handleMessage(const utils::fingerprint_t &fingerprint, ClientToServerMessage msg);

    MQManager mqManager;
};

#endif // SIK_NETWORMS_GAME_H
