#include "../utils/getopt_wrapper.h"
#include "../utils/utils.h"
#include <exception>
#include <iostream>
#include <stdexcept>
#include <utility>

namespace {
    constexpr uint16_t DEFAULT_SERVER_PORT = 2021;
    constexpr uint16_t DEFAULT_GUI_PORT = 20210;
    const std::string DEFAULT_GUI_SERVER = "localhost";

    constexpr uint16_t MIN_PORT = 1024;
    constexpr uint16_t MAX_PORT = 65535;
} // namespace

struct ClientConfig {
    std::string playerName;
    uint16_t serverPort;
    std::string guiServer;
    uint16_t guiPort;
    std::string gameServer;

    ClientConfig(std::string gameServerParam, std::unordered_map<char, std::string> params)
        : serverPort{DEFAULT_SERVER_PORT}, guiServer{DEFAULT_GUI_SERVER}, guiPort{DEFAULT_GUI_PORT},
          gameServer{std::move(gameServerParam)} {
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

        if (!utils::isSyntacticallyValidHostAddress(gameServer)) {
            throw std::runtime_error("invalid game_server parameter value");
        }
        if ((it = params.find('i')) != params.end()) {
            guiServer = it->second;
            if (!utils::isSyntacticallyValidHostAddress(guiServer)) {
                throw std::runtime_error("invalid gui_server parameter value");
            }
        }
    }
};

namespace {
    ClientConfig parseCmdParams(int argc, char *argv[]) {
        try {
            std::unordered_map<char, std::string> params;
            std::vector<std::string> positionalParameters;
            utils::parseCmdParameters(argc, argv, "npir", params, positionalParameters);

            if (positionalParameters.empty()) {
                throw std::runtime_error("missing required game_server parameter");
            } else if (positionalParameters.size() > 1) {
                throw std::runtime_error("too many positional parameters passed");
            }

            return ClientConfig(positionalParameters[0], params);
        } catch (const std::exception &e) {
            std::cerr << "ERROR: " << e.what() << '\n';
            std::cerr << "Usage: ./screen-worms-client game_server [-n player_name] [-p n] [-i gui_server] [-r n]"
                      << std::endl;
            exit(EXIT_FAILURE);
        }
    }
} // namespace

int main(int argc, char *argv[]) {
    [[maybe_unused]] ClientConfig config = ::parseCmdParams(argc, argv);

    return 0;
}
