#ifndef SIK_NETWORMS_GAME_H
#define SIK_NETWORMS_GAME_H

#include "../common/fingerprint.h"
#include "../common/messages.h"
#include "../common/rng.h"
#include <map>
#include <set>
#include <vector>

using gameid_t = uint32_t;
using playername_t = std::string;

using MessageAndRecipient = std::tuple<utils::fingerprint_t, uint32_t, ServerToClientMessage>;

class MQManager {
    using ScheduledEvent = std::pair<EventType, GameEventVariant>;

    std::vector<std::pair<gameid_t, std::vector<ScheduledEvent>>> nextGames;

    std::map<utils::fingerprint_t, uint32_t> queues;
    std::vector<Event> currentGameEvents;
    gameid_t currentlyBroadcastedGameId;

    bool hasPendingMessagesForCurrentGame() const {
        for (const auto &queue : queues) {
            if (queue.second < currentGameEvents.size()) {
                return true;
            }
        }
        return false;
    }

  public:
    bool isEmpty() const {
        return !hasPendingMessagesForCurrentGame() && nextGames.empty();
    }

    void schedule(uint32_t gameId, EventType eventType, const GameEventVariant &event);
    MessageAndRecipient getNextMessage();

    void ack(const utils::fingerprint_t &fingerprint, uint32_t nextExpectedEventNo) {
        queues.find(fingerprint)->second =
            std::min(static_cast<uint32_t>(currentGameEvents.size()), nextExpectedEventNo);
    }

    void dropQueue(const utils::fingerprint_t &fingerprint) {
        queues.erase(fingerprint);
    }

    void addQueue(const utils::fingerprint_t &fingerprint, uint32_t nextExpectedEventNo) {
        queues.insert({fingerprint, nextExpectedEventNo});
    }
};

struct Watcher {
    utils::fingerprint_t fingerprint;
    uint64_t sessionId;
};

struct Player {
    utils::fingerprint_t fingerprint;
    playername_t playerName;

    struct {
        double x, y;
    } coords;

    uint64_t sessionId;

    struct {
        uint16_t x, y;
    } pixel;

    uint16_t movementDirection;
    TurnDirection turnDirection;
    bool takesPartInCurrentGame;
    bool isDisconnected;
    bool isEliminated;
};

class GameManager {
    uint32_t gameId;
    bool gameStarted;

    std::map<utils::fingerprint_t, Watcher> watchers;
    std::map<std::string, Player> players;

    using int_coords = std::pair<uint16_t, uint16_t>;
    std::set<int_coords> eatenPixels;

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
