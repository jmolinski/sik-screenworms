#ifndef SIK_NETWORMS_CLIENT_H
#define SIK_NETWORMS_CLIENT_H

#include "../common/udp_socket.h"
#include "client_config.h"

class Client {
    ClientConfig config;
    UdpSocket serverSocket;

  public:
    explicit Client(ClientConfig config);
    ~Client();

    void run();
};

#endif // SIK_NETWORMS_CLIENT_H
