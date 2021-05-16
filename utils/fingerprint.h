#ifndef SIK_NETWORMS_FINGERPRINT_H
#define SIK_NETWORMS_FINGERPRINT_H

#include <arpa/inet.h>
#include <string>

std::string fingerprintNetuser(const sockaddr * addr);

#endif // SIK_NETWORMS_FINGERPRINT_H
