#ifndef SIK_NETWORMS_GAME_H
#define SIK_NETWORMS_GAME_H

#include "../common/messages.h"
#include "rng.h"

constexpr unsigned MAX_PLAYERS = 27;

class MQManager {
  public:
    void schedule() {
    }
    bool isEmpty() {
        return true;
    }
};

struct Player {
    uint64_t sessionId;
    uint8_t turnDirection;
    uint32_t nextExpectedEventNo;
    std::string playerName;

    bool isWatcher() const {
        return playerName.empty();
    };
};

class Game {};

class GameManager {
    RNG rng;

  public:
    explicit GameManager(uint32_t rngSeed);
    void runTurn();
    void handleMessage(const std::string &fingerprint, ClientToServerMessage msg);
};

#endif // SIK_NETWORMS_GAME_H
