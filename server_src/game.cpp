#include "game.h"
#include <cmath>
#include <iostream>

GameManager::GameManager(uint32_t rngSeed, uint8_t turningSpeed, uint16_t maxx, uint16_t maxy)
    : gameId(0), rng(rngSeed), maxx(maxx), maxy(maxy), turningSpeed(turningSpeed) {
}

void GameManager::handleMessageFromWatcher(const utils::fingerprint_t &fingerprint, const ClientToServerMessage &msg) {
    auto it = watchers.find(fingerprint);
    if (it == watchers.end()) {
        if (watchers.size() + players.size() < MAX_PLAYERS) {
            watchers.insert({fingerprint, {fingerprint, msg.sessionId}});
            mqManager.addQueue(fingerprint, msg.nextExpectedEventNo);
        }
    } else {
        auto &watcher = it->second;
        if (watcher.sessionId == msg.sessionId) {
            mqManager.ack(fingerprint, msg.nextExpectedEventNo);
        } else if (watcher.sessionId < msg.sessionId) {
            mqManager.dropQueue(fingerprint);
            mqManager.addQueue(fingerprint, msg.nextExpectedEventNo);
            watcher.sessionId = msg.sessionId;
        }
    }
}

void GameManager::handleMessageFromPlayer(const utils::fingerprint_t &fingerprint, const ClientToServerMessage &msg) {
    auto it = players.find(fingerprint);
    if (it == players.end()) {
        // TODO add new player
    } else {
        // TODO update player
    }
}

void GameManager::handleMessage(const utils::fingerprint_t &fingerprint, ClientToServerMessage msg) {
    std::cerr << "DEBUG MSG: sessionid " << msg.sessionId << " turn "
              << (uint32_t) static_cast<uint8_t>(msg.turnDirection) << " eventNo " << msg.nextExpectedEventNo
              << " pname " << msg.playerName << std::endl;

    if (msg.playerNameSize == 0) {
        handleMessageFromWatcher(fingerprint, msg);
    } else {
        handleMessageFromPlayer(fingerprint, msg);
    }
}

void GameManager::eliminatePlayer(Player &player) {
    std::cerr << "Player " << player.playerName << " eliminated" << std::endl;
    player.isEliminated = true;
    alivePlayers -= 1;

    if (alivePlayers < 2) {
        // TODO emit GAME_OVER
    }
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
            player.movementDirection = (player.movementDirection + turningSpeed) % 360;
        } else if (player.turnDirection == TurnDirection::left) {
            player.movementDirection = (player.movementDirection + 360 - turningSpeed) % 360;
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
            eatenPixels.insert({x, y});
            emitPixelEvent(player);
        }
    }
}
