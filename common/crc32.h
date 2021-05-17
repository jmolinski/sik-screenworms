#ifndef SIK_NETWORMS_CRC32_H
#define SIK_NETWORMS_CRC32_H

#include <cstdint>
#include <cstring>

namespace utils {
    uint32_t crc32(const unsigned char *buff, size_t len);
}

#endif // SIK_NETWORMS_CRC32_H
