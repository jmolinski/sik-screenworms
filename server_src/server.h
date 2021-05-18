#ifndef SIK_NETWORMS_SERVER_H
#define SIK_NETWORMS_SERVER_H

#include "../common/fingerprint.h"
#include "../common/sockets.h"
#include "../common/time_utils.h"
#include "game.h"
#include "server_config.h"
#include <unordered_map>

class Server {
    ServerConfig config;
    UdpSocket sock;
    timer_fd_t turnTimerFd;
    timer_fd_t expirationTimerFd;
    GameManager gameManager;

    std::unordered_map<utils::fingerprint_t, addrinfo> clientAddrs;
    std::unordered_map<utils::fingerprint_t, utils::time_stamp_t> lastCommunicationTs;

  public:
    explicit Server(ServerConfig config);
    [[noreturn]] void run();

  private:
    void readMessageFromClient();
    void sendMessageToClient();
    void cleanupObsoleteConnections();
};

#endif // SIK_NETWORMS_SERVER_H
