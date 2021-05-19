#include "sockets.h"
#include "fingerprint.h"
#include <iostream>
#include <netinet/tcp.h>
#include <unistd.h>

constexpr unsigned TCP_BUFF_SIZE = 1000;

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
        std::cout << "Successfully created socket: " << utils::fingerprintNetuser(info.ai_addr) << std::endl;
    }

    freeaddrinfo(servinfo);
}

UdpSocket::~UdpSocket() {
    if (fd != -1) {
        std::cout << "Closing UDP socket" << std::endl;
        close(fd);
    }
}

void UdpSocket::connectPeer(const addrinfo &peer) {
    if (connect(fd, peer.ai_addr, peer.ai_addrlen) < 0) {
        perror("connect on UDP socket failed");
        exit(EXIT_FAILURE);
    }
}

TcpSocket::TcpSocket(addrinfo addrInfo, const std::string &hostname, uint16_t portNum) {
    addrinfo *servinfo;
    std::string port = std::to_string(portNum);
    int status = getaddrinfo(hostname.c_str(), port.c_str(), &addrInfo, &servinfo);
    if (status != 0) {
        std::cerr << "ERROR: getaddrinfo error: " << gai_strerror(status) << std::endl;
        exit(EXIT_FAILURE);
    }

    // Loop through all the results, create socket and connect to the first we can.
    for (addrinfo *p = servinfo; p != nullptr; p = p->ai_next) {
        fd = -1;
        if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }
        if (connect(fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(fd);
            fd = -1;
            continue;
        }
        int yes = 1;
        if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) != 0) {
            close(fd);
            fd = -1;
            continue;
        }
        break;
    }
    freeaddrinfo(servinfo);

    if (fd == -1) {
        std::cerr << "Failed to create TCP socket" << std::endl;
        exit(EXIT_FAILURE);
    }
}

TcpSocket::~TcpSocket() {
    if (fd != -1) {
        std::cout << "Closing TCP socket" << std::endl;
        close(fd);
    }
}

void TcpSocket::writeData(const std::string &data) {
    for (const char &c : data) {
        outBuffer.push_back(c);
    }
}

bool TcpSocket::hasPendingOutgoingData() {
    return !outBuffer.empty();
}

void TcpSocket::flushOutgoing() {
    if (!hasPendingOutgoingData()) {
        return;
    }

    errno = 0;
    ssize_t written = write(fd, outBuffer.data(), outBuffer.size());
    if (written < 0) {
        if (errno == EPIPE) {
            throw std::runtime_error("peer clossed connection");
        }
        perror("problem when trying to write to tcp socket");
    } else {
        auto offset = static_cast<size_t>(written);
        if (offset == outBuffer.size()) {
            outBuffer.clear();
        } else {
            size_t leftInBuffer = outBuffer.size() - offset;
            for (size_t i = 0; i < leftInBuffer; i++) {
                outBuffer[i] = outBuffer[offset + i];
            }
            outBuffer.resize(leftInBuffer);
        }
    }
}

std::pair<bool, std::string> TcpSocket::readline() {
    if (hasAvailableLine()) {
        for (size_t i = 0;; i++) {
            if (inBuffer[i] != '\n') {
                continue;
            }

            std::string line(inBuffer.data(), inBuffer.data() + i);
            inBuffer.resize(inBuffer.size() - i - 1);
            unreadNewlines--;
            break;
        }
    }

    static char buffer[TCP_BUFF_SIZE];
    ssize_t readBytes = read(fd, &buffer, TCP_BUFF_SIZE);
    if (readBytes == 0) {
        throw std::runtime_error("peer clossed connection");
    }
    if (readBytes < 0) {
        perror("error happened when trying to read from tcp socket");
        return {false, ""};
    }

    for (ssize_t i = 0; i < readBytes; i++) {
        if (buffer[i] == '\n') {
            unreadNewlines++;
        }
        inBuffer.push_back(buffer[i]);
    }

    return {false, ""};
}

bool TcpSocket::hasAvailableLine() const {
    return unreadNewlines > 0;
}
