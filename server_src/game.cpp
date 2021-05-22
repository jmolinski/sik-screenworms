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
        // New game id, we must create a new queue for it.
        nextGames.push_back({gameId, {ScheduledEvent{eventType, event}}});
    }
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

GameManager::GameManager(uint32_t rngSeed, uint8_t turningSpeed, uint16_t maxx, uint16_t maxy)
    : gameId(0), gameStarted(false), rng(rngSeed), maxx(maxx), maxy(maxy), turningSpeed(turningSpeed) {
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
    auto it = players.find(msg.playerName);
    // TODO walidacje i wszystko
    if (it == players.end()) {
        Player p;
        p.playerName = msg.playerName;
        p.fingerprint = fingerprint;
        p.takesPartInCurrentGame = false;  // TODO to nie jest respektowane
        p.isEliminated = false;
        p.isDisconnected = false;
        p.sessionId = msg.sessionId;
        players.insert({p.playerName, p});
        mqManager.addQueue(fingerprint, msg.nextExpectedEventNo);
    } else {
        auto &p = players.find(msg.playerName)->second;
        p.turnDirection = msg.turnDirection;
        mqManager.ack(fingerprint, msg.nextExpectedEventNo);
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

void GameManager::eliminatePlayer(Player &player) {
    player.isEliminated = true;
    alivePlayers -= 1;

    emitPlayerEliminatedEvent(player);

    if (alivePlayers < 2) {
        emitGameOver();
        gameStarted = false;
    }
}

void GameManager::saveEvent(EventType eventType, const GameEventVariant &event) {
    mqManager.schedule(gameId, eventType, event);
}

void GameManager::emitNewGameEvent() {
    std::cerr << "New Game event" << std::endl;
    std::vector<std::string> playerNames;
    for (const auto &p : players) {
        playerNames.push_back(p.first);
    }
    saveEvent(EventType::newGame, EventNewGame(maxx, maxy, playerNames));
}

void GameManager::emitGameOver() {
    std::cerr << "Game Over event " << std::endl;
    saveEvent(EventType::gameOver, EventGameOver());
}

void GameManager::emitPixelEvent(const Player &player) {
    const auto &playerNo = playerNumberInGame.find(player.playerName)->second;
    saveEvent(EventType::pixel, EventPixel(playerNo, player.pixel.x, player.pixel.y));
}

void GameManager::emitPlayerEliminatedEvent(const Player &player) {
    std::cerr << "Player eliminated event for player " << player.playerName << std::endl;
    const auto &playerNo = playerNumberInGame.find(player.playerName)->second;
    saveEvent(EventType::playerEliminated, EventPlayerEliminated(playerNo));
}

bool GameManager::canStartGame() {
    unsigned readyPlayers = 0;
    for (const auto &player : players) {
        if (player.second.turnDirection != TurnDirection::straight) {
            readyPlayers++;
        }
    }

    return readyPlayers > 1 && readyPlayers == players.size();
}

void GameManager::startGame() {
    gameId = rng.generateNext();
    gameStarted = true;
    alivePlayers = static_cast<uint8_t>(players.size());

    playerNumberInGame.clear();
    for (auto &p : players) {
        playerNumberInGame.insert({p.first, playerNumberInGame.size()});
    }
    emitNewGameEvent();

    for (auto &p : players) {
        auto &player = p.second;
        player.takesPartInCurrentGame = true;
        player.isEliminated = false; // TODO używane?

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
}

void GameManager::runTurn() {
    if (!gameStarted) {
        return;
    }

    for (auto &playerPair : players) {
        auto &player = playerPair.second;
        if (player.isEliminated || !player.takesPartInCurrentGame) {
            continue;
        }
        if (!gameStarted) {
            break;
        }

        if (player.turnDirection == TurnDirection::right) {
            player.movementDirection = static_cast<uint16_t>((player.movementDirection + turningSpeed) % 360);
        } else if (player.turnDirection == TurnDirection::left) {
            player.movementDirection = static_cast<uint16_t>((player.movementDirection + 360 - turningSpeed) % 360);
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
