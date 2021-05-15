#include "server_config.h"
#include "../utils/misc.h"

namespace {
    constexpr uint16_t DEFAULT_PORT = 2021;
    constexpr uint8_t DEFAULT_TURNING_SPEED = 6;
    constexpr uint8_t DEFAULT_ROUNDS_PER_SEC = 50;
    constexpr uint16_t DEFAULT_WIDTH = 640;
    constexpr uint16_t DEFAULT_HEIGHT = 480;

    constexpr uint16_t MIN_PORT = 1024;
    constexpr uint16_t MAX_PORT = 65535;
    constexpr uint8_t MIN_TURNING_SPEED = 1;
    constexpr uint8_t MAX_TURNING_SPEED = 25;
    constexpr uint8_t MIN_ROUNDS_PER_SEC = 10;
    constexpr uint8_t MAX_ROUNDS_PER_SEC = 100;
    constexpr uint16_t MIN_BOARD_DIM_SIZE = 100;
    constexpr uint16_t MAX_BOARD_DIM_SIZE = 1920;
} // namespace

ServerConfig::ServerConfig(std::unordered_map<char, std::string> params)
    : port{DEFAULT_PORT}, rngSeed{static_cast<uint32_t>(time(nullptr))}, turningSpeed{DEFAULT_TURNING_SPEED},
      roundsPerSec{DEFAULT_ROUNDS_PER_SEC}, boardWidth{DEFAULT_WIDTH}, boardHeight{DEFAULT_HEIGHT} {
    std::unordered_map<char, std::string>::iterator it;

    if ((it = params.find('p')) != params.end()) {
        port = utils::optionalParamValueToUint<uint16_t, MIN_PORT, MAX_PORT>(it);
    }
    if ((it = params.find('s')) != params.end()) {
        rngSeed = utils::optionalParamValueToUint<uint32_t, 0, std::numeric_limits<uint32_t>::max()>(it);
    }
    if ((it = params.find('t')) != params.end()) {
        turningSpeed = utils::optionalParamValueToUint<uint8_t, MIN_TURNING_SPEED, MAX_TURNING_SPEED>(it);
    }
    if ((it = params.find('v')) != params.end()) {
        roundsPerSec = utils::optionalParamValueToUint<uint8_t, MIN_ROUNDS_PER_SEC, MAX_ROUNDS_PER_SEC>(it);
    }
    if ((it = params.find('w')) != params.end()) {
        boardWidth = utils::optionalParamValueToUint<uint16_t, MIN_BOARD_DIM_SIZE, MAX_BOARD_DIM_SIZE>(it);
    }
    if ((it = params.find('h')) != params.end()) {
        boardHeight = utils::optionalParamValueToUint<uint16_t, MIN_BOARD_DIM_SIZE, MAX_BOARD_DIM_SIZE>(it);
    }
}
