#include "game.h"
#include <cmath>
#include <iostream>

GameManager::GameManager(uint32_t rngSeed, uint8_t turningSpeed, uint16_t maxx, uint16_t maxy)
    : gameId(0), rng(rngSeed), maxx(maxx), maxy(maxy), turningSpeed(turningSpeed) {
}

void GameManager::handleMessage(const std::string &fingerprint, ClientToServerMessage msg) {
    std::cerr << "DEBUG MSG: sessionid " << msg.sessionId << " turn "
              << (uint32_t) static_cast<uint8_t>(msg.turnDirection) << " eventNo " << msg.nextExpectedEventNo
              << " pname " << msg.playerName << std::endl;

    std::cerr << fingerprint << std::endl;
}

void GameManager::eliminatePlayer(Player &player) {
    std::cerr << "Player " << player.playerName << " eliminated" << std::endl;
    player.isEliminated = true;

    // TODO potentially end game
}

void GameManager::emitPixelEvent(const Player &player) {
    std::cerr << "Pixel event for player " << player.playerName << std::endl;
}

void GameManager::runTurn() {
    std::cerr << "turn alarm" << std::endl;

    if (!gameStarted) {
        return;
    }

    for (auto &playerPair : players) {
        auto &player = playerPair.second;
        if (player.isEliminated) {
            continue;
        }
        if (!gameStarted) {
            break;
        }

        if (player.turnDirection == TurnDirection::right) {
            player.movementDirection += turningSpeed;
        } else if (player.turnDirection == TurnDirection::left) {
            player.movementDirection -= turningSpeed;
        }

        double movementDirRad = player.movementDirection * M_PI / 180.0;
        player.coords.x += std::cos(movementDirRad);
        player.coords.y += std::sin(movementDirRad);

        if (player.coords.x < 0 || player.coords.y < 0) {
            eliminatePlayer(player);
            continue;
        }

        auto x = static_cast<uint16_t>(player.coords.x);
        auto y = static_cast<uint16_t>(player.coords.y);
        if (x == player.pixel.x && y == player.pixel.y) {
            continue;
        }
        player.pixel.x = x;
        player.pixel.y = y;

        if (x >= maxx || y >= maxy || eatenPixels.find({x, y}) != eatenPixels.end()) {
            eliminatePlayer(player);
        } else {
            emitPixelEvent(player);
        }
    }
}
