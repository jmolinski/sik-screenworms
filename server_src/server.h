#ifndef SIK_NETWORMS_SERVER_H
#define SIK_NETWORMS_SERVER_H

#include "../utils/interval_timer.h"
#include "../utils/udp_socket.h"
#include "server_config.h"

class Server {
    ServerConfig config;
    UdpSocket socket;
    timer_fd_t timerFd;

  public:
    explicit Server(ServerConfig config);
    ~Server();

    void run();
};

#endif // SIK_NETWORMS_SERVER_H
