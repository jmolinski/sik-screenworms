#ifndef SIK_NETWORMS_UDP_SOCKET_H
#define SIK_NETWORMS_UDP_SOCKET_H

#include <cstdint>

class UdpSocket {
    using socket_fd_t = int;

    socket_fd_t fd;

  public:
    explicit UdpSocket(uint16_t port);

    socket_fd_t getFd() const {
        return fd;
    }
};

#endif // SIK_NETWORMS_UDP_SOCKET_H
