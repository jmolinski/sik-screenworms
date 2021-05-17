#ifndef SIK_NETWORMS_CLIENT_H
#define SIK_NETWORMS_CLIENT_H

#include "../common/messages.h"
#include "../common/time_utils.h"
#include "../common/udp_socket.h"
#include "client_config.h"

namespace {
    constexpr size_t MSG_TO_SERVER_BUFFER_SIZE = 50;
} // namespace

struct MessageToServer {
    unsigned char data[MSG_TO_SERVER_BUFFER_SIZE];
    size_t size;
    bool ready;
};

class Client {
    ClientConfig config;
    UdpSocket serverSock;
    timer_fd_t timerFd;

    MessageToServer messageToServer;
    uint64_t sessionId;
    TurnDirection turnDirection;

  public:
    explicit Client(ClientConfig config);
    [[noreturn]] void run();

  private:
    void readMessageFromServer();
    void enqueueMessageToServer();
    void sendMessageToServer();
};

#endif // SIK_NETWORMS_CLIENT_H
