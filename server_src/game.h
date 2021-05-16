#ifndef SIK_NETWORMS_GAME_H
#define SIK_NETWORMS_GAME_H

#include <iostream>

constexpr unsigned MAX_PLAYERS = 27;

class GameManager {
  public:
    void runTurn() {
        std::cerr << "alarm" << std::endl;
    }
};

class Game {};

class MQManager {
  public:
    void schedule() {
    }
    bool isEmpty() {
        return true;
    }
};

#endif // SIK_NETWORMS_GAME_H
