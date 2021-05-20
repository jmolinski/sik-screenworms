#ifndef SIK_NETWORMS_RNG_H
#define SIK_NETWORMS_RNG_H

#include <cstdint>

class RNG {
    uint32_t v;

  public:
    explicit RNG(uint32_t seed) : v(seed) {
    }

    uint32_t generateNext() {
        uint32_t toReturn = v;
        v = static_cast<uint32_t>((static_cast<uint64_t>(v) * 279410273) % 4294967291);
        return toReturn;
    }
};

#endif // SIK_NETWORMS_RNG_H
