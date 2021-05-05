#ifndef SIK_NETWORMS_GETOPT_WRAPPER_H
#define SIK_NETWORMS_GETOPT_WRAPPER_H

#include <string>
#include <unordered_map>
#include <vector>

namespace utils {
    void parseCmdParameters(int argc, char **argv, const std::string &accepted,
                            std::unordered_map<char, std::string> &params,
                            std::vector<std::string> &positionalParameters);
}

#endif // SIK_NETWORMS_GETOPT_WRAPPER_H
