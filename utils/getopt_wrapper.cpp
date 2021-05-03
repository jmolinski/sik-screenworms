#include "getopt_wrapper.h"

#include <sstream>
#include <unistd.h>

namespace {
    std::string interweaveWithColons(const std::string &s) {
        std::ostringstream os;
        for (const char &c : s) {
            os << c << ':';
        }
        return os.str();
    }
} // namespace

namespace utils {
    std::unordered_map<char, std::string> parseOptParameters(int argc, char **argv,
                                                             const std::string &accepted) {
        std::unordered_map<char, std::string> params;
        std::string getoptOptstring = ::interweaveWithColons(accepted);

        int c;
        while ((c = getopt(argc, argv, getoptOptstring.c_str())) != -1) {
            char optname = static_cast<char>(c);
            if (optname == '?') {
                throw std::runtime_error("invalid parameter or missing value");
            } else if (accepted.find(optname) == std::string::npos) {
                throw std::runtime_error("unrecognized option " + std::string(1, optname));
            } else if (params.find(optname) != params.end()) {
                throw std::runtime_error("duplicate parameter " + std::string(1, optname));
            } else {
                params.insert({optname, std::string(optarg)});
            }
        }

        if (optind < argc) {
            throw std::runtime_error("unexpected argument passed");
        }

        return params;
    }
} // namespace utils
