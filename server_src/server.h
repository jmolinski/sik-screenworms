#ifndef SIK_NETWORMS_SERVER_H
#define SIK_NETWORMS_SERVER_H

#include "../utils/interval_timer.h"
#include "../utils/udp_socket.h"
#include "server_config.h"
#include "game.h"

class Server {
    ServerConfig config;
    UdpSocket socket;
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
