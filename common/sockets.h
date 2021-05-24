#ifndef SIK_NETWORMS_SOCKETS_H
#define SIK_NETWORMS_SOCKETS_H

#include <cstdint>
#include <netdb.h>
#include <string>
#include <vector>

using socket_fd_t = int;

class UdpSocket {
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

class TcpSocket {
    socket_fd_t fd;

    std::vector<char> outBuffer;
    std::vector<char> inBuffer;
    uint32_t unreadNewlines{};

  public:
    TcpSocket(addrinfo addrInfo, const std::string &hostname, uint16_t portNum);
    ~TcpSocket();

    [[nodiscard]] socket_fd_t getFd() const {
        return fd;
    }

    void writeData(const std::string &data);
    bool hasPendingOutgoingData();
    void flushOutgoing();

    std::pair<bool, std::string> readline();
    bool hasAvailableLine() const;
};

#endif // SIK_NETWORMS_SOCKETS_H
