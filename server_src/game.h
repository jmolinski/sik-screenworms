#ifndef SIK_NETWORMS_GAME_H
#define SIK_NETWORMS_GAME_H

#include <iostream>

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
  public:
    void runTurn() {
        std::cerr << "alarm" << std::endl;
    }
};

#endif // SIK_NETWORMS_GAME_H
