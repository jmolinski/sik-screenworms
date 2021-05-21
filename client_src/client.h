#ifndef SIK_NETWORMS_CLIENT_H
#define SIK_NETWORMS_CLIENT_H

#include "../common/messages.h"
#include "../common/sockets.h"
#include "../common/time_utils.h"
#include "client_config.h"
#include <unordered_map>
#include <unordered_set>

namespace {
    constexpr size_t MSG_TO_SERVER_BUFFER_SIZE = 50;
} // namespace

struct MessageToServer {
    unsigned char data[MSG_TO_SERVER_BUFFER_SIZE];
    size_t size;
    bool ready;
};

class Client {
    // Client config & persistent objects like sockets.
    ClientConfig config;
    UdpSocket serverSock;
    TcpSocket guiSock;
    timer_fd_t timerFd;

    // Fields relating to client->server communication.
    MessageToServer messageToServer;
    uint64_t sessionId;
    TurnDirection turnDirection;

    // Current game events & fields relating to client->gui communication.
    std::unordered_map<uint32_t, Event> events;
    uint32_t nextWantedEventNo;
    uint32_t nextEventToSendNo;

    // Registry of played games, current gameId.
    std::unordered_set<uint32_t> seenGameIds;
    int64_t gameId;
    uint8_t playersInGame;

    // Current game settings.
    uint32_t maxx, maxy;
    std::unordered_map<uint8_t, std::string> playerNames;

  public:
    explicit Client(ClientConfig config);
    [[noreturn]] void run();

  private:
    void readMessageFromServer();
    void enqueueMessageToServer();
    void sendMessageToServer();
    void readUpdateFromGui();
    void processMessageFromServer(const ServerToClientMessage &);
    void processMessageFromServerWithMismatchedGameId(const ServerToClientMessage &);
    void readInEventsListOfCurrentGame(const ServerToClientMessage &msg);
    void validateEvent(const Event &event);
};

#endif // SIK_NETWORMS_CLIENT_H
