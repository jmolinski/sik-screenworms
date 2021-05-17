#include "game.h"
#include <iostream>

GameManager::GameManager(uint32_t rngSeed) : rng(rngSeed) {
}

void GameManager::handleMessage(const std::string &fingerprint, ClientToServerMessage msg) {
    std::cerr << "DEBUG MSG: sessionid " << msg.sessionId << " turn "
              << (uint32_t) static_cast<uint8_t>(msg.turnDirection) << " eventNo " << msg.nextExpectedEventNo
              << " pname " << msg.playerName << std::endl;

    std::cerr << fingerprint << std::endl;
}

void GameManager::runTurn() {
    std::cerr << "turn alarm" << std::endl;
}
