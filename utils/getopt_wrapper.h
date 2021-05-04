#ifndef SIK_NETWORMS_GETOPT_WRAPPER_H
#define SIK_NETWORMS_GETOPT_WRAPPER_H

#include <string>
#include <unordered_map>

namespace utils {
    std::unordered_map<char, std::string> parseOptParameters(int argc, char **argv, const std::string &accepted);
}

#endif // SIK_NETWORMS_GETOPT_WRAPPER_H
