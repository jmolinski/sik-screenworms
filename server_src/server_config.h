#ifndef SIK_NETWORMS_SERVER_CONFIG_H
#define SIK_NETWORMS_SERVER_CONFIG_H

#include <cstdint>
#include <string>
#include <unordered_map>

struct ServerConfig {
    uint16_t port;
    uint32_t rngSeed;
    uint8_t turningSpeed;
    uint16_t roundsPerSec;
    uint16_t boardWidth, boardHeight;

    explicit ServerConfig(std::unordered_map<char, std::string> params);
};

#endif // SIK_NETWORMS_SERVER_CONFIG_H
