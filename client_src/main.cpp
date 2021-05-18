#include "../common/getopt_wrapper.h"
#include "client.h"
#include <iostream>
#include <csignal>

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
    signal(SIGPIPE, SIG_IGN);

    Client(::parseCmdParams(argc, argv)).run();
}
