#ifndef SIK_NETWORMS_CLIENT_H
#define SIK_NETWORMS_CLIENT_H

#include "../common/messages.h"
#include "../common/sockets.h"
#include "../common/time_utils.h"
#include "client_config.h"
#include <unordered_map>

namespace {
    constexpr size_t MSG_TO_SERVER_BUFFER_SIZE = 50;
} // namespace

struct MessageToServer {
    unsigned char data[MSG_TO_SERVER_BUFFER_SIZE];
    size_t size;
    bool ready;
};

struct DecodedEvent {
    std::string guiFormat;
    uint32_t eventNo;
};

class Client {
    ClientConfig config;
    UdpSocket serverSock;
    TcpSocket guiSock;
    timer_fd_t timerFd;

    MessageToServer messageToServer;
    uint64_t sessionId;
    TurnDirection turnDirection;

    uint32_t nextWantedEventNo;  // TODO ?
    uint32_t nextEventToSendNo;  // TODO ?
    std::unordered_map<uint32_t, DecodedEvent> events;

  public:
    explicit Client(ClientConfig config);
    [[noreturn]] void run();

  private:
    void readMessageFromServer();
    void enqueueMessageToServer();
    void sendMessageToServer();
    void readUpdateFromGui();
};

#endif // SIK_NETWORMS_CLIENT_H
