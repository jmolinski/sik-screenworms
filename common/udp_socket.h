#ifndef SIK_NETWORMS_UDP_SOCKET_H
#define SIK_NETWORMS_UDP_SOCKET_H

#include <cstdint>
#include <netdb.h>
#include <string>

class UdpSocket {
    using socket_fd_t = int;

    socket_fd_t fd;
    addrinfo info;

  public:
    UdpSocket(addrinfo addrInfo, const std::string &hostname, uint16_t portNum, bool doBind = false);
    ~UdpSocket();

    [[nodiscard]] socket_fd_t getFd() const {
        return fd;
    }

    [[nodiscard]] addrinfo getAddrInfo() const {
        return info;
    }
};

#endif // SIK_NETWORMS_UDP_SOCKET_H
