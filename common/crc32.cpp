#include "crc32.h"
#include <array>

namespace {
    constexpr std::array<uint32_t, 256> generate_crc2_table() {
        std::array<uint32_t, 256> arr{0};

        uint64_t POLYNOMIAL = 0xEDB88320;
        uint8_t b = 0;
        do {
            // Start with the data byte
            uint64_t remainder = b;
            for (unsigned long bit = 8; bit > 0; --bit) {
                if (remainder & 1) {
                    remainder = (remainder >> 1) ^ POLYNOMIAL;
                } else {
                    remainder = (remainder >> 1);
                }
            }
            arr[b] = static_cast<uint32_t>(remainder);
        } while (0 != ++b);

        return arr;
    }

    constexpr std::array<uint32_t, 256> crc2_table = generate_crc2_table();
} // namespace

namespace utils {
    uint32_t crc32(const unsigned char *data, size_t len) {
        uint32_t crc32Value = 0xFFFFFFFF;

        for (size_t i = 0; i < len; i++) {
            uint8_t nLookupIndex = (crc32Value ^ data[i]) & 0xFF;
            crc32Value = (crc32Value >> 8) ^ ::crc2_table[nLookupIndex];
        }

        crc32Value ^= 0xFFFFFFFF;
        return crc32Value;
    }
} // namespace utils
