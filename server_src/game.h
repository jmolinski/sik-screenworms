#ifndef SIK_NETWORMS_GAME_H
#define SIK_NETWORMS_GAME_H

#include "../common/fingerprint.h"
#include "../common/messages.h"
#include "../common/rng.h"
#include "../common/time_utils.h"
#include <deque>
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
    std::deque<utils::fingerprint_t> clientsToServeSchedule;

    std::vector<Event> currentGameEvents;
    gameid_t currentlyBroadcastedGameId;

    bool hasPendingMessagesForCurrentGame() const;

  public:
    bool isEmpty() const {
        return !hasPendingMessagesForCurrentGame() && nextGames.empty();
    }

    void schedule(uint32_t gameId, EventType eventType, const GameEventVariant &event);
    MessageAndRecipient getNextMessage();
    void ack(const utils::fingerprint_t &fingerprint, uint32_t nextExpectedEventNo);
    void dropQueue(const utils::fingerprint_t &fingerprint);
    void addQueue(const utils::fingerprint_t &fingerprint, uint32_t nextExpectedEventNo);
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
    bool isReadyToPlay;
};

class GameManager {
    uint32_t gameId;
    bool gameStarted;

    std::map<utils::fingerprint_t, Watcher> watchers;
    std::map<playername_t, Player> players;
    std::map<playername_t, uint8_t> playerNumberInGame;

    using int_coords = std::pair<uint16_t, uint16_t>;
    std::set<int_coords> eatenPixels;

    RNG rng;
    uint16_t maxx, maxy;
    uint8_t turningSpeed;
    uint8_t alivePlayers;

    timer_fd_t turnTimerFd;
    long turnIntervalNs;

    void handleMessageFromWatcher(const utils::fingerprint_t &, const ClientToServerMessage &);
    void handleMessageFromPlayer(const utils::fingerprint_t &, const ClientToServerMessage &);
    bool canStartGame();
    void startGame();
    void endGame();
    void eliminatePlayer(Player &);
    void eraseDisconnectedPlayers();

    void saveEvent(EventType eventType, const GameEventVariant &event);

    void emitPixelEvent(const Player &);
    void emitPlayerEliminatedEvent(const Player &);
    void emitNewGameEvent();
    void emitGameOver();

  public:
    GameManager(uint32_t rngSeed, uint8_t turningSpeed, uint16_t maxx, uint16_t maxy, timer_fd_t turnTimerFd,
                long turnIntervalNs);
    void runTurn();
    void handleMessage(const utils::fingerprint_t &fingerprint, ClientToServerMessage msg);
    void dropConnection(const utils::fingerprint_t &fingerprint);

    MQManager mqManager;
};

#endif // SIK_NETWORMS_GAME_H
