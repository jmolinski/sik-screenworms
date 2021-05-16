#include "client_config.h"
#include "../utils/misc.h"

namespace {
    constexpr uint16_t DEFAULT_SERVER_PORT = 2021;
    constexpr uint16_t DEFAULT_GUI_PORT = 20210;
    const std::string DEFAULT_GUI_SERVER = "localhost";

    constexpr uint16_t MIN_PORT = 1024;
    constexpr uint16_t MAX_PORT = 65535;
} // namespace

ClientConfig::ClientConfig(const std::string &server, std::unordered_map<char, std::string> params)
    : serverPort{DEFAULT_SERVER_PORT}, guiServer{DEFAULT_GUI_SERVER}, guiPort{DEFAULT_GUI_PORT}, gameServer{server} {
    std::unordered_map<char, std::string>::iterator it;

    if ((it = params.find('p')) != params.end()) {
        serverPort = utils::optionalParamValueToUint<uint16_t, MIN_PORT, MAX_PORT>(it);
    }
    if ((it = params.find('r')) != params.end()) {
        guiPort = utils::optionalParamValueToUint<uint16_t, MIN_PORT, MAX_PORT>(it);
    }

    if ((it = params.find('n')) != params.end()) {
        playerName = it->second;
        if (!utils::isValidPlayerName(playerName)) {
            throw std::runtime_error("invalid player_name parameter value");
        }
    }

    if ((it = params.find('i')) != params.end()) {
        guiServer = it->second;
    }
}
