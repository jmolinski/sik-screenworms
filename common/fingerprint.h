#ifndef SIK_NETWORMS_FINGERPRINT_H
#define SIK_NETWORMS_FINGERPRINT_H

#include <arpa/inet.h>
#include <string>

namespace utils {
    using fingerprint_t = std::string;

    fingerprint_t fingerprintNetuser(const sockaddr *addr);
} // namespace utils

#endif // SIK_NETWORMS_FINGERPRINT_H
