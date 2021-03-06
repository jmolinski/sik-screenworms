#include "game.h"
#include <cmath>
#include <iostream>

void MQManager::schedule(uint32_t gameId, EventType eventType, const GameEventVariant &event) {
    if (gameId == currentlyBroadcastedGameId) {
        auto eventNo = static_cast<uint32_t>(currentGameEvents.size());
        currentGameEvents.emplace_back(eventNo, eventType, event);
    } else {
        for (auto &upcomingGame : nextGames) {
            if (upcomingGame.first == gameId) {
                upcomingGame.second.emplace_back(eventType, event);
                return;
            }
        }
        // New game id, we must create a new container for it.
        nextGames.push_back({gameId, {ScheduledEvent{eventType, event}}});
    }
}

bool MQManager::hasPendingMessagesForCurrentGame() const {
    for (const auto &queue : queues) {
        if (queue.second < currentGameEvents.size()) {
            return true;
        }
    }
    return false;
}

MessageAndRecipient MQManager::getNextMessage() {
    if (!hasPendingMessagesForCurrentGame() && !nextGames.empty()) {
        for (auto &queue : queues) {
            queue.second = 0;
        }
        currentlyBroadcastedGameId = nextGames[0].first;
        currentGameEvents.clear();
        for (const auto &x : nextGames[0].second) {
            auto eventNo = static_cast<uint32_t>(currentGameEvents.size());
            currentGameEvents.emplace_back(eventNo, x.first, x.second);
        }

        nextGames.erase(nextGames.begin());
    }

    while (true) {
        auto fprint = clientsToServeSchedule.front();
        clientsToServeSchedule.pop_front();
        clientsToServeSchedule.push_back(fprint);

        auto &clientNextWanted = queues.find(fprint)->second;
        if (clientNextWanted < currentGameEvents.size()) {
            auto [itemsRead, msg] = ServerToClientMessage::fromEvents(
                currentlyBroadcastedGameId, currentGameEvents.cbegin() + clientNextWanted, currentGameEvents.cend());
            return {fprint, clientNextWanted + itemsRead, msg};
        }
    }
}

void MQManager::ack(const utils::fingerprint_t &fingerprint, uint32_t nextExpectedEventNo) {
    if (nextGames.empty()) {
        uint32_t newValue = std::min(static_cast<uint32_t>(currentGameEvents.size()), nextExpectedEventNo);
        queues.find(fingerprint)->second = newValue;
    }
}

void MQManager::dropQueue(const utils::fingerprint_t &fingerprint) {
    if (queues.find(fingerprint) != queues.end()) {
        queues.erase(fingerprint);
    }
    auto it = clientsToServeSchedule.begin();
    while (it != clientsToServeSchedule.end()) {
        if (*it == fingerprint) {
            clientsToServeSchedule.erase(it);
            return;
        }
        it++;
    }
}

void MQManager::addQueue(const utils::fingerprint_t &fingerprint, uint32_t nextExpectedEventNo) {
    if (queues.find(fingerprint) != queues.end()) {
        return;
    }
    uint32_t nextExpected = std::min(static_cast<uint32_t>(currentGameEvents.size()), nextExpectedEventNo);
    queues.insert({fingerprint, nextExpected});
    clientsToServeSchedule.push_back(fingerprint);
}

GameManager::GameManager(uint32_t rngSeed, uint8_t turningSpeed, uint16_t maxx, uint16_t maxy, timer_fd_t turnTimerFd,
                         long turnIntervalNs)
    : gameId(0), gameStarted(false), rng(rngSeed), maxx(maxx), maxy(maxy), turningSpeed(turningSpeed),
      turnTimerFd(turnTimerFd), turnIntervalNs(turnIntervalNs) {
    utils::setIntervalTimer(turnTimerFd, turnIntervalNs);
}

void GameManager::handleMessageFromWatcher(const utils::fingerprint_t &fingerprint, const ClientToServerMessage &msg) {
    auto it = watchers.find(fingerprint);
    if (it == watchers.end()) {
        for (const auto &p : players) {
            if (p.second.fingerprint == fingerprint) {
                return;
            }
        }
        watchers.insert({fingerprint, {msg.sessionId}});
        mqManager.addQueue(fingerprint, msg.nextExpectedEventNo);
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
    auto it = players.find(msg.playerName);

    if (it != players.end()) {
        auto &p = it->second;
        if (p.fingerprint != fingerprint) {
            return;
        }
        p.turnDirection = msg.turnDirection;
        if (!gameStarted && msg.turnDirection != TurnDirection::straight) {
            p.isReadyToPlay = true;
        }
        mqManager.ack(fingerprint, msg.nextExpectedEventNo);
    } else {
        for (const auto &p : players) {
            if (p.second.fingerprint == fingerprint && msg.sessionId > p.second.sessionId) {
                dropConnection(fingerprint);
            }
        }
        if (players.size() == MAX_PLAYERS) {
            return;
        }

        Player p;
        p.playerName = msg.playerName;
        p.fingerprint = fingerprint;
        p.sessionId = msg.sessionId;

        p.takesPartInCurrentGame = false;
        p.isEliminated = false;
        p.isDisconnected = false;
        p.isReadyToPlay = false;
        if (!gameStarted && msg.turnDirection != TurnDirection::straight) {
            p.isReadyToPlay = true;
        }
        p.turnDirection = msg.turnDirection;

        players.insert({p.playerName, p});
        mqManager.addQueue(fingerprint, msg.nextExpectedEventNo);
    }

    if (!gameStarted) {
        if (canStartGame()) {
            startGame();
        }
    }
}

void GameManager::handleMessage(const utils::fingerprint_t &fingerprint, ClientToServerMessage msg) {
    if (msg.playerNameSize == 0) {
        handleMessageFromWatcher(fingerprint, msg);
    } else {
        handleMessageFromPlayer(fingerprint, msg);
    }
}

void GameManager::dropConnection(const utils::fingerprint_t &fingerprint) {
    mqManager.dropQueue(fingerprint);

    if (watchers.find(fingerprint) != watchers.end()) {
        watchers.erase(fingerprint);
        return;
    }

    for (auto &p : players) {
        if (p.second.fingerprint == fingerprint) {
            p.second.isDisconnected = true;
            break;
        }
    }
    if (!gameStarted) {
        eraseDisconnectedPlayers();
    }
}

void GameManager::eliminatePlayer(Player &player) {
    player.isEliminated = true;
    alivePlayers -= 1;

    emitPlayerEliminatedEvent(player);

    if (alivePlayers < 2) {
        endGame();
    }
}

void GameManager::saveEvent(EventType eventType, const GameEventVariant &event) {
    mqManager.schedule(gameId, eventType, event);
}

void GameManager::emitNewGameEvent() {
    std::vector<std::string> playerNames;
    for (const auto &p : players) {
        playerNames.push_back(p.first);
    }
    saveEvent(EventType::newGame, EventNewGame(maxx, maxy, playerNames));
}

void GameManager::emitGameOver() {
    saveEvent(EventType::gameOver, EventGameOver());
}

void GameManager::emitPixelEvent(const Player &player) {
    const auto &playerNo = playerNumberInGame.find(player.playerName)->second;
    saveEvent(EventType::pixel, EventPixel(playerNo, player.pixel.x, player.pixel.y));
}

void GameManager::emitPlayerEliminatedEvent(const Player &player) {
    const auto &playerNo = playerNumberInGame.find(player.playerName)->second;
    saveEvent(EventType::playerEliminated, EventPlayerEliminated(playerNo));
}

bool GameManager::canStartGame() {
    unsigned readyPlayers = 0;
    for (const auto &player : players) {
        if (player.second.isReadyToPlay) {
            readyPlayers++;
        }
    }

    return readyPlayers > 1 && readyPlayers == players.size();
}

void GameManager::startGame() {
    gameId = rng.generateNext();
    gameStarted = true;
    alivePlayers = static_cast<uint8_t>(players.size());
    eatenPixels.clear();

    playerNumberInGame.clear();
    for (auto &p : players) {
        playerNumberInGame.insert({p.first, playerNumberInGame.size()});
    }
    emitNewGameEvent();

    for (auto &p : players) {
        auto &player = p.second;
        player.takesPartInCurrentGame = true;
        player.isEliminated = false;

        player.coords.x = rng.generateNext() % maxx + 0.5;
        player.coords.y = rng.generateNext() % maxy + 0.5;
        player.movementDirection = static_cast<uint16_t>(rng.generateNext() % 360);

        player.pixel.x = static_cast<uint16_t>(player.coords.x);
        player.pixel.y = static_cast<uint16_t>(player.coords.y);

        if (eatenPixels.find({player.pixel.x, player.pixel.y}) != eatenPixels.end()) {
            eliminatePlayer(player);
        } else {
            eatenPixels.insert({player.pixel.x, player.pixel.y});
            emitPixelEvent(player);
        }

        if (!gameStarted) {
            // Game might have ended on player elimination above.
            break;
        }
    }
    if (gameStarted) {
        utils::setIntervalTimer(turnTimerFd, turnIntervalNs);
        utils::clearTimer(turnTimerFd);
    }
}

void GameManager::endGame() {
    emitGameOver();
    gameStarted = false;

    eraseDisconnectedPlayers();
}

void GameManager::eraseDisconnectedPlayers() {
    std::vector<std::pair<playername_t, utils::fingerprint_t>> playersToErase;
    for (auto &p : players) {
        p.second.isReadyToPlay = false;

        if (p.second.isDisconnected) {
            playersToErase.emplace_back(p.first, p.second.fingerprint);
        }
    }
    for (const auto &p : playersToErase) {
        players.erase(p.first);
        mqManager.dropQueue(p.second);
    }
}

void GameManager::runTurn() {
    for (auto &playerPair : players) {
        if (!gameStarted) {
            break;
        }

        auto &player = playerPair.second;
        if (player.isEliminated || !player.takesPartInCurrentGame) {
            continue;
        }
        if (player.turnDirection == TurnDirection::right) {
            player.movementDirection = static_cast<uint16_t>((player.movementDirection + turningSpeed) % 360);
        } else if (player.turnDirection == TurnDirection::left) {
            player.movementDirection = static_cast<uint16_t>((player.movementDirection + 360 - turningSpeed) % 360);
        }

        double movementDirRad = player.movementDirection / 180.0 * M_PI;
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

        if (x >= maxx || y >= maxy || eatenPixels.find({x, y}) != eatenPixels.end()) {
            eliminatePlayer(player);
        } else {
            player.pixel.x = x;
            player.pixel.y = y;
            eatenPixels.insert({x, y});
            emitPixelEvent(player);
        }
    }
}
