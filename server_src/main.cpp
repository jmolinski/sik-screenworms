#include "../common/getopt_wrapper.h"
#include "server.h"
#include <iostream>

namespace {
    ServerConfig parseCmdParams(int argc, char *argv[]) {
        try {
            std::unordered_map<char, std::string> params;
            std::vector<std::string> positionalParameters;
            utils::parseCmdParameters(argc, argv, "pstvwh", params, positionalParameters);

            if (!positionalParameters.empty()) {
                throw std::runtime_error(positionalParameters[0] + ": unexpected positional parameter passed");
            }

            return ServerConfig(params);
        } catch (const std::exception &e) {
            std::cerr << "ERROR: " << e.what() << '\n';
            std::cerr << "Usage: ./screen-worms-server [-p n] [-s n] [-t n] [-v n] [-w n] [-h n]" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
} // namespace

int main(int argc, char *argv[]) {
    Server(::parseCmdParams(argc, argv)).run();

    return 0;
}
