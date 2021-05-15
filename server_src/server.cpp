#include "server.h"

#include <netdb.h>
#include <sys/socket.h>

Server::Server(ServerConfig parsedConfig) : config(parsedConfig) {
    addrinfo hints{AI_PASSIVE, AF_INET6, SOCK_DGRAM, 0, 0, nullptr, nullptr, nullptr};
    socket = UdpSocket(hints, "", config.port, true);

    // TODO create game controller, create mq, create timer
}

Server::~Server() {
}

void Server::run() {
}
