#ifndef SIK_NETWORMS_SERVER_H
#define SIK_NETWORMS_SERVER_H

#include "../common/time_utils.h"
#include "../common/udp_socket.h"
#include "game.h"
#include "server_config.h"

class Server {
    ServerConfig config;
    UdpSocket sock;
    timer_fd_t timerFd;
    GameManager gameManager;
    MQManager mqManager;

  public:
    explicit Server(ServerConfig config);
    [[noreturn]] void run();

  private:
    void readMessageFromClient();
    void sendMessageToClient();
};

#endif // SIK_NETWORMS_SERVER_H
