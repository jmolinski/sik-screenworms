#ifndef SIK_NETWORMS_SERVER_H
#define SIK_NETWORMS_SERVER_H

#include "server_config.h"
#include "../utils/udp_socket.h"

class Server {
    ServerConfig config;
    UdpSocket socket;

  public:
    explicit Server(ServerConfig config);
    ~Server();

    void run();
};

#endif // SIK_NETWORMS_SERVER_H
