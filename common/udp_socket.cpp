#include "udp_socket.h"
#include "fingerprint.h"
#include <iostream>
#include <unistd.h>

UdpSocket::UdpSocket() : fd{-1}, info{0, 0, 0, 0, 0, nullptr, nullptr, nullptr} {
}

UdpSocket::UdpSocket(addrinfo addrInfo, const std::string &hostname, uint16_t portNum, bool doBind) {
    addrinfo *servinfo;
    std::string port = std::to_string(portNum);
    int status = getaddrinfo(hostname.empty() ? nullptr : hostname.c_str(), port.c_str(), &addrInfo, &servinfo);
    if (status != 0) {
        std::cerr << "ERROR: getaddrinfo error: " << gai_strerror(status) << std::endl;
        exit(EXIT_FAILURE);
    }

    // Loop through all the results, create socket and potentially bind to the first we can.
    for (addrinfo *p = servinfo; p != nullptr; p = p->ai_next) {
        fd = -1;
        if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }
        if (doBind) {
            int rv = 0;
            int yes = 1;
            rv += setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
            if (p->ai_family == AF_INET6) {
                int no = 0;
                rv += setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(int));
            }
            if (rv != 0) {
                close(fd);
                fd = -1;
                continue;
            }
            if (bind(fd, p->ai_addr, p->ai_addrlen) == -1) {
                close(fd);
                fd = -1;
                continue;
            }
        }

        info = *p;
        info.ai_next = nullptr;
        info.ai_canonname = nullptr;
        break;
    }

    if (fd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (doBind) {
        std::cout << "Successfully created socket: " << fingerprintNetuser(info.ai_addr) << std::endl;
    }

    freeaddrinfo(servinfo);
}

UdpSocket::~UdpSocket() {
    if (fd != -1) {
        std::cout << "Closing UDP socket" << std::endl;
        close(fd);
    }
}
