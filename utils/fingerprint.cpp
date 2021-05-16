#include "fingerprint.h"
#include <cstring>
#include <endian.h>

static inline const void *getInAddr(const sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &reinterpret_cast<const sockaddr_in *>(sa)->sin_addr;
    }
    return &reinterpret_cast<const sockaddr_in6 *>(sa)->sin6_addr;
}

static inline uint16_t getInPort(const sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return be16toh(reinterpret_cast<const sockaddr_in *>(sa)->sin_port);
    }
    return be16toh(reinterpret_cast<const sockaddr_in6 *>(sa)->sin6_port);
}

std::string fingerprintNetuser(const sockaddr *addr) {
    static char s[INET6_ADDRSTRLEN + 1];
    memset(s, 0, sizeof s);

    if (inet_ntop(addr->sa_family, getInAddr(addr), s, (sizeof s) - 1) == nullptr) {
        return "";
    } else {
        return std::string(s) + ":" + std::to_string(getInPort(addr));
    }
}
